#include "ReceiverController.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QStandardPaths>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

ReceiverController::ReceiverController(QString root, QString backend, QObject *parent)
    : QObject(parent), m_root(QDir(root).absolutePath()),
      m_backend(backend.isEmpty() ? m_root + "/runtime/uxplay.exe" : backend),
      m_data(m_root + "/data") {
    QFile settings(m_data + "/settings.json");
    if (settings.open(QIODevice::ReadOnly)) {
        const auto object = QJsonDocument::fromJson(settings.readAll()).object();
        const QString savedName = object.value("receiverName").toString();
        if (!savedName.trimmed().isEmpty()) m_name = savedName;
        m_requirePin = object.value("requirePin").toBool(true);
    }
    m_process.setProcessChannelMode(QProcess::MergedChannels);
#ifdef Q_OS_WIN
    m_process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_NO_WINDOW;
    });
#endif
    m_startTimer.setSingleShot(true);
    m_startTimer.setInterval(15000);
    m_stopTimer.setSingleShot(true);
    m_stopTimer.setInterval(1500);
    connect(&m_startTimer, &QTimer::timeout, this, [this] {
        transition("error", tr("The receiver did not start within 15 seconds. Check the log."));
        m_process.kill();
    });
    connect(&m_stopTimer, &QTimer::timeout, &m_process, &QProcess::kill);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &ReceiverController::readOutput);
    connect(&m_process, &QProcess::started, this, [this] {
#ifdef Q_OS_WIN
        m_job = CreateJobObjectW(nullptr, nullptr);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        HANDLE child = OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE, FALSE,
                                   static_cast<DWORD>(m_process.processId()));
        const bool attached = m_job && SetInformationJobObject(m_job, JobObjectExtendedLimitInformation,
                                                               &info, sizeof(info)) && child &&
                              AssignProcessToJobObject(m_job, child);
        if (child) CloseHandle(child);
        if (!attached) {
            if (m_job) { CloseHandle(m_job); m_job = nullptr; }
            transition("error", tr("Could not attach the receiver to this app. The receiver has been stopped."));
            m_process.kill();
        }
#endif
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (m_state == "stopping" || m_state == "error") return;
        if (error == QProcess::FailedToStart) {
            m_startTimer.stop();
            appendLine(m_process.errorString());
            transition("error", tr("Could not start the receiver. Check the log."));
        }
    });
    connect(&m_process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        readOutput();
        if (!m_pending.isEmpty()) { appendLine(QString::fromUtf8(m_pending)); m_pending.clear(); }
        m_startTimer.stop();
        m_stopTimer.stop();
#ifdef Q_OS_WIN
        if (m_job) { CloseHandle(m_job); m_job = nullptr; }
#endif
        m_pin.clear();
        if (m_state == "stopping") transition("idle");
        else if (m_state != "error") transition("error", tr("The receiver exited (code %1). Check the log.").arg(code));
        else emit stateChanged();
    });
}

ReceiverController::~ReceiverController() {
    m_startTimer.stop();
    m_stopTimer.stop();
    if (m_process.state() != QProcess::NotRunning) {
        m_process.write("stop\n");
        if (!m_process.waitForFinished(1500)) {
            m_process.kill();
            m_process.waitForFinished(2000);
        }
    }
#ifdef Q_OS_WIN
    if (m_job) CloseHandle(m_job);
#endif
}

bool ReceiverController::active() const {
    return m_process.state() != QProcess::NotRunning || m_state == "starting" || m_state == "stopping";
}
bool ReceiverController::available() const { return QFileInfo::exists(m_backend); }

QString ReceiverController::localAddresses() const {
    QStringList addresses;
    for (const auto &iface : QNetworkInterface::allInterfaces()) {
        if (!(iface.flags() & QNetworkInterface::IsUp) || !(iface.flags() & QNetworkInterface::IsRunning)
            || (iface.flags() & QNetworkInterface::IsLoopBack)) continue;
        for (const auto &entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                addresses << entry.ip().toString();
        }
    }
    return addresses.isEmpty() ? tr("No local IPv4 address") : addresses.join(" · ");
}

void ReceiverController::transition(const QString &state, const QString &error) {
    m_state = state;
    m_error = error;
    emit stateChanged();
}

bool ReceiverController::ensureDataDirectory() {
    if (!QDir().mkpath(m_data)) {
        transition("error", tr("Cannot access the data folder. Move Air2Win to a writable folder."));
        return false;
    }
    QSaveFile probe(m_data + "/.write-check");
    if (!probe.open(QIODevice::WriteOnly)) {
        transition("error", tr("The app folder is read-only. Move Air2Win to a writable folder."));
        return false;
    }
    probe.cancelWriting();
    return true;
}

