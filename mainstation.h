#ifndef MAINSTATION_H
#define MAINSTATION_H

#include <QObject>
#include <QtCore/QThread>
#include <QTcpServer>
#include <QTcpSocket>

class QTimer ;
class SendUdpBrocastThread : public QThread
{
    Q_OBJECT

public :
    SendUdpBrocastThread();
    ~SendUdpBrocastThread();
protected:
    void run();
public slots:
    void slot_time();
signals:
    void sendtime(QString str);
private:
    QTimer *m_pTimer;
};

#endif // MAINSTATION_H
