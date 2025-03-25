// v4l2.h
#ifndef V4L2_H
#define V4L2_H

#include <string>

class v4l2 {
public:
    explicit v4l2();
    ~v4l2();

    int v4l2_open();
    int v4l2_close();

private:
    int video_widht = 600;
    int video_height = 1024;

    char *userbuff[4];
    char userbuff_length[4];

    int video_fd;
};

#endif // V4L2_H
