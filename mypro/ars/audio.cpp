#include "audio.h"
#include <QDebug>
#include <QUrl>
#include <QDateTime>
#include <QDir>
#include <QCoreApplication>

Audio::Audio(QObject *parent)
{
    /* 录制音频的类 */
    m_audioRecorder = new QAudioRecorder(this);
    /* 用于探测缓冲区的数据 */
    m_probe = new QAudioProbe(this);

    /* 设置探测的对象 */
    m_probe->setSource(m_audioRecorder);

    /* 扫描本地声卡设备 */
    devicesVar.append(QVariant(QString()));
    for (auto &device: m_audioRecorder->audioInputs()) {
        devicesVar.append(QVariant(device));
        //qDebug()<<"本地声卡设备："<<device<<endl;
    }

    /* 音频编码 */
    codecsVar.append(QVariant(QString()));
    for (auto &codecName: m_audioRecorder->supportedAudioCodecs()) {
        codecsVar.append(QVariant(codecName));
        //qDebug()<<"音频编码："<<codecName<<endl;
    }

    /* 容器/支持的格式 */
    containersVar.append(QVariant(QString()));
    for (auto &containerName: m_audioRecorder->supportedContainers()) {
        containersVar.append(QVariant(containerName));
        //qDebug()<<"支持的格式："<<containerName<<endl;
    }

    /* 采样率 */
    sampleRateVar.append(QVariant(0));
    /* 百度语音识别只支持8000、 16000采样率 */
    sampleRateVar.append(QVariant(8000));
    sampleRateVar.append(QVariant(16000));
    for (int sampleRate: m_audioRecorder->supportedAudioSampleRates()) {
        sampleRateVar.append(QVariant(sampleRate));
        //qDebug()<<"采样率："<<sampleRate<<endl;
    }


    /* 通道 */
    channelsVar.append(QVariant(-1));
    channelsVar.append(QVariant(1));
    channelsVar.append(QVariant(2));
    channelsVar.append(QVariant(4));

    /* 质量 */
    qualityVar.append(QVariant(int(QMultimedia::LowQuality)));
    qualityVar.append(QVariant(int(QMultimedia::NormalQuality)));
    qualityVar.append(QVariant(int(QMultimedia::HighQuality)));

    /* 比特率 */
    bitratesVar.append(QVariant(0));
    bitratesVar.append(QVariant(32000));
    bitratesVar.append(QVariant(64000));
    bitratesVar.append(QVariant(96000));
    bitratesVar.append(QVariant(128000));

    m_audioRecorder->setAudioInput(devicesVar.at(0).toString());

    /* 下面的是录音设置 */
    QAudioEncoderSettings settings;
    settings.setCodec(codecsVar.at(0).toString());
    settings.setSampleRate(sampleRateVar[2].toInt());
    settings.setBitRate(bitratesVar[0].toInt());
    settings.setChannelCount(channelsVar[1].toInt());
    settings.setQuality(QMultimedia::EncodingQuality(
                            qualityVar[0].toInt()));

    /* 以恒定的质量录制，可选恒定的比特率 */
    settings.setEncodingMode(QMultimedia::ConstantQualityEncoding);

    /* I.MX6ULL第20个支持的格式为 audio/x-wav */
    QString container = containersVar.at(20).toString();

    /* 使用配置 */
    m_audioRecorder->setEncodingSettings(settings,
                                         QVideoEncoderSettings(),
                                         container);
    /* 录音保存为16k.wav文件 */
    m_audioRecorder->setOutputLocation(QUrl::fromLocalFile(tr("./16k.wav")));

}


Audio::~Audio()
{

}

void Audio::startRecorder()
{
        /* 开始录音 */
    m_audioRecorder->record();
}

void Audio::stopRecorder()
{
    /* 停止录音 */
    m_audioRecorder->stop();
}
