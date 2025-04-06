#ifndef AUDIO_H
#define AUDIO_H

#include <QObject>
#include <QAudioRecorder>
#include <QAudioProbe>
#include <QAudioBuffer>

class Audio : public QObject
{
    Q_OBJECT
public:
    Audio(QObject *parent = nullptr);
    ~Audio();

    void startRecorder();
    /* 停止录音槽函数 */
    void stopRecorder();

private:
    /* 录音类 */
    QAudioRecorder *m_audioRecorder = nullptr;
    /* 用于探测缓冲区的level */
    QAudioProbe *m_probe = nullptr;

    /* 录音设置容器，保存录音设备的可用信息 */
    QList<QVariant>devicesVar;
    QList<QVariant>codecsVar;
    QList<QVariant>containersVar;
    QList<QVariant>sampleRateVar;
    QList<QVariant>channelsVar;
    QList<QVariant>qualityVar;
    QList<QVariant>bitratesVar;
};

#endif // AUDIO_H
