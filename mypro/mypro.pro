# 指定目标可执行文件输出目录
DESTDIR = $$PWD/

# 定义中间文件存放目录
BUILD_DIR = build

# 将各类中间文件路径指向同一目录
OBJECTS_DIR = $$BUILD_DIR
MOC_DIR = $$BUILD_DIR
RCC_DIR = $$BUILD_DIR
UI_DIR = $$BUILD_DIR

# 自动创建目录
!exists($$BUILD_DIR): mkdir($$BUILD_DIR)
!exists($$DESTDIR): mkdir($$DESTDIR)

QT       += core gui


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia network serialport

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ars/asr.cpp \
    ars/audio.cpp \
    gpio/beep.cpp \
    gpio/led.cpp \
    main.cpp \
    mainwindow.cpp \
    usart/gps.cpp \
    usart/usart.cpp \
    v4l2/camerashow.cpp \
    v4l2/v4l2.cpp

HEADERS += \
    ars/asr.h \
    ars/audio.h \
    gpio/beep.h \
    gpio/led.h \
    mainwindow.h \
    usart/gps.h \
    usart/usart.h \
    v4l2/camerashow.h \
    v4l2/v4l2.h

FORMS += \
    mainwindow.ui \
    v4l2/camerashow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
