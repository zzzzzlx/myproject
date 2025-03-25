#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include "v4l2.h"

v4l2::v4l2() : video_fd(-1)
{

}

v4l2::~v4l2()
{
    ::close(video_fd);
}

int v4l2::v4l2_open()
{
    video_fd = open("/dev/video1",  O_RDWR | O_NONBLOCK);
    if(video_fd < 0){
        printf("open camera fail!\n");
        return -1;
    }

    struct v4l2_capability capability;
    ioctl(video_fd, VIDIOC_QUERYCAP, &capability);
    if((V4L2_CAP_VIDEO_CAPTURE & capability.capabilities) == 0){
        perror("该摄像头设备不支持视频采集！");
        ::close(video_fd);
        return  -2;
    }
    if((V4L2_MEMORY_MMAP & capability.capabilities) == 0){
        perror("该摄像头设备不支持视频采集！");
        ::close(video_fd);
        return  -3;
    }

    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Supportformat:\r\n");
    while(ioctl(video_fd, VIDIOC_ENUM_FMT, &fmtdesc)!=-1){
        printf("/t%d.%s\r\n",fmtdesc.index+1,fmtdesc.description);
        fmtdesc.index++;

        struct v4l2_streamparm streamparm;
        streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(0 == ioctl(video_fd, VIDIOC_G_PARM, &streamparm))
        printf("该格式默认帧率 %d fps\n", streamparm.parm.capture.timeperframe.denominator);

        struct v4l2_frmsizeenum frmsizeenum;
        frmsizeenum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        frmsizeenum.pixel_format = fmtdesc.pixelformat;
        int j = 0;
        printf("支持的分辨率有：\n");
        while(1){
            frmsizeenum.index = j++;
            if(0 == ioctl(video_fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum))
                printf("%d x %d\n", frmsizeenum.discrete.width, frmsizeenum.discrete.height);
            else break;
                }
    }



    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = video_widht;
    format.fmt.pix.height = video_height;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
    if(0 > ioctl(video_fd, VIDIOC_S_FMT, &format)){
        perror("设置摄像头参数失败！");
        ::close(video_fd);
        return -4;
        }

    struct v4l2_requestbuffers requestbuffers;
    requestbuffers.count = 4;
    requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestbuffers.memory = V4L2_MEMORY_MMAP;
    if(0 == ioctl(video_fd, VIDIOC_REQBUFS, &requestbuffers)){
        for(int i = 0; i < requestbuffers.count; i++){
            struct v4l2_buffer buffer;
            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer.index = i;
            buffer.memory = V4L2_MEMORY_MMAP;
            if(0 == ioctl(video_fd, VIDIOC_QBUF, &buffer)){
                userbuff[i] = (char *)mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, buffer.m.offset);
                userbuff_length[i] = buffer.length;
            }
        }
    }else{
        perror("申请内存失败！");
        ::close(video_fd);
        return -5;
    }

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(0 > ioctl(video_fd, VIDIOC_STREAMON, &type)){
        perror("打开视频流失败！");
        return -6;
        }
    return 0;

}

int v4l2::v4l2_close()
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(0 == ioctl(video_fd, VIDIOC_STREAMOFF, &type)){
        /* 9.释放映射 */
        for(int i = 0; i < 4; i++){
            munmap(userbuff[i], userbuff_length[i]);
        }
        ::close(video_fd);
        printf("关闭相机成功!\n");
        return 0;
    }
    return -1;
}
