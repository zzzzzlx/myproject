#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "v4l2.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void displayFrame(const QImage &frame);

private:
    Ui::MainWindow *ui;
    v4l2 *m_camera;


};
#endif // MAINWINDOW_H
