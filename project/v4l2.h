// v4l2.h
#ifndef V4L2_H
#define V4L2_H

#include <QWidget>
#include <QImage>
#include <QThread>
#include <linux/videodev2.h>

class v4l2 : public QThread{
    Q_OBJECT
public:
    explicit v4l2(const QString& device, QObject* parent = nullptr);
    ~v4l2();

    bool v4l2Init();
    void v4l2_enum_formats();
    void v4l2_print_formats();
    bool setFormat();
    bool initBuffers();
    bool startCapturing();
    void run();

    int video_height = 600;
    int video_width = 1024;


signals:
    void errorOccurred(const QString& error);
    void frameReady(const QImage& image);
private:
    struct BufferInfo {
        unsigned char* start;
        size_t length;
    };

    QString m_device;
    int video_fd = -1;
    bool m_running = true;

    BufferInfo m_buffers[4];

    QImage processFrame(const BufferInfo& buffer);
    void stop();




};

#endif // V4L2_H
