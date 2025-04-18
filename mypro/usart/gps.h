#ifndef GPS_H
#define GPS_H

#include <QObject>
#include <QDebug>

class gps : public QObject
{
    Q_OBJECT
public:
    explicit gps(QObject *parent = nullptr);
    struct GpsData {
        bool isValid = false;
        double latitude = 0.0;
        double longitude = 0.0;
        QString time;          // HHMMSS
        QString date;          // DDMMYY
        int satellites = 0;
        double speed = 0.0;   // 节
        double altitude = 0.0;
        };
signals:

public slots:
    void processData(const QByteArray &data);

private:
    QByteArray m_buffer;  // 数据缓冲区

    bool validateChecksum(const QString &sentence);
    gps::GpsData parseGGA(const QStringList &fields);
    gps::GpsData parseRMC(const QStringList &fields);
    double nmeaToDecimal(const QString &coord, const QString &direction);
    void handleGpsData(const GpsData &data);

};

#endif // GPS_H
