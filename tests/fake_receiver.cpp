#include <QCoreApplication>
#include <QTimer>
#include <cstdio>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    setvbuf(stdout, nullptr, _IONBF, 0);
    if (app.arguments().contains("Fail")) return 7;
    QTimer::singleShot(50, [] { puts("AIR2WIN_READY"); });
    QTimer::singleShot(150, [] { puts("AIR2WIN_STREAMING"); });
    QTimer::singleShot(350, [] { puts("AIR2WIN_IDLE"); });
    return app.exec();
}
