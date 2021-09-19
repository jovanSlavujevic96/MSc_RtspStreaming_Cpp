#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // disable av logs
    av_log_set_level(0);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
