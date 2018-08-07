#include "gateway.h"
#include "clientcenter.h"
#include <QtDebug>
#include <QUdpSocket>
#include "device.h"
#include "comm/pnet103app.h"
#include "comm/promonitorapp.h"

GateWay::GateWay(ClientCenter *parent) :
    QObject(parent)
{
    m_clientCenter = parent;
}


ClientCenter* GateWay::GetCenter()
{
    return m_clientCenter;
}

bool GateWay::HasConnect()
{
//    foreach (TcpSocket* tcp, m_lstSocket) {
//        if(tcp->socket->state() == QTcpSocket::ConnectedState){
//            return true;
//        }
//    }
//    return false;
}

ushort GateWay::GetStationAddr()
{
    return m_staionAddr;
}

ushort GateWay::GetDevAddr()
{
    return m_devAddr;
}

void GateWay::SendPacket(const NetPacket &np, int index)
{
    foreach (TcpSocket* tcp, m_lstSocket) {
        if(index==-1 || tcp->GetIndex()==index){
            tcp->SendPacket(np);
        }
    }
}

QString GateWay::GetAddrString()
{
    return QString("s%1d%2")
            .arg(m_staionAddr)
            .arg(m_devAddr);
}


void GateWay::OnTime()
{
    foreach (TcpSocket* tcp, m_lstSocket) {
        tcp->OnTimer();
    }
}

void GateWay::SendAppData(const NetPacket& np)
{
    SendPacket(np);
}

void GateWay::SendIPacket(const NetPacket& np)
{
    SendPacket(np);
}

void GateWay::Init(ushort sta,ushort dev,const QStringList& ips)
{
    qDebug()<<"gateway init...";
    m_staionAddr = sta;
    m_devAddr = dev;
    qDebug()<<"gateway dev"<<dev;
    m_lstIp = ips;
    int index=0;
    foreach (QString ip, m_lstIp) {
        TcpSocket* tcp = new TcpSocket(this);
        tcp->Init(ip,m_clientCenter->GetRemotePort(),
                  index++);
        connect(tcp,
                SIGNAL(PacketReceived(NetPacket,int)),
                this,
                SLOT(PacketReceived(NetPacket,int))
                );
        connect(tcp,
                SIGNAL(Closed(int)),
                this,
                SLOT(Closed(int))
                );
        m_lstSocket.append(tcp);
    }
    foreach(TcpSocket* tcp,m_lstSocket)
    {
        SendUdp(tcp->GetRemoteIP());
    }
}

void GateWay::PacketReceived(const NetPacket &np, int index)
{
    Device* d=0;
    ushort sta ;
    ushort dev;
    np.GetDestAddr(sta,dev);
    qDebug()<<"sta"<<sta<<"dev"<<dev;
    QByteArray data = np.GetAppData();
    PNet103App::GetInstance()
            ->EmitRecvASDU(sta,dev,data);

    if(sta != 0xff && dev!=0xffff ){
        qDebug()<<"sta"<<sta<<"dev"<<dev<<"PacketReceived";
        d = m_clientCenter->GetDevice(sta,dev);
        d->AddGateWay(this);
        d->NetState(index,true);
    }
}

void GateWay::Closed(int index)
{
    Q_UNUSED(index);
    if(!HasConnect()){
  //      InitNumber();
    }
}

void GateWay::SendUdp(QString ip)
{
    QDateTime now = QDateTime::currentDateTime();
    //发点对点广播
    QUdpSocket *m_pUdpSocket = new QUdpSocket();
    uchar con_packet[41];
    memset(con_packet, 0x00, sizeof(con_packet));
    con_packet[0]=0xFF;
    con_packet[1]=1;
    struct TIME_S
    {
        ushort	ms;
        uchar	min;
        uchar	hour;
        uchar	day;
        uchar	month;
        uchar	year;
    }Time;
    Time.year	= (uchar)now.date().year()-2000;
    Time.month	= (uchar)now.date().month();
    Time.day	= (uchar)now.date().day();
    Time.hour	= (uchar)now.time().hour();
    Time.min	= (uchar)now.time().minute();
    Time.ms		= now.time().second()*1000+now.time().msec();
    memcpy(&con_packet[2], &Time, 7);
    QHostAddress ha;
    ha.setAddress(ip);
    m_pUdpSocket->writeDatagram((char*)con_packet,41,ha,1032);
}

void GateWay::SyncSocket(QTcpSocket* socket)
{
    foreach(TcpSocket* s,m_lstSocket)
    {
        s->CheckConnect(socket);
    }
}
