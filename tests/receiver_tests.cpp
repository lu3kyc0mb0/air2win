#include "ReceiverController.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

QString fakeBackend;
class ReceiverTests : public QObject {
    Q_OBJECT
private slots:
    void missingBackend() {
        QTemporaryDir dir;
        ReceiverController receiver(dir.path());
        receiver.start();
        QCOMPARE(receiver.state(), "error");
        QVERIFY(!receiver.active());
    }
    void settingsArePortableAndValidated() {
        QTemporaryDir dir;
        ReceiverController receiver(dir.path());
        QVERIFY(receiver.saveSettings("Living room", false));
        QVERIFY(QFile::exists(dir.path() + "/data/settings.json"));
        ReceiverController restored(dir.path());
        QCOMPARE(restored.receiverName(), "Living room");
        QVERIFY(!restored.requirePin());
        QVERIFY(!receiver.saveSettings("", true));
        QVERIFY(!receiver.saveSettings("bad.name", true));
        QVERIFY(!receiver.saveSettings(QString(60, QChar('a')), true));
        QCOMPARE(receiver.receiverName(), "Living room");
    }
    void unwritableDataReportsError() {
        QTemporaryDir dir;
        QFile file(dir.path() + "/data");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        ReceiverController receiver(dir.path(), fakeBackend);
        QVERIFY(!receiver.saveSettings("Air2Win", true));
        QCOMPARE(receiver.state(), "error");
    }
    void confirmedLifecycleAndRestart() {
        QTemporaryDir dir;
        ReceiverController receiver(dir.path(), fakeBackend);
        receiver.start();
        QCOMPARE(receiver.state(), "starting");
        QVERIFY(!receiver.saveSettings("Other", false));
        QTRY_COMPARE(receiver.state(), "listening");
        QCOMPARE(receiver.pin().size(), 4);
        QTRY_COMPARE(receiver.state(), "streaming");
        QTRY_COMPARE(receiver.state(), "listening");
        receiver.stop();
        QTRY_COMPARE(receiver.state(), "idle");
        QVERIFY(!receiver.active());
        QVERIFY(receiver.pin().isEmpty());
        receiver.start();
        QTRY_COMPARE(receiver.state(), "listening");
        receiver.stop();
        QTRY_COMPARE(receiver.state(), "idle");
    }
    void earlyExitIsNotReady() {
        QTemporaryDir dir;
        ReceiverController receiver(dir.path(), fakeBackend);
        QVERIFY(receiver.saveSettings("Fail", true));
        receiver.start();
        QTRY_COMPARE(receiver.state(), "error");
        QVERIFY(receiver.error().contains("7"));
        QVERIFY(!receiver.active());
    }
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) return 1;
    fakeBackend = QString::fromLocal8Bit(argv[1]);
    ReceiverTests tests;
    int testArgc = 1;
    return QTest::qExec(&tests, testArgc, argv);
}
#include "receiver_tests.moc"
