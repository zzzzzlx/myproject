#include "camerashow.h"
#include "ui_camerashow.h"

camerashow::camerashow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::camerashow)
{
    ui->setupUi(this);

    timer = new QTimer(this);

    m_camera = new v4l2("/dev/video1", this);
    connect(m_camera, &v4l2::frameReady, this, &camerashow::displayFrame);
    connect(timer, &QTimer::timeout, m_camera, &v4l2::readFrame);
    connect(this, &camerashow::CameraStop, m_camera, &v4l2::camerastop);

    m_camera->start();
    timer->start(30);

}

camerashow::~camerashow()
{
    delete ui;
}

void camerashow::displayFrame(const QImage &image)
{
    // 将QImage转换为QPixmap并显示在QLabel上
    QPixmap pixmap = QPixmap::fromImage(image);
    ui->videoLabel->setPixmap(pixmap.scaled(ui->videoLabel->size(), Qt::KeepAspectRatio));
}


void camerashow::on_pushButton_back_clicked()
{
    timer->stop();          // 停止定时器
    emit CameraStop(false);
    this->close();
    MainWindow *s = new MainWindow();
    s->show();
}
