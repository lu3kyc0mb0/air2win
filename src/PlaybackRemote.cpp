#include "PlaybackRemote.h"
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QNetworkProxy>
#include <QRegularExpression>
#include <QUrlQuery>
#include <QtEndian>

namespace {
quint16 word(const QByteArray &data, qsizetype at) {
    return qFromBigEndian<quint16>(reinterpret_cast<const uchar *>(data.constData() + at));
}
quint32 number(const QByteArray &data, qsizetype at) {
    return qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(data.constData() + at));
}
QString dnsName(const QByteArray &data, qsizetype &offset) {
    QStringList labels;
    qsizetype cursor = offset;
    bool jumped = false;
    for (int depth = 0; depth < 128 && cursor < data.size(); ++depth) {
        const auto length = quint8(data.at(cursor++));
        if (!length) {
            if (!jumped) offset = cursor;
            return labels.join('.').toLower();
        }
        if ((length & 0xc0) == 0xc0) {
            if (cursor >= data.size()) break;
            if (!jumped) offset = cursor + 1;
            cursor = ((length & 0x3f) << 8) | quint8(data.at(cursor));
            jumped = true;
        } else {
            if (length > 63 || cursor + length > data.size()) break;
            labels << QString::fromLatin1(data.constData() + cursor, length);
            cursor += length;
        }
    }
    offset = -1;
    return {};
}
QByteArray dnsQuestion(const QString &name) {
    QByteArray query = QByteArray::fromHex("000000000001000000000000");
    for (const auto &label : name.toLatin1().split('.')) {
        query.append(char(label.size())); query.append(label);
    }
    query.append(QByteArray::fromHex("0000218001")); // SRV, unicast response requested
    return query;
}
}

quint16 PlaybackRemote::servicePort(const QByteArray &packet, const QString &instance) {
    if (packet.size() < 12 || packet.size() > 65535 || !(word(packet, 2) & 0x8000)) return 0;
    const int questions = word(packet, 4);
    const int records = word(packet, 6) + word(packet, 8) + word(packet, 10);
    if (questions > 64 || records > 512) return 0;
    qsizetype offset = 12;
    for (int i = 0; i < questions; ++i) {
        dnsName(packet, offset);
        if (offset < 0 || offset + 4 > packet.size()) return 0;
        offset += 4;
    }
    for (int i = 0; i < records; ++i) {
        const auto name = dnsName(packet, offset);
        if (offset < 0 || offset + 10 > packet.size()) return 0;
        const auto type = word(packet, offset);
        const auto recordClass = word(packet, offset + 2) & 0x7fff;
        const auto ttl = number(packet, offset + 4);
        const auto length = word(packet, offset + 8);
        offset += 10;
        if (offset + length > packet.size()) return 0;
        if (type == 33 && recordClass == 1 && ttl && length >= 7 && name == instance.toLower())
            return word(packet, offset + 4);
        offset += length;
    }
    return 0;
}

PlaybackRemote::PlaybackRemote(QObject *parent) : QObject(parent) {
    m_http.setProxy(QNetworkProxy::NoProxy);
    m_discoveryTimer.setInterval(2500);
    connect(&m_discoveryTimer, &QTimer::timeout, this, &PlaybackRemote::discover);
    m_pollTimer.setInterval(5000);
    connect(&m_pollTimer, &QTimer::timeout, this, &PlaybackRemote::poll);
    connect(&m_dns, &QUdpSocket::readyRead, this, [this] {
        while (m_dns.hasPendingDatagrams()) {
            const auto datagram = m_dns.receiveDatagram(65535);
            acceptDiscovery(datagram.data(), datagram.senderAddress());
        }
    });
}

PlaybackRemote::~PlaybackRemote() {
    const auto replies = m_replies;
    for (auto *reply : replies) { reply->disconnect(this); reply->abort(); }
}

void PlaybackRemote::clear() {
    ++m_generation;
    m_discoveryTimer.stop(); m_pollTimer.stop(); m_dns.close();
    const auto replies = m_replies;
    for (auto *reply : replies) reply->abort();
    m_id.clear(); m_token.clear(); m_peer.clear(); m_error.clear();
    m_port = 0; m_attempts = 0;
    m_busy = m_seekRejected = m_stateKnown = m_playing = false;
    emit changed();
}

void PlaybackRemote::setSession(const QString &id, const QString &token, const QHostAddress &peer) {
    static const QRegularExpression idPattern("^[0-9a-fA-F]{1,32}$"), tokenPattern("^[0-9]{1,20}$");
    if (!idPattern.match(id).hasMatch() || !tokenPattern.match(token).hasMatch() || peer.isNull() || peer.isMulticast()) return;
    if (m_id == id.toUpper() && m_token == token && m_peer == peer) return;
    clear();
    m_id = id.toUpper(); m_token = token; m_peer = peer;
    if (!m_dns.bind(peer.protocol() == QAbstractSocket::IPv6Protocol ? QHostAddress::AnyIPv6 : QHostAddress::AnyIPv4, 0)) return;
    discover();
    m_discoveryTimer.start();
}

