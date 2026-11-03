#include "ReceiverController.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QImage>
#include <QtEndian>

static QByteArray item(const QByteArray &tag, const QByteArray &value) {
    QByteArray data = tag;
    const quint32 length = qToBigEndian(quint32(value.size()));
    data.append(reinterpret_cast<const char *>(&length), 4);
    return data + value;
}
static QString track(const QByteArray &fields) {
    return "AIR2WIN_METADATA " + QString::fromLatin1(item("mlit", fields).toBase64());
}

QString fakeBackend;
class ReceiverTests : public QObject {
    Q_OBJECT
private slots:
    void musicMetadataIsBoundedAndCleared() {
        NowPlaying model;
        const auto valid = track(item("minm", "A song") + item("asar", QByteArray::fromHex("426ac3b6726b")) + item("asal", "An album"));
        QVERIFY(model.consumeLine(valid));
        QVERIFY(model.available());
        QCOMPARE(model.title(), "A song");
        QCOMPARE(model.artist(), QString::fromUtf8("Bj\xc3\xb6rk"));
        QCOMPARE(model.album(), "An album");
        // A truncated DMAP field must not replace the current track.
        model.consumeLine(track(QByteArray("minm\xff\xff\xff\xff", 8)));
        model.consumeLine("AIR2WIN_METADATA invalid-base64!");
        QCOMPARE(model.title(), "A song");
        model.consumeLine("AIR2WIN_PROGRESS 12000 90000");
        QCOMPARE(model.position(), 12);
        QCOMPARE(model.duration(), 90);
        model.consumeLine("AIR2WIN_PROGRESS -1 90000");
        model.consumeLine("AIR2WIN_PROGRESS 100000 90000");
        QCOMPARE(model.position(), 12);
        model.consumeLine(track(item("minm", "Next") + item("asaa", "Album artist")));
        QCOMPARE(model.artist(), "Album artist");
        QCOMPARE(model.duration(), 0);
        model.consumeLine("AIR2WIN_AUDIO");
        QVERIFY(model.receiving());
        model.clear();
        QVERIFY(!model.available());
        QVERIFY(!model.receiving());
        QVERIFY(model.title().isEmpty());
    }
    void artworkIsValidatedAndTransient() {
        QTemporaryDir dir;
        const auto path = dir.path() + "/cover";
        QImage cover(100, 100, QImage::Format_RGB32);
        cover.fill(QColor("#7297aa"));
        QVERIFY(cover.save(path, "PNG"));
        NowPlaying model;
        model.consumeLine("AIR2WIN_ARTWORK", path);
        QVERIFY(model.artwork().startsWith("data:image/png;base64,"));
        QVERIFY(model.available());
        const auto previous = model.artwork();
        QFile invalid(path);
        QVERIFY(invalid.open(QIODevice::WriteOnly | QIODevice::Truncate));
        invalid.write("not an image"); invalid.close();
        model.consumeLine("AIR2WIN_ARTWORK", path);
        QCOMPARE(model.artwork(), previous);
        QImage empty(1, 1, QImage::Format_RGB32); empty.fill(Qt::black);
        QVERIFY(empty.save(path, "PNG"));
        model.consumeLine("AIR2WIN_ARTWORK", path);
        QVERIFY(model.artwork().isEmpty());
        QVERIFY(!model.available());
    }
    void missingBackend() {
        QTemporaryDir dir;
        ReceiverController receiver(dir.path());
        receiver.start();
        QCOMPARE(receiver.state(), "error");
        QVERIFY(!receiver.active());
    }
    void musicBridgeClearsOnDisconnect() {
        QTemporaryDir dir;
        ReceiverController receiver(dir.path(), fakeBackend);
        QVERIFY(receiver.saveSettings("Music", false));
        receiver.start();
        QTRY_VERIFY(receiver.nowPlaying()->available());
        QCOMPARE(receiver.nowPlaying()->title(), "Test track");
        QCOMPARE(receiver.nowPlaying()->position(), 12);
        QVERIFY(receiver.nowPlaying()->receiving());
        QVERIFY(!receiver.diagnostics().contains("AIR2WIN_METADATA"));
        QTRY_VERIFY(!receiver.nowPlaying()->available());
        QCOMPARE(receiver.state(), "listening");
        receiver.stop();
        QTRY_COMPARE(receiver.state(), "idle");
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
