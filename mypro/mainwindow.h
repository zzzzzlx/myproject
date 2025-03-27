#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "v4l2/v4l2.h"
#include "v4l2/camerashow.h"

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


private slots:
    void on_push_camera_clicked();

private:
    Ui::MainWindow *ui;

};
#endif // MAINWINDOW_H
