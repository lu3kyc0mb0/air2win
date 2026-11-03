#pragma once
#include <QObject>
#include <QColor>
#include <QElapsedTimer>
#include <QTimer>

class NowPlaying final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString artist READ artist NOTIFY changed)
    Q_PROPERTY(QString album READ album NOTIFY changed)
    Q_PROPERTY(QString artwork READ artwork NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(int position READ position NOTIFY changed)
    Q_PROPERTY(int duration READ duration NOTIFY changed)
    Q_PROPERTY(bool receiving READ receiving NOTIFY changed)
public:
    explicit NowPlaying(QObject *parent = nullptr);
    bool available() const { return !m_title.isEmpty() || !m_artist.isEmpty() || !m_album.isEmpty() || !m_artwork.isEmpty(); }
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString artwork() const { return m_artwork; }
    QColor accent() const { return m_accent; }
    int position() const { return m_position; }
    int duration() const { return m_duration; }
    bool receiving() const { return m_receiving; }
    bool consumeLine(const QString &line, const QString &artworkPath = {});
    void clear();
signals:
    void changed();
    void availableChanged();
private:
    void metadata(const QByteArray &bytes);
    void artworkFile(const QString &path);
    void notify(bool wasAvailable);
    QString m_title, m_artist, m_album, m_artwork;
    QColor m_accent{"#786a72"};
    int m_position = 0, m_duration = 0;
    bool m_receiving = false;
    QElapsedTimer m_lastAudio;
    QTimer m_activityTimer;
};
