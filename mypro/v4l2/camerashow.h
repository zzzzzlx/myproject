#ifndef CAMERASHOW_H
#define CAMERASHOW_H

#include <QWidget>
#include "v4l2.h"
#include "mainwindow.h"
#include <QTimer>

namespace Ui {
class camerashow;
}

class camerashow : public QWidget
{
    Q_OBJECT

public:
    explicit camerashow(QWidget *parent = nullptr);
    ~camerashow();

signals:
    void frameReturn(int index);
    void StopSign();

public slots:
    void displayFrame(const QImage &frame, int index);

private slots:
    void on_pushButton_back_clicked();

private:
    Ui::camerashow *ui;
    v4l2 *m_camera;
};

#endif // CAMERASHOW_H
