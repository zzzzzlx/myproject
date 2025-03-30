// v4l2.h
#ifndef V4L2_H
#define V4L2_H

#include <QWidget>
#include <QImage>
#include <QThread>
#include <linux/videodev2.h>
#include <QMutex>

class v4l2 : public QThread{
    Q_OBJECT
public:
    explicit v4l2(const QString& device, QObject* parent = nullptr);
    ~v4l2();

    void cameraStop();

    int video_height = 600;
    int video_width = 1024;


signals:
    void frameReady(const QImage& image,int index);

public slots:
    void frameStatus(int index);
    void cameraClean();


private:
    struct BufferInfo {
        unsigned char* start;
        size_t length;
        bool in_use;
    };


    QString m_device;
    int video_fd = -1;

    QMutex m_mutex;
    bool m_running = true;

    BufferInfo m_buffers[4];

    bool OpenCamera();
    bool setFormat();
    bool initBuffers();
    bool startCapturing();

    void readFrame();
    void run();


    QImage processFrame(const BufferInfo& buffer);

};

#endif // V4L2_H
