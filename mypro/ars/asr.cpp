#include "asr.h"
#include <QCoreApplication>


asr::asr(QObject *parent) : QObject(parent)
{
    networkAccessManger = new QNetworkAccessManager(this);

    QString fileName = QCoreApplication::applicationDirPath() + "/16k.wav";
    file.setFileName(fileName);

    tokenUrl = QString(baiduTokeurl).arg(client_id).arg(client_secret);

    QByteArray requestData;
    requestData.clear();

    RequestNetwork(tokenUrl, requestData);

}

asr::~asr()
{

}

void asr::RequestNetwork(QString url, QByteArray requestData)
{
    QNetworkRequest networkRequest;

    QSslConfiguration config;
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    config.setProtocol(QSsl::TlsV1SslV3);
    networkRequest.setSslConfiguration(config);

    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json;charset=UTF-8");

    networkRequest.setRawHeader("Content-Type", QString("audio/pcm;rate=16000").toLatin1());
    networkRequest.setUrl(url);

    QNetworkReply *newReply = networkAccessManger->post(networkRequest, requestData);

    connect(newReply, SIGNAL(finished()), this, SLOT(replyFinished()));
    connect(newReply, SIGNAL(readyRead()), this, SLOT(readyReadData()));
}

void asr::replyFinished()
{
    QNetworkReply *reply = (QNetworkReply *)sender();
    reply->deleteLater();
    reply = nullptr;
}

void asr::readyReadData()
{
    QNetworkReply *reply = (QNetworkReply *)sender();
    QByteArray data = reply->readAll();

    if(reply->url() == QUrl(tokenUrl) ){
        qDebug()<< QString(data) << endl;
        QString key = "access_token";

        QString temp = getJsonValue(data, key);
        accessToken = temp;
        qDebug()<<accessToken<<endl;
    }

    if(reply->url() == QUrl(serverApiUrl)){
        qDebug()<< QString(data) << endl;
        QString key = "result";

        QString temp = getJsonValue(data, key);


        qDebug()<<temp<<endl;
    }
}

QString asr::getJsonValue(QByteArray ba, QString key)
{
    QJsonParseError JsonParseError;
    QJsonDocument jsonDocunment = QJsonDocument::fromJson(ba, &JsonParseError);

    if(JsonParseError.error == QJsonParseError::NoError)
    {
        if(jsonDocunment.isObject())
        {
            QJsonObject JsonObj = jsonDocunment.object();
            if(JsonObj.contains(key))
            {
                QJsonValue JsonValue = JsonObj.value(key);
                if(JsonValue.isString())
                {
                    return JsonValue.toString();
                }else if(JsonValue.isArray()){
                    QJsonArray arr = JsonValue.toArray();
                    QJsonValue jv = arr.at(0);
                    return jv.toString();
                }
            }
        }
    }
    return "";
}

void asr::getTheResult(QString fileName)
{
    file.setFileName(fileName);
    if (!file.exists()) {
        qDebug()<<"已返回，文件"<<fileName<<"不存在"<<endl;
        return;
    }

    QByteArray requestData;
    file.open(QIODevice::ReadOnly);
    requestData = file.readAll();
    file.close();

    serverApiUrl = QString(server_api).arg(QHostInfo::localHostName()).arg(accessToken);
    qDebug()<<"serverApiUrl:"<<QString(serverApiUrl) << endl;

    RequestNetwork(serverApiUrl, requestData);
}



