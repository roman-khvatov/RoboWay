#include "tetrisemulator.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TetrisEmulator window;
    window.show();
    return app.exec();
}
