#include "NowPlaying.h"
#include "ArtworkLoader.h"
#include <QCryptographicHash>
#include <QFile>
#include <QtEndian>
#include <algorithm>

NowPlaying::NowPlaying(QObject *parent) : QObject(parent) {
    m_activityTimer.setInterval(500);
    connect(&m_controls, &PlaybackRemote::progressReceived, this, &NowPlaying::setProgress);
    connect(&m_activityTimer, &QTimer::timeout, this, [this] {
        if (m_receiving && m_lastAudio.elapsed() > 2500) {
            m_receiving = false;
            m_activityTimer.stop();
            emit receivingChanged();
        }
    });
}

NowPlaying::~NowPlaying() {
    if (m_artworkLoader) {
        m_artworkLoader->cancel(++m_artworkSerial);
        m_artworkThread.quit();
        m_artworkThread.wait();
    }
}

void NowPlaying::clear() {
    const bool before = available();
    m_activityTimer.stop();
    if (m_artworkLoader) m_artworkLoader->cancel(++m_artworkSerial);
    m_artworkDigest.clear();
    m_title.clear(); m_artist.clear(); m_album.clear(); m_artwork.clear(); m_background.clear();
    m_accent = QColor("#786a72");
    m_position = m_duration = 0;
    m_receiving = false;
    m_controls.clear();
    emit metadataChanged(); emit artworkChanged(); emit progressChanged(); emit receivingChanged();
    if (before != available()) emit availableChanged();
}

void NowPlaying::metadata(const QByteArray &bytes) {
    if (bytes.size() < 8 || bytes.size() > 65536 || bytes.first(4) != "mlit") return;
    const auto lengthAt = [&](qsizetype offset) {
        return qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(bytes.constData() + offset));
    };
    if (lengthAt(4) != static_cast<quint32>(bytes.size() - 8)) return;
    QString title, artist, album, albumArtist;
    for (qsizetype offset = 8; offset < bytes.size();) {
        if (bytes.size() - offset < 8) return;
        const auto tag = bytes.sliced(offset, 4);
        const auto length = lengthAt(offset + 4);
        offset += 8;
        if (length > static_cast<quint32>(bytes.size() - offset)) return;
        if (tag == "minm" || tag == "asar" || tag == "asal" || tag == "asaa") {
            QString value = QString::fromUtf8(bytes.constData() + offset, std::min(length, quint32(4096)));
            value.remove(QChar('\0'));
            value = value.simplified().left(1024);
            if (tag == "minm") title = value;
            else if (tag == "asar") artist = value;
            else if (tag == "asal") album = value;
            else albumArtist = value;
        }
        offset += length;
    }
    if (artist.isEmpty()) artist = albumArtist;
    if (m_title == title && m_artist == artist && m_album == album) return;
    const bool before = available();
    m_title = title; m_artist = artist; m_album = album;
    m_position = m_duration = 0;
    emit metadataChanged(); emit progressChanged();
    if (before != available()) emit availableChanged();
}

void NowPlaying::artworkFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 8 * 1024 * 1024) return;
    auto bytes = file.readAll();
    const auto digest = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
    if (digest == m_artworkDigest) return;
    m_artworkDigest = digest;
    if (!m_artworkLoader) {
        m_artworkLoader = new ArtworkLoader;
        m_artworkLoader->moveToThread(&m_artworkThread);
        connect(&m_artworkThread, &QThread::finished, m_artworkLoader, &QObject::deleteLater);
        connect(m_artworkLoader, &ArtworkLoader::prepared, this,
                [this](quint64 serial, const QString &source, const QColor &accent, const QString &background) {
            if (serial != m_artworkSerial || (m_artwork == source && m_accent == accent)) return;
            const bool before = available();
            m_artwork = source;
            m_background = background;
            m_accent = accent;
            emit artworkChanged();
            if (before != available()) emit availableChanged();
        });
        m_artworkThread.start();
    }
    m_artworkLoader->enqueue(std::move(bytes), ++m_artworkSerial);
}

void NowPlaying::setProgress(int positionMs, int durationMs) {
    const int position = positionMs / 1000, duration = durationMs / 1000;
    if (m_position == position && m_duration == duration) return;
    m_position = position; m_duration = duration;
    emit progressChanged();
}

bool NowPlaying::consumeLine(const QString &line, const QString &artworkPath) {
    if (line.startsWith("AIR2WIN_METADATA ")) {
        const auto payload = line.sliced(17).toLatin1();
        if (payload.size() <= 90000) {
            const auto decoded = QByteArray::fromBase64Encoding(payload, QByteArray::AbortOnBase64DecodingErrors);
            if (decoded) metadata(decoded.decoded);
        }
    } else if (line == "AIR2WIN_ARTWORK") {
        artworkFile(artworkPath);
    } else if (line.startsWith("AIR2WIN_PROGRESS ")) {
        const auto parts = line.sliced(17).split(' ', Qt::SkipEmptyParts);
        bool positionOk = false, durationOk = false;
        const qint64 position = parts.value(0).toLongLong(&positionOk);
        const qint64 duration = parts.value(1).toLongLong(&durationOk);
        if (parts.size() == 2 && positionOk && durationOk && duration > 0 && duration <= 86400000 && position >= 0 && position <= duration) {
            setProgress(int(position), int(duration));
        }
    } else if (line == "AIR2WIN_AUDIO") {
        m_lastAudio.restart();
        m_activityTimer.start();
        if (!m_receiving) { m_receiving = true; emit receivingChanged(); }
    } else if (line.startsWith("AIR2WIN_REMOTE ")) {
        const auto parts = line.sliced(15).split(' ', Qt::SkipEmptyParts);
        if (parts.size() == 3) m_controls.setSession(parts[0], parts[1], QHostAddress(parts[2]));
    } else return false;
    return true;
}
