#ifndef LED_H
#define LED_H

#include <QObject>
#include <QFile>

class led : public QObject
{
    Q_OBJECT
public:
    explicit led(QObject *parent = nullptr);
    ~led();

    void setLedState(bool);

private:
    QFile file;
};

#endif // LED_H