bool ReceiverController::saveSettings(const QString &name, bool requirePin) {
    if (active()) return false;
    const auto clean = name.trimmed();
    bool invalid = clean.isEmpty() || clean.toUtf8().size() > 50 || clean.contains('.') || clean.contains('\\') || clean.contains('/');
    for (auto character : clean) invalid |= !character.isPrint();
    if (invalid) {
        transition("error", tr("Use a name of at most 50 UTF-8 bytes, without dots, slashes, or control characters."));
        return false;
    }
    if (!ensureDataDirectory()) return false;
    QSaveFile file(m_data + "/settings.json");
    const auto bytes = QJsonDocument(QJsonObject{{"receiverName", clean}, {"requirePin", requirePin}}).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        transition("error", tr("Could not save settings next to the app."));
        return false;
    }
    m_name = clean;
    m_requirePin = requirePin;
    emit settingsChanged();
    transition("idle");
    return true;
}

void ReceiverController::start() {
    if (active()) return;
    if (!available()) {
        transition("error", tr("Receiver module not found. Extract the complete Air2Win package."));
        return;
    }
    if (!ensureDataDirectory()) return;
    // Explicit config prevents reading a user's unrelated ~/.uxplayrc.
    QFile config(m_data + "/uxplayrc");
    if (!config.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        transition("error", tr("Could not create receiver settings."));
        return;
    }
    config.close();
    m_pending.clear();
    m_pin = m_requirePin ? QString::number(QRandomGenerator::system()->bounded(1000, 10000)) : QString();
    auto env = QProcessEnvironment::systemEnvironment();
    const auto runtime = QFileInfo(m_backend).absolutePath();
    env.insert("PATH", QDir::toNativeSeparators(runtime) + ";" + env.value("PATH"));
    env.insert("GST_PLUGIN_PATH_1_0", runtime + "/plugins");
    env.insert("GST_PLUGIN_SYSTEM_PATH_1_0", runtime + "/plugins");
    env.insert("GST_REGISTRY_1_0", m_data + "/gstreamer-registry.bin");
    env.insert("GST_REGISTRY_FORK", "no");
    env.insert("XDG_CACHE_HOME", m_data + "/cache");
    env.insert("XDG_CONFIG_HOME", m_data);
    m_process.setProcessEnvironment(env);
    m_process.setWorkingDirectory(m_data);
    QStringList args{"-n", m_name, "-nh", "-rc", m_data + "/uxplayrc",
                     "-key", m_data + "/identity.key", "-s", "1920x1080", "-fps", "60",
                     "-vs", "d3d11videosink", "-as", "wasapisink", "-p", "7000"};
    // Password mode asks again on connection; legacy -pin pairing can be cached.
    if (m_requirePin) args << "-pw" << m_pin;
    appendLine(tr("Starting the AirPlay receiver…"));
    transition("starting");
    m_startTimer.start();
    m_process.start(m_backend, args);
}

void ReceiverController::stop() {
    if (!active() || m_state == "stopping") return;
    m_startTimer.stop();
    transition("stopping");
    m_process.write("stop\n");
    m_stopTimer.start();
}

void ReceiverController::readOutput() {
    m_pending += m_process.readAllStandardOutput();
    while (true) {
        const auto newline = m_pending.indexOf('\n');
        if (newline < 0) break;
        const auto line = QString::fromUtf8(m_pending.left(newline)).trimmed();
        m_pending.remove(0, newline + 1);
        appendLine(line);
        if (m_state == "stopping" || m_state == "error") continue;
        if (line == "AIR2WIN_READY") { m_startTimer.stop(); transition("listening"); }
        else if (line == "AIR2WIN_STREAMING") transition("streaming");
        else if (line == "AIR2WIN_IDLE" && (m_state == "streaming" || m_state == "listening")) transition("listening");
    }
    if (m_pending.size() > 16384) {
        appendLine(QString::fromUtf8(m_pending.left(16384)));
        m_pending.clear();
    }
}

void ReceiverController::appendLine(const QString &line) {
    if (line.isEmpty()) return;
    m_lines.append(line.left(2048));
    while (m_lines.size() > 200) m_lines.removeFirst();
    emit diagnosticsChanged();
}
void ReceiverController::clearDiagnostics() {
    m_lines.clear();
    emit diagnosticsChanged();
}
