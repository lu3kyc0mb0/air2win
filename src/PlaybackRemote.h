#pragma once
#include <QObject>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUdpSocket>
#include <QTimer>
#include <QSet>

class PlaybackRemote final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool canSeek READ canSeek NOTIFY changed)
    Q_PROPERTY(bool stateKnown READ stateKnown NOTIFY changed)
    Q_PROPERTY(bool playing READ playing NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
public:
    explicit PlaybackRemote(QObject *parent = nullptr);
    ~PlaybackRemote() override;
    bool available() const { return m_port != 0; }
    bool busy() const { return m_busy; }
    bool canSeek() const { return available() && !m_seekRejected; }
    bool stateKnown() const { return m_stateKnown; }
    bool playing() const { return m_playing; }
    QString error() const { return m_error; }
    void setSession(const QString &id, const QString &token, const QHostAddress &peer);
    void clear();
    // Accept only a service matching this session and a response from its sender.
    void acceptDiscovery(const QByteArray &packet, const QHostAddress &sender);
    static quint16 servicePort(const QByteArray &packet, const QString &instance);
    Q_INVOKABLE void playPause();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void next();
    Q_INVOKABLE void seek(int seconds);
    Q_INVOKABLE void setMonitoring(bool enabled);
signals:
    void changed();
    void progressReceived(int positionMs, int durationMs);
private:
    void discover();
    void poll();
    void request(const QString &command, bool action, bool seeking = false);
    void parseStatus(const QByteArray &body);
    QString m_id, m_token, m_error;
    QHostAddress m_peer;
    quint16 m_port = 0;
    int m_generation = 0, m_attempts = 0;
    bool m_busy = false, m_seekRejected = false, m_stateKnown = false, m_playing = false, m_monitoring = false;
    QUdpSocket m_dns;
    QTimer m_discoveryTimer, m_pollTimer;
    QNetworkAccessManager m_http;
    QSet<QNetworkReply *> m_replies;
};
