#ifndef GATEWAY_H
#define GATEWAY_H

#include <QObject>
#include "tcpsocket.h"
#include <QStringList>
#include "netpacket.h"

class ClientCenter;

class Device;

const ushort MAX_PACKET_NUMBER = 16383;
const ushort MAX_PACKET_NUMBER_LONG = 32767;

const ushort MAX_PENDING_NUMBER= 100;

class GateWay : public QObject
{
    Q_OBJECT
public:
    explicit GateWay(ClientCenter *parent = 0);
    void Init(ushort sta,ushort dev,const QStringList& ips);

    ClientCenter* GetCenter();

    bool HasConnect();
    int ConnectNetCount();

    ushort GetStationAddr();
    ushort GetDevAddr();

    void SendPacket(const NetPacket& np, int index=-1);
    void SendAppData(const NetPacket& np);
    void OnTime();

    bool LessThan(ushort num1,ushort num2);
    void IncNumber(ushort& num);

    QString GetAddrString();

    ushort GetMaxNumber();
private:
    void SendIPacket(const NetPacket &np);
    void InitNumber();
    void AckRecv(ushort num);
    bool IsNumberValid(ushort num);

    bool CanSend(ushort num);

    void CheckSend();

    void SendAck();

    bool AcceptRecv(ushort num);

    void CheckAckRecv();
private slots:
    void PacketReceived(const NetPacket& np,int index);
    void Closed(int index);
private:
    QStringList m_lstIp;
    ushort m_staionAddr;
    ushort m_devAddr;
    QList<TcpSocket*> m_lstSocket;
    ushort m_remotePort;
    ClientCenter* m_clientCenter;
    QList<Device*> m_lstDevice;

    ushort m_recvNumber;
    ushort m_sendNumber;
    bool m_clearNumber;
    ushort m_ackNumber;
    ushort m_ackSendNumber;
    QDateTime m_sendAckTime;
    QDateTime m_recvAckTime;
    QList<NetPacket> m_lstIPacketSend;

    int m_packetNotAck;
};

#endif // GATEWAY_H
