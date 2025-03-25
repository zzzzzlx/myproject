#include "mainwindow.h"
#include "v4l2.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    /*QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();*/

    v4l2 v4l2;
    v4l2.v4l2_open();
    v4l2.v4l2_close();
    return 0;
}
