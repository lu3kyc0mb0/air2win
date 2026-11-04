#pragma once
#include <QObject>
#include <QColor>
#include <QElapsedTimer>
#include <QTimer>
#include "PlaybackRemote.h"

class NowPlaying final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(QString title READ title NOTIFY metadataChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY metadataChanged)
    Q_PROPERTY(QString album READ album NOTIFY metadataChanged)
    Q_PROPERTY(QString artwork READ artwork NOTIFY artworkChanged)
    Q_PROPERTY(QColor accent READ accent NOTIFY artworkChanged)
    Q_PROPERTY(int position READ position NOTIFY progressChanged)
    Q_PROPERTY(int duration READ duration NOTIFY progressChanged)
    Q_PROPERTY(bool receiving READ receiving NOTIFY receivingChanged)
    Q_PROPERTY(PlaybackRemote *controls READ controls CONSTANT)
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
    PlaybackRemote *controls() { return &m_controls; }
    bool consumeLine(const QString &line, const QString &artworkPath = {});
    void clear();
signals:
    void metadataChanged();
    void artworkChanged();
    void progressChanged();
    void receivingChanged();
    void availableChanged();
private:
    void metadata(const QByteArray &bytes);
    void artworkFile(const QString &path);
    void setProgress(int positionMs, int durationMs);
    QString m_title, m_artist, m_album, m_artwork;
    QColor m_accent{"#786a72"};
    int m_position = 0, m_duration = 0;
    bool m_receiving = false;
    QElapsedTimer m_lastAudio;
    QTimer m_activityTimer;
    PlaybackRemote m_controls{this};
};
