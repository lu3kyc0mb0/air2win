#include <QCoreApplication>
#include <QTimer>
#include <QtEndian>
#include <cstdio>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    setvbuf(stdout, nullptr, _IONBF, 0);
    if (app.arguments().contains("Fail")) return 7;
    QTimer::singleShot(50, [] { puts("AIR2WIN_READY"); });
    QTimer::singleShot(150, [] { puts("AIR2WIN_STREAMING"); });
    const bool music = app.arguments().contains("Music");
    if (music) {
        QTimer::singleShot(180, [] {
            QByteArray title("minm");
            quint32 size = qToBigEndian(quint32(10));
            title.append(reinterpret_cast<const char *>(&size), 4);
            title.append("Test track");
            QByteArray listing("mlit");
            size = qToBigEndian(quint32(title.size()));
            listing.append(reinterpret_cast<const char *>(&size), 4);
            listing += title;
            printf("AIR2WIN_METADATA %s\n", listing.toBase64().constData());
            puts("AIR2WIN_PROGRESS 12000 90000");
            puts("AIR2WIN_AUDIO");
        });
    }
    QTimer::singleShot(music ? 800 : 350, [] { puts("AIR2WIN_IDLE"); });
    return app.exec();
}
