#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "v4l2/v4l2.h"
#include "v4l2/camerashow.h"
#include "ars/audio.h"
#include "gpio/led.h"
#include "gpio/beep.h"
#include "ars/audio.h"
#include "ars/asr.h"

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
    void on_pushButton_led_clicked();
    void on_pushButton_beep_clicked();

    void on_pushButton_audio_clicked();

    void on_pushButton_test_clicked();

private:
    Ui::MainWindow *ui;

    led *myled = nullptr;
    bool Ledstatus = false;

    beep *mybeep = nullptr;
    bool Beepstatus = false;

    Audio *myAudio = nullptr;
    bool Audiostatus = false;

    asr *myasr = nullptr;





};
#endif // MAINWINDOW_H
