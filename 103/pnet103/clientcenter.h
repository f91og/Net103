#ifndef CLIENTCENTER_H
#define CLIENTCENTER_H

#include <QObject>
#include <QTcpServer>
#include "gateway.h"
#include "device.h"

const int DEVICE_TEST_TIME_T2 = 10;
const int DEVICE_TEST_TIMES = 4;

const int IPACKET_ACK_T2=10;
const int IPACKET_WAIT_ACK_T1=15;

const int IDEL_WAIT_T3=20;
const int START_WAIT_T1=15;

class ClientCenter : public QObject
{
    Q_OBJECT
public:
    explicit ClientCenter(QObject *parent = 0);
    ushort GetRemotePort();
    void SetDeviceList(const QVariantList& list);

    Device* GetDevice(ushort sta,ushort dev);

    void SendAppData(ushort sta,ushort dev, const QByteArray& data);

    QTcpServer* GetTcpServer();

    QList<QTcpSocket*> GetSocketList();

protected:
    void timerEvent(QTimerEvent *);
    void Clear();

private slots:
    server_New_Connect();
private:
    QTcpServer* server;
    ushort m_remotePort;
    QList<GateWay*> m_lstGateWay;
    QList<Device*> m_lstDevice;
    QMap<quint32,Device*> m_mapDevice;
    QList<QTcpSocket*> m_socketList;
};

#endif // CLIENTCENTER_H
