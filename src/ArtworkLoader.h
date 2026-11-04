#pragma once
#include <QObject>
#include <QColor>
#include <atomic>

class ArtworkLoader final : public QObject {
    Q_OBJECT
public:
    void enqueue(QByteArray bytes, quint64 serial);
    void cancel(quint64 serial) { m_latest.store(serial); }
signals:
    void prepared(quint64 serial, const QString &source, const QColor &accent, const QString &background);
private:
    std::atomic<quint64> m_latest{0};
};
