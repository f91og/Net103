#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>

class GateWay;
class ClientCenter;
class Device : public QObject
{
    Q_OBJECT
public:
    explicit Device(ClientCenter *parent = 0);
    void AddGateWay(GateWay* gw);

    void NetState(int index, bool conn);
    bool HasConnect();
    void OnTimer();
    void SendAppData(const QByteArray& data);
private:
    void OnTimer(int index);

    void Test(int index);
public:
    QList<GateWay*> m_lstGateWay;
    ushort m_stationAddr;
    ushort m_devAddr;
private:
    ClientCenter* m_clientCenter;
    bool m_state[2];

    ushort m_testTimer[2];
    ushort m_testTimes[2];
    ushort link_t1;
};

#endif // DEVICE_H
