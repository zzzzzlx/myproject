#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    /*myAudio = new Audio(this);*/
    myUsart = new usart(this);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete  myAudio;
    myAudio = nullptr;
}



void MainWindow::on_push_camera_clicked()
{
    camerashow *CameraShow = new camerashow();
    CameraShow->setAttribute(Qt::WA_DeleteOnClose);
    CameraShow->show();
    this->close();
}

void MainWindow::on_pushButton_led_clicked()
{
    myled = new led(this);
    if(Ledstatus == false)
    {
        Ledstatus = true;
        myled->setLedState(Ledstatus);
        ui->pushButton_led->setText("led off");
    }else{
        Ledstatus = false;
        myled->setLedState(Ledstatus);
        ui->pushButton_led->setText("led on");
    }
}

void MainWindow::on_pushButton_beep_clicked()
{
    mybeep = new beep(this);
    if(Beepstatus == false)
    {
        Beepstatus = true;
        mybeep->setLedState(Beepstatus);
        ui->pushButton_beep->setText("beep off");
    }else{
        Beepstatus = false;
        mybeep->setLedState(Beepstatus);
        ui->pushButton_beep->setText("beep on");
    }
}

void MainWindow::on_pushButton_audio_clicked()
{
    if(Audiostatus == false){
        Audiostatus = true;
       myAudio->startRecorder();
        ui->pushButton_audio->setText("audio off");
    }else{
        Audiostatus = false;
        myAudio->stopRecorder();
        ui->pushButton_audio->setText("audio on");

    }
}

void MainWindow::on_pushButton_test_clicked()
{
    myasr = new asr(this);
    QString fileName = QCoreApplication::applicationDirPath() + "/16k.wav";
    myasr->getTheResult(fileName);
}
