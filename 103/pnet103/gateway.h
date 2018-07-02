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

    QString GetAddrString();
    void SendUdp(QString ip);
private:
    void SendIPacket(const NetPacket &np);
    void InitNumber();
    void AckRecv(ushort num);
    bool IsNumberValid(ushort num);
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
    QList<NetPacket> m_lstIPacketSend;
};

#endif // GATEWAY_H
