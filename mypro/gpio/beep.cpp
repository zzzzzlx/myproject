#include "beep.h"
#include <QDebug>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

beep::beep(QObject *parent) : QObject(parent)
{
    this->setParent(parent);
    file.setFileName("/sys/class/leds/beep/brightness");
}

beep::~beep()
{

}
void beep::setLedState(bool flag)
{
    /* 如果文件不存在，则返回 */
    if (!file.exists())
        return;

    if(!file.open(QIODevice::ReadWrite))
        qDebug()<<file.errorString();

    QByteArray buf[2] = {"0", "1"};

    /* 写0或1,1~255都可以点亮LED */
    if (flag)
        file.write(buf[1]);
    else
        file.write(buf[0]);

    /* 关闭文件 */
    file.close();
}
