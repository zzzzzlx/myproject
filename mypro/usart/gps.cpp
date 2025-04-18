#include "gps.h"

gps::gps(QObject *parent) : QObject(parent)
{

}

void gps::processData(const QByteArray &data)
{
    //qDebug()<< data<<endl;
    m_buffer.append(data);
    while (m_buffer.contains("\n")) {
        int endIndex = m_buffer.indexOf("\n");
        QByteArray line = m_buffer.left(endIndex).trimmed();
        m_buffer = m_buffer.mid(endIndex + 1);
        QString sentence = QString::fromLatin1(line);

        // 校验和验证
        if (!validateChecksum(sentence)) {
            qWarning() << "校验失败：" << sentence;
            continue;
                }
        // 分割字段
        QStringList fields = sentence.mid(1, sentence.indexOf('*')-1).split(',');

        GpsData gpsData;
        if (fields[0] == "GNGGA") {
             gpsData = parseGGA(fields);
             } else if (fields[0] == "GNRMC") {
             gpsData = parseRMC(fields);
             }
        if (gpsData.isValid) {
            handleGpsData(gpsData);
                }
    }
}

bool gps::validateChecksum(const QString &sentence) {
    int starIndex = sentence.indexOf('*');
    if (starIndex == -1) return false;

    QByteArray content = sentence.mid(1, starIndex-1).toLatin1();
    QByteArray checksum = sentence.mid(starIndex+1, 2).toLatin1();

    char calculated = 0;
    for (char c : content) {
        calculated ^= c;
    }

    return QByteArray::number(calculated, 16).toUpper() == checksum;
}

gps::GpsData gps::parseGGA(const QStringList &fields) {
    GpsData data;
    if (fields.size() < 15) return data;

    data.isValid = (fields[6].toInt() > 0);  // 定位状态
    data.time = fields[1].left(6);          // 时间
    data.latitude = nmeaToDecimal(fields[2], fields[3]);
    data.longitude = nmeaToDecimal(fields[4], fields[5]);
    data.satellites = fields[7].toInt();
    data.altitude = fields[9].toDouble();

    return data;
}

gps::GpsData gps::parseRMC(const QStringList &fields) {
    GpsData data;
    if (fields.size() < 12) return data;

    data.isValid = (fields[2] == "A");      // 状态
    data.time = fields[1].left(6);          // 时间
    data.date = fields[9];                  // 日期
    data.latitude = nmeaToDecimal(fields[3], fields[4]);
    data.longitude = nmeaToDecimal(fields[5], fields[6]);
    data.speed = fields[7].toDouble();      // 速度（节）

    return data;
}

double gps::nmeaToDecimal(const QString &coord, const QString &direction) {
    if (coord.isEmpty()) return 0.0;

    int degrees;
    double minutes;

    if (direction == "N" || direction == "S") {
        degrees = coord.left(2).toInt();
        minutes = coord.mid(2).toDouble();
    } else {
        degrees = coord.left(3).toInt();
        minutes = coord.mid(3).toDouble();
    }

    double decimal = degrees + minutes / 60.0;
    if (direction == "S" || direction == "W") {
        decimal *= -1;
    }
    return decimal;
}

void gps::handleGpsData(const GpsData &data) {
    qDebug() << "--- GPS 数据更新 ---";
    qDebug() << "有效性:" << data.isValid;
    qDebug() << "时间:" << data.time;
    qDebug() << "纬度:" << data.latitude;
    qDebug() << "经度:" << data.longitude;
    qDebug() << "海拔:" << data.altitude << "米";
    qDebug() << "速度:" << data.speed << "节";
    qDebug() << "卫星数:" << data.satellites;
}
