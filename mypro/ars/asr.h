#ifndef ASR_H
#define ASR_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QSslConfiguration>
#include <QSslSocket>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>

#include <QHostInfo>


#include <QDebug>

#include <QFile>

class asr : public QObject
{
    Q_OBJECT
public:
    asr(QObject *parent = nullptr);
    ~asr();

    void RequestNetwork(QString url, QByteArray requestData);

    void getTheResult(QString filename);


private slots:
    void replyFinished();
    void readyReadData();


private:
    QNetworkAccessManager *networkAccessManger;
    QFile file;

    QString tokenUrl;
    /* 最终需要访问token的地址 */
    QString accessToken;

    /* 存储serverapi地址 */
    QString serverApiUrl;

    QString getJsonValue(QByteArray ba, QString key);

    const QString baiduTokeurl = "https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=%1&client_secret=%2&";
    const QString client_id = "iiZU6Q8hYRyXBnnk2KddinaP";
    const QString client_secret = "J7o8Vr9Ob5YUAcnVWNQvSKEpXvjdkFgO";

    /* 百度服务器API接口，发送语音可返回识别结果 */
    const QString server_api = "http://vop.baidu.com/server_api?dev_pid=1537&cuid=%1&token=%2";
    /*http://vop.baidu.com/server_api?dev_pid=1537&cuid=%1&token=%2*/


};

#endif // ASR_H
