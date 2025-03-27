#include "v4l2.h"
#include <QPainter>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>


v4l2::v4l2(const QString& device, QObject* parent)
    :QThread(parent), m_device(device)
{

}

v4l2::~v4l2()
{

}

bool v4l2::OpenCamera()
{
    video_fd = open("/dev/video1", O_RDWR);
    if (video_fd < 0) {
        printf("open fail\n");
        return false;
        }

    struct v4l2_capability cap = {0};
    ioctl(video_fd, VIDIOC_QUERYCAP, &cap);
    if((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0){
        printf("V4L2_CAP_VIDEO_CAPTURE fail\n");
        return false;
    }
    if((cap.capabilities & V4L2_MEMORY_MMAP) == 0){
        printf("V4L2_MEMORY_MMAP fail\n");
        return false;
    }
    return true;
}

bool v4l2::setFormat(){
    struct v4l2_format format = {};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.height = video_height;
    format.fmt.pix.width = video_width;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;

    if(ioctl(video_fd, VIDIOC_S_FMT, &format)){
        printf("fail 1\n");
        return false;
    }
    return true;
}

bool v4l2::initBuffers()
{
    struct v4l2_requestbuffers requestbuffers{};
    requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestbuffers.count = 4;
    requestbuffers.memory = V4L2_MEMORY_MMAP;
    if(ioctl(video_fd, VIDIOC_REQBUFS, &requestbuffers)){
        printf("fail 2\n");
        return false;
    }

    for(int i = 0; i < requestbuffers.count; i++){
        struct v4l2_buffer buffer{};
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.index = i;
        buffer.memory = V4L2_MEMORY_MMAP;

        if (ioctl(video_fd, VIDIOC_QUERYBUF, &buffer) < 0) {
            printf("fail 3\n");
            return false;
            }

        m_buffers[i].length = buffer.length;
        m_buffers[i].start = static_cast<unsigned char*>(
                    mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, buffer.m.offset));

        if (m_buffers[i].start == MAP_FAILED) {
            printf("fail 4\n");
            return false;
            }

        if (ioctl(video_fd, VIDIOC_QBUF, &buffer) < 0) {
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
        printf("fail 6\n");
        return false;
    }
    m_running = true;
    return true;
}

QImage v4l2::processFrame(const BufferInfo& buffer) {
    return QImage(buffer.start, video_width, video_height, QImage::Format_RGB16);
}

void v4l2::run()
{
    OpenCamera();
    setFormat();
    initBuffers();
    startCapturing();

    m_running = true;
}

void v4l2::readFrame()
{
    if (!m_running) return;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(video_fd, &fds);
    timeval tv = {0, 10000}; // 10ms超时

    if (select(video_fd + 1, &fds, NULL, NULL, &tv) <= 0) return;

    v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(video_fd, VIDIOC_DQBUF, &buf) < 0) return;


    QImage image(m_buffers[buf.index].start, video_width, video_height, QImage::Format_RGB16);
    emit frameReady(image);

    ioctl(video_fd, VIDIOC_QBUF, &buf);
}

void v4l2::camerastop(bool status)
{
    if(status == false)
    {
        m_running = false;

        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (video_fd >= 0) {
            ioctl(video_fd, VIDIOC_STREAMOFF, &type);
            for (int i = 0; i < 4; ++i) {
                if (m_buffers[i].start) {
                    munmap(m_buffers[i].start, m_buffers[i].length);
                    m_buffers[i].start = nullptr;
                }
            }
            ::close(video_fd);
            video_fd = -1;
        }
    }
}
