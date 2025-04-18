#include "led.h"
#include <QDebug>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


led::led(QObject *parent) : QObject(parent)
{
    this->setParent(parent);
    file.setFileName("/sys/class/leds/led/brightness");
}

led::~led()
{

}

void led::setLedState(bool flag)
{
    /* 如果文件不存在，则返回 */
    if (!file.exists())
        return;

    if(!file.open(QIODevice::ReadWrite))
        qDebug()<<file.errorString();

    QByteArray buf[2] = {"0", "1"};

    if (flag)
        file.write(buf[1]);
    else
        file.write(buf[0]);

    /* 关闭文件 */
    file.close();
}
