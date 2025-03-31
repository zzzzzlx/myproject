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
    disconnect(this, &camerashow::StopSign, m_camera, &v4l2::cameraClean);
    delete m_camera;
    m_camera = nullptr;
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
    disconnect(this, &camerashow::frameReturn, m_camera, &v4l2::frameStatus);

    m_camera->cameraStop();
    MainWindow *s = new MainWindow();
    s->show();
    s->setAttribute(Qt::WA_DeleteOnClose); // 窗口关闭时自动删除
    emit StopSign();
    this->close();

}
