#ifndef USART_H
#define USART_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QByteArray>
#include "gps.h"

class usart : public QObject
{
    Q_OBJECT
public:
    explicit usart(QObject *parent = nullptr);
    ~usart();

signals:
    void gpsDataUpdated(const QByteArray &data);

private slots:
    void usart_ReadData();
private:

    QSerialPort * serial_port = nullptr;

    gps *mygps = nullptr;

};

#endif // USART_H
