#pragma once
#include <QObject>
#include <QProcess>
#include <QTimer>

class ReceiverController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(QString receiverName READ receiverName NOTIFY settingsChanged)
    Q_PROPERTY(bool requirePin READ requirePin NOTIFY settingsChanged)
    Q_PROPERTY(QString pin READ pin NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(QString diagnostics READ diagnostics NOTIFY diagnosticsChanged)
    Q_PROPERTY(QString localAddresses READ localAddresses CONSTANT)
public:
    explicit ReceiverController(QString root, QString backend = {}, QObject *parent = nullptr);
    ~ReceiverController() override;
    QString state() const { return m_state; }
    bool active() const;
    bool available() const;
    QString receiverName() const { return m_name; }
    bool requirePin() const { return m_requirePin; }
    QString pin() const { return m_pin; }
    QString error() const { return m_error; }
    QString diagnostics() const { return m_lines.join('\n'); }
    QString localAddresses() const;
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE bool saveSettings(const QString &name, bool requirePin);
    Q_INVOKABLE void clearDiagnostics();
signals:
    void stateChanged();
    void settingsChanged();
    void diagnosticsChanged();
private:
    void transition(const QString &state, const QString &error = {});
    void readOutput();
    void appendLine(const QString &line);
    bool ensureDataDirectory();
    QString m_root, m_backend, m_data;
    QString m_state = "idle", m_error, m_name = "Air2Win", m_pin;
    bool m_requirePin = true;
    QStringList m_lines;
    QByteArray m_pending;
    QProcess m_process;
    QTimer m_startTimer, m_stopTimer;
    void *m_job = nullptr;
};
