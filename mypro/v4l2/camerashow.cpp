#include "camerashow.h"
#include "ui_camerashow.h"

camerashow::camerashow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::camerashow)
{
    ui->setupUi(this);

    m_camera = new v4l2("/dev/video1", this);

    connect(m_camera, &v4l2::frameReady, this, &camerashow::displayFrame);/*, Qt::DirectConnection*/
    connect(this, &camerashow::frameReturn, m_camera, &v4l2::frameStatus);
    connect(this, &camerashow::StopSign, m_camera, &v4l2::cameraClean);
    m_camera->start();

}

camerashow::~camerashow()
{
    m_camera->cameraStop();
    m_camera->wait();
    delete ui;
}

void camerashow::displayFrame(const QImage &image, int index)
{
    if (!m_camera || !m_camera->isRunning()) {
            return; // 若摄像头已停止，直接返回
        }

    // 将QImage转换为QPixmap并显示在QLabel上
    QPixmap pixmap = QPixmap::fromImage(image);
    ui->videoLabel->setPixmap(pixmap.scaled(ui->videoLabel->size(), Qt::KeepAspectRatio));
    emit frameReturn(index);
}


void camerashow::on_pushButton_back_clicked()
{
    m_camera->cameraStop();
    wait();
    emit StopSign();


    MainWindow *s = new MainWindow();
    s->show();
    this->close();
}
