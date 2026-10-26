#include "ReceiverController.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QDir>
#include <QIcon>
#include <QFontDatabase>
#include <QLockFile>
#include <QTimer>
#include <cstdio>

int main(int argc, char *argv[]) {
    // Keep Qt's optional disk shader/QML caches inside the portable directory.
    qputenv("QML_DISABLE_DISK_CACHE", "1");
    qputenv("QSG_RHI_DISABLE_DISK_CACHE", "1");
    QGuiApplication app(argc, argv);
    app.setApplicationName("Air2Win");
    app.setOrganizationName("Air2Win");
    app.setApplicationVersion("1.0.0");
    QQuickStyle::setStyle("Basic");
    app.setWindowIcon(QIcon(":/branding/7629289.png"));
    QFont uiFont;
    const auto families = QFontDatabase::families();
    for (const auto &family : {QStringLiteral("SF Pro Text"), QStringLiteral("SF Pro Display"),
                               QStringLiteral("SF Pro"), QStringLiteral("Segoe UI Variable"), QStringLiteral("Segoe UI")}) {
        if (families.contains(family)) { uiFont.setFamily(family); break; }
    }
    uiFont.setPixelSize(14);
    app.setFont(uiFont);
    const QString root = QCoreApplication::applicationDirPath();
    QDir().mkpath(root + "/data");
    QLockFile lock(root + "/data/air2win.lock");
    if (!lock.tryLock(0)) return 2;
    ReceiverController receiver(root);
    if (app.arguments().contains("--receiver-check")) {
        QObject::connect(&receiver, &ReceiverController::stateChanged, &app, [&] {
            if (receiver.state() == "listening") {
                fprintf(stdout, "Receiver startup confirmed\n");
                fflush(stdout);
                QTimer::singleShot(1000, &receiver, &ReceiverController::stop);
            } else if (receiver.state() == "idle") app.exit(0);
            else if (receiver.state() == "error") {
                fprintf(stderr, "%s\n%s\n", receiver.error().toUtf8().constData(), receiver.diagnostics().toUtf8().constData());
                app.exit(1);
            }
        });
        QTimer::singleShot(0, &receiver, &ReceiverController::start);
        return app.exec();
    }
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("receiver", &receiver);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    bool qmlWarnings = false;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app,
                     [&](const QList<QQmlError> &) { qmlWarnings = true; });
    engine.loadFromModule("Air2Win", "Main");
    if (app.arguments().contains("--smoke-test")) {
        const bool checkReceiver = app.arguments().contains("--start-receiver");
        if (checkReceiver) receiver.start();
        QTimer::singleShot(checkReceiver ? 5000 : 1500, &app, [&, checkReceiver] {
            if (checkReceiver && receiver.state() != "listening") { app.exit(1); return; }
            bool saved = false;
            if (!engine.rootObjects().isEmpty()) {
                auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
                if (window) saved = window->grabWindow().save(root + "/data/preview.png");
            }
            if (!saved || qmlWarnings) { app.exit(1); return; }
            auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
            window->setProperty("page", 1);
            QTimer::singleShot(400, &app, [&, window] {
                if (!window->grabWindow().save(root + "/data/preview-settings.png")) { app.exit(1); return; }
                window->setProperty("page", 2);
                QTimer::singleShot(400, &app, [&, window] {
                    const bool result = window->grabWindow().save(root + "/data/preview-diagnostics.png");
                    app.exit(result && !qmlWarnings ? 0 : 1);
                });
            });
        });
    }
    return app.exec();
}