void PlaybackRemote::discover() {
    if (m_port || m_id.isEmpty() || ++m_attempts > 12) { m_discoveryTimer.stop(); return; }
    const auto query = dnsQuestion("iTunes_Ctrl_" + m_id + "._dacp._tcp.local");
    m_dns.writeDatagram(query, m_peer, 5353);
    if (m_peer.protocol() == QAbstractSocket::IPv4Protocol && !m_peer.isLoopback()) {
        for (const auto &iface : QNetworkInterface::allInterfaces()) {
            if (!(iface.flags() & QNetworkInterface::IsUp) || !(iface.flags() & QNetworkInterface::CanMulticast) ||
                (iface.flags() & QNetworkInterface::IsLoopBack)) continue;
            m_dns.setMulticastInterface(iface);
            m_dns.writeDatagram(query, QHostAddress("224.0.0.251"), 5353);
        }
    }
}

void PlaybackRemote::acceptDiscovery(const QByteArray &packet, const QHostAddress &sender) {
    if (m_id.isEmpty() || sender != m_peer) return;
    const auto port = servicePort(packet, "iTunes_Ctrl_" + m_id + "._dacp._tcp.local");
    if (!port || port == m_port) return;
    m_port = port;
    m_discoveryTimer.stop();
    emit changed();
    poll();
    if (m_monitoring) m_pollTimer.start();
}

void PlaybackRemote::setMonitoring(bool enabled) {
    m_monitoring = enabled;
    if (enabled && available()) { poll(); m_pollTimer.start(); }
    else m_pollTimer.stop();
}

void PlaybackRemote::poll() {
    if (available() && !m_busy && m_replies.isEmpty()) request("playstatusupdate?revision-number=1", false);
}
void PlaybackRemote::playPause() { request("playpause", true); }
void PlaybackRemote::previous() { request("previtem", true); }
void PlaybackRemote::next() { request("nextitem", true); }
void PlaybackRemote::seek(int seconds) {
    if (canSeek() && seconds >= 0 && seconds <= 86400)
        request("setproperty?dacp.playingtime=" + QString::number(qint64(seconds) * 1000), true, true);
}

void PlaybackRemote::request(const QString &command, bool action, bool seeking) {
    if (!available() || (action && m_busy)) return;
    const int generation = m_generation;
    QUrl url; url.setScheme("http"); url.setHost(m_peer.toString()); url.setPort(m_port);
    url.setPath("/ctrl-int/1/" + command.section('?', 0, 0));
    if (command.contains('?')) url.setQuery(command.section('?', 1));
    QNetworkRequest req(url);
    req.setRawHeader("Active-Remote", m_token.toLatin1());
    req.setRawHeader("User-Agent", "Air2Win/1.0.0");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    req.setTransferTimeout(2500);
    if (action) { m_busy = true; m_error.clear(); emit changed(); }
    auto *reply = m_http.get(req);
    reply->setReadBufferSize(65537);
    m_replies.insert(reply);
    connect(reply, &QIODevice::readyRead, this, [reply] { if (reply->bytesAvailable() > 65536) reply->abort(); });
    connect(reply, &QNetworkReply::finished, this, [this, reply, generation, action, seeking] {
        m_replies.remove(reply);
        reply->deleteLater();
        if (generation != m_generation) return;
        const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool ok = reply->error() == QNetworkReply::NoError && code >= 200 && code < 300;
        if (action) {
            m_busy = false;
            if (!ok) {
                if (seeking) m_seekRejected = true;
                m_error = seeking ? tr("Seeking is not supported by this sender.") : tr("The sender did not accept this control.");
            }
            emit changed();
            if (ok) QTimer::singleShot(200, this, &PlaybackRemote::poll);
        } else if (ok) parseStatus(reply->readAll());
    });
}

void PlaybackRemote::parseStatus(const QByteArray &body) {
    if (body.size() < 8 || body.first(4) != "cmst" || number(body, 4) != quint32(body.size() - 8)) return;
    int state = -1;
    qint64 duration = -1, remaining = -1;
    for (qsizetype offset = 8; offset < body.size();) {
        if (offset + 8 > body.size()) return;
        const auto tag = body.sliced(offset, 4);
        const auto length = number(body, offset + 4);
        offset += 8;
        if (length > quint32(body.size() - offset)) return;
        if (tag == "caps" && length == 1) state = quint8(body.at(offset));
        else if (tag == "cast" && length == 4) duration = number(body, offset);
        else if (tag == "cant" && length == 4) remaining = number(body, offset);
        offset += length;
    }
    if (state >= 2 && state <= 4) {
        m_playing = state == 4; m_stateKnown = true;
        emit changed();
    }
    if (duration > 0 && duration <= 86400000 && remaining >= 0 && remaining <= duration)
        emit progressReceived(int(duration - remaining), int(duration));
}
