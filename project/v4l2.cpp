#include "v4l2.h"
#include <QPainter>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

typedef struct camera_format {
    unsigned char description[32];  //字符串描述信息
    unsigned int pixelformat;       //像素格式
} cam_fmt;
static cam_fmt cam_fmts[10];

v4l2::v4l2(const QString& device, QObject* parent)
    :QThread(parent), m_device(device){}

v4l2::~v4l2()
{
    m_running = false;
    wait(); // 等待线程结束
}



bool v4l2::v4l2Init()
{
    video_fd = open(m_device.toLocal8Bit().constData(), O_RDWR);
    if (video_fd < 0) {
        emit errorOccurred("Open device failed");
        printf("open fail\n");
        return false;
        }

    struct v4l2_capability cap = {0};
    ioctl(video_fd, VIDIOC_QUERYCAP, &cap);
    if((V4L2_CAP_VIDEO_CAPTURE & cap.capabilities) == 0){
        printf("cap not support V4L2_CAP_VIDEO_CAPTURE");
        ::close(video_fd);
        return false;
    }
    if((V4L2_MEMORY_MMAP & cap.capabilities) == 0){
        printf("cap not support V4L2_MEMORY_MMAP");
        ::close(video_fd);
        return false;
    }
    printf("v4l2Init \n");
}

void v4l2::v4l2_enum_formats()
{
    struct v4l2_fmtdesc fmtdesc = {0};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index = 0;
    while (0 ==ioctl(video_fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
        cam_fmts[fmtdesc.index].pixelformat = fmtdesc.pixelformat;
        memcpy(cam_fmts[fmtdesc.index].description, fmtdesc.description, sizeof(fmtdesc.description));
        fmtdesc.index++;
    }
}

void v4l2::v4l2_print_formats()
{
     struct v4l2_frmsizeenum frmsize = {0};
     struct v4l2_frmivalenum frmival = {0};

     frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
     frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for (int i = 0; cam_fmts[i].pixelformat; i++){
        printf("format<0x%x>, description<%s>\n", cam_fmts[i].pixelformat,
                            cam_fmts[i].description);

        frmsize.index = 0;
        frmsize.pixel_format = cam_fmts[i].pixelformat;
        frmival.pixel_format = cam_fmts[i].pixelformat;
        while (0 == ioctl(video_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize)){
            printf("size<%d*%d> ",
                frmsize.discrete.width,
                frmsize.discrete.height);
            frmsize.index++;

            frmival.index = 0;
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;
            while (0 == ioctl(video_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival)){
                printf("<%dfps>", frmival.discrete.denominator /
                                        frmival.discrete.numerator);
                frmival.index++;
            }
            printf("\n");
        }
        printf("\n");
    }
}

bool v4l2::setFormat(){
    struct v4l2_format format = {0};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.height = video_height;
    format.fmt.pix.width = video_width;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;

    if(ioctl(video_fd, VIDIOC_S_FMT, &format)){
        emit errorOccurred("Set format failed");
        printf("fail 1\n");
        return false;
    }
}

bool v4l2::initBuffers()
{
    struct v4l2_requestbuffers requestbuffers;
    requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestbuffers.count = 4;
    requestbuffers.memory = V4L2_MEMORY_MMAP;
    if(ioctl(video_fd, VIDIOC_REQBUFS, &requestbuffers)){
        emit errorOccurred("requestbuffers failed");
        printf("fail 2\n");
        return false;
    }

    for(int i = 0; i < requestbuffers.count; i++){
        struct v4l2_buffer buffer;
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.index = i;
        buffer.memory = V4L2_MEMORY_MMAP;

        if (ioctl(video_fd, VIDIOC_QUERYBUF, &buffer) < 0) {
            emit errorOccurred("Query buffer failed");
            printf("fail 3\n");
            return false;
            }

        m_buffers[i].length = buffer.length;
        m_buffers[i].start = static_cast<unsigned char*>(
                    mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, buffer.m.offset));

        if (m_buffers[i].start == MAP_FAILED) {
            emit errorOccurred("Memory map failed");
            printf("fail 4\n");
            return false;
            }

        if (ioctl(video_fd, VIDIOC_QBUF, &buffer) < 0) {
            emit errorOccurred("Queue buffer failed");
            printf("fail 5\n");
            return false;
            }
    }
    printf("initBuffers \n");
    return true;
}

bool v4l2::startCapturing()
{
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(video_fd, VIDIOC_STREAMON, &type)){
        emit errorOccurred("Start stream failed");
        printf("fail 6\n");
        return false;
    }
    m_running = true;
    return true;
}

void v4l2::run()
{
    while(m_running){
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(video_fd, &fds);

        timeval tv{};
        tv.tv_sec = 2;

        if (QThread::currentThread()->isInterruptionRequested()) {
            m_running = false;
            break;
        }

        int ret = select(video_fd + 1, &fds, NULL, NULL, &tv);
        if (ret < 0) {
        emit errorOccurred("Select error");
        break;
        }

        if(ret == 0)
            continue;

        struct v4l2_buffer buf{};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(video_fd, VIDIOC_DQBUF, &buf) < 0) {
            emit errorOccurred("Dequeue buffer failed");
            continue;
            }

        QImage image = processFrame(m_buffers[buf.index]);
        emit frameReady(image);

        if (ioctl(video_fd, VIDIOC_QBUF, &buf) < 0) {
            emit errorOccurred("Requeue buffer failed");
        }
    }

    // Cleanup
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(video_fd, VIDIOC_STREAMOFF, &type);
        for (int i = 0; i < 4; ++i) {
            munmap(m_buffers[i].start, m_buffers[i].length);
        }
        close(video_fd);
}

void v4l2::stop()
{
    m_running = false;
    wait(); // 等待线程结束
}


QImage v4l2::processFrame(const BufferInfo& buffer) {
    return QImage(buffer.start, video_width, video_height, QImage::Format_RGB16);
}
