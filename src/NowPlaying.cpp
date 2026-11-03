#include "NowPlaying.h"
#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QtEndian>
#include <algorithm>

NowPlaying::NowPlaying(QObject *parent) : QObject(parent) {
    m_activityTimer.setInterval(500);
    connect(&m_activityTimer, &QTimer::timeout, this, [this] {
        if (m_receiving && m_lastAudio.elapsed() > 2500) {
            m_receiving = false;
            m_activityTimer.stop();
            emit changed();
        }
    });
}

void NowPlaying::notify(bool wasAvailable) {
    emit changed();
    if (wasAvailable != available()) emit availableChanged();
}

void NowPlaying::clear() {
    const bool before = available();
    m_activityTimer.stop();
    m_title.clear(); m_artist.clear(); m_album.clear(); m_artwork.clear();
    m_accent = QColor("#786a72");
    m_position = m_duration = 0;
    m_receiving = false;
    notify(before);
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
    notify(before);
}

void NowPlaying::artworkFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 8 * 1024 * 1024) return;
    QImageReader reader(&file);
    if (reader.format() != "png" && reader.format() != "jpeg") return;
    const auto size = reader.size();
    if (!size.isValid() || size.width() > 8192 || size.height() > 8192 ||
        qint64(size.width()) * size.height() > 32000000) return;
    const bool before = available();
    // UxPlay sends a one-pixel image to clear the previous cover.
    if (size.width() <= 1 && size.height() <= 1) {
        m_artwork.clear(); m_accent = QColor("#786a72");
        notify(before);
        return;
    }
    reader.setAutoTransform(true);
    reader.setScaledSize(size.scaled(640, 640, Qt::KeepAspectRatio));
    const auto image = reader.read();
    if (image.isNull()) return;
    const auto thumbnail = image.scaled(1, 1, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    m_accent = thumbnail.pixelColor(0, 0);
    QByteArray png;
    QBuffer output(&png);
    output.open(QIODevice::WriteOnly);
    if (!image.save(&output, "PNG")) return;
    m_artwork = "data:image/png;base64," + QString::fromLatin1(png.toBase64());
    notify(before);
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
            m_position = int(position / 1000); m_duration = int(duration / 1000);
            emit changed();
        }
    } else if (line == "AIR2WIN_AUDIO") {
        m_lastAudio.restart();
        m_activityTimer.start();
        if (!m_receiving) { m_receiving = true; emit changed(); }
    } else return false;
    return true;
}
