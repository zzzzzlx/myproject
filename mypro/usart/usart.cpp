#include "usart.h"

usart::usart(QObject *parent) : QObject(parent), serial_port(nullptr)
{
    serial_port = new QSerialPort();
    mygps = new gps(this);

    serial_port->setPortName("/dev/ttymxc2");

    serial_port->setBaudRate(QSerialPort::Baud38400);//设置波特率为38400
    serial_port->setDataBits(QSerialPort::Data8);//设置数据位8
    serial_port->setParity(QSerialPort::NoParity); //校验位设置为0
    serial_port->setStopBits(QSerialPort::OneStop);//停止位设置为1
    serial_port->setFlowControl(QSerialPort::NoFlowControl);//设置为无流控制

    if (!serial_port->open(QIODevice::ReadWrite)) {
            qDebug() << "Error opening serial port:" << serial_port->errorString();
            delete serial_port; // 避免内存泄漏
            serial_port = nullptr; // 标记为无效
            return;
        }

    QObject::connect(serial_port,&QSerialPort::readyRead,this,&usart::usart_ReadData);
    QObject::connect(this, &usart::gpsDataUpdated, mygps,&gps::processData);
}

usart::~usart()
{
    if (serial_port && serial_port->isOpen()) {
            serial_port->close();
        }
    delete serial_port;
    serial_port = nullptr;
}

void usart::usart_ReadData()
{
    QByteArray buf;
    buf = serial_port->readAll();
    if(!buf.isEmpty())
    {
        /*qDebug()<< buf << endl;*/
        emit gpsDataUpdated(buf);
    }
    buf.clear();
}
