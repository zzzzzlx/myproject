#ifndef BEEP_H
#define BEEP_H

#include <QObject>
#include <QFile>

class beep : public QObject
{
    Q_OBJECT
public:
    explicit beep(QObject *parent = nullptr);
    ~beep();

    void setLedState(bool);

private:
    QFile file;
};

#endif // BEEP_H
