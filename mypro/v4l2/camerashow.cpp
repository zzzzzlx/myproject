#include "camerashow.h"
#include "ui_camerashow.h"

camerashow::camerashow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::camerashow)
{
    ui->setupUi(this);

    m_camera = new v4l2("/dev/video1", this);

    connect(m_camera, &v4l2::frameReady, this, &camerashow::displayFrame);
    m_camera->start();

}

camerashow::~camerashow()
{
    m_camera->cameraStop();
    m_camera->wait();
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
    m_camera->cameraStop();

    MainWindow *s = new MainWindow();
    s->show();
    this->close();
}
