#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_camera = new v4l2("/dev/video1", this);
    connect(m_camera, &v4l2::frameReady, this, &MainWindow::displayFrame);

    m_camera->v4l2Init();
    m_camera->setFormat();
    m_camera->initBuffers();
    m_camera->startCapturing();
    m_camera->start(); // 启动线程
}

MainWindow::~MainWindow()
{
    delete ui;
    m_camera->requestInterruption(); // 请求停止线程
    m_camera->wait();
    delete m_camera;
}


void MainWindow::displayFrame(const QImage &frame)
{
    // 将QImage转换为QPixmap并显示在QLabel上
    QPixmap pixmap = QPixmap::fromImage(frame);
    ui->videoLabel->setPixmap(pixmap.scaled(ui->videoLabel->size(), Qt::KeepAspectRatio));
}
