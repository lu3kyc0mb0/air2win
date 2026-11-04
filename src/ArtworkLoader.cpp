#include "ArtworkLoader.h"
#include <QBuffer>
#include <QImageReader>
#include <QPainter>
#include <QPainterPath>

void ArtworkLoader::enqueue(QByteArray bytes, quint64 serial) {
    m_latest.store(serial);
    QMetaObject::invokeMethod(this, [this, bytes = std::move(bytes), serial]() mutable {
        if (serial != m_latest.load()) return;
        QBuffer input(&bytes);
        input.open(QIODevice::ReadOnly);
        QImageReader reader(&input);
        if (reader.format() != "png" && reader.format() != "jpeg") return;
        const auto size = reader.size();
        if (!size.isValid() || size.width() > 8192 || size.height() > 8192 ||
            qint64(size.width()) * size.height() > 32000000) return;
        if (size.width() <= 1 && size.height() <= 1) {
            if (serial == m_latest.load()) emit prepared(serial, {}, QColor("#786a72"));
            return;
        }
        reader.setAutoTransform(true);
        reader.setScaledSize(size.scaled(688, 688, Qt::KeepAspectRatio));
        const auto image = reader.read();
        if (image.isNull() || serial != m_latest.load()) return;
        const auto accent = image.scaled(1, 1, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).pixelColor(0, 0);
        QImage rounded(688, 688, QImage::Format_ARGB32_Premultiplied);
        rounded.fill(Qt::transparent);
        {
            QPainter painter(&rounded);
            painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
            QPainterPath outline;
            outline.addRoundedRect(QRectF(0, 0, 688, 688), 32, 32);
            painter.setClipPath(outline);
            const auto cover = image.size().scaled(688, 688, Qt::KeepAspectRatioByExpanding);
            painter.drawImage(QRectF((688 - cover.width()) / 2.0, (688 - cover.height()) / 2.0,
                                    cover.width(), cover.height()), image);
        }
        QByteArray png;
        QBuffer output(&png);
        output.open(QIODevice::WriteOnly);
        if (!rounded.save(&output, "PNG") || serial != m_latest.load()) return;
        emit prepared(serial, "data:image/png;base64," + QString::fromLatin1(png.toBase64()), accent);
    }, Qt::QueuedConnection);
}
