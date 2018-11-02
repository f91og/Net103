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
    m_lastConnectIndex=0;
}


ClientCenter* GateWay::GetCenter()
{
    return m_clientCenter;
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
    m_staionAddr = sta;
    m_devAddr = dev;
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
                SIGNAL(Closed(int,QString,uchar)),
                this,
                SLOT(Closed(int,QString,uchar))
                );
        m_lstSocket.append(tcp);
    }
//    foreach(TcpSocket* tcp,m_lstSocket)
//    {
//        SendUdp(tcp->GetRemoteIP(),tcp->GetIndex(),false);
//    }
}

void GateWay::PacketReceived(const NetPacket &np, int index)
{
    Device* d=0;
    ushort sta ;
    ushort dev;

    np.GetDestAddr(sta,dev);
    QByteArray data = np.GetAppData();
    QByteArray send=np.GetAppData();
    if((uchar)send[8]==0x05 && ((uchar)send[9]==0xc8 || (uchar)send[9]==0xc9)){
        PNet103App::GetInstance()
                ->EmitRecvASDU(sta,dev,data);
        return;
    }
    if(sta != 0xff && dev!=0xffff ){
        d = m_clientCenter->GetDevice(sta,dev);
        d->AddGateWay(this);
        d->NetState(index,true);
    }

    if(m_lstSocket.at(0)->IsConnected()){
        m_lastConnectIndex=0;
    }else{
        m_lastConnectIndex=1;
    }
    if(index!=m_lastConnectIndex)
        return;
    PNet103App::GetInstance()
            ->EmitRecvASDU(sta,dev,data);

}

void GateWay::Closed(int index, QString ip, uchar cpu_no)
{
//    QString ip_last=ip.section(".",3,3);
//    ushort dev_addr=ip_last.toUShort()*0x100+cpu_no;
//    Device* d=0;//
//    d = m_clientCenter->GetDevice(0,dev_addr);
//    d->NetState(index,false);
}

void GateWay::SendUdp(QString ip, int index, bool isConnected)
{

    QDateTime now = QDateTime::currentDateTime();
    //发点对点广播
    QUdpSocket *m_pUdpSocket = new QUdpSocket();
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
    QHostAddress ha;
    ha.setAddress(ip);
    // 没有连接则发送点对点握手，有连接则发送udp心跳
    int port=(index==0? 1032:1033);
    if(isConnected){
        QByteArray heart_udp(9,0);
        heart_udp[0]=0x88;
        memcpy(heart_udp.data()+2, &Time, 7);
        m_pUdpSocket->writeDatagram(heart_udp.data(),9,ha,port);

        //对时，建立连接后握手报文和心跳报文都要发
        QByteArray con_packet(41,0);
        con_packet[0]=0xFF;
        con_packet[1]=1;
        memcpy(con_packet.data()+2, &Time, 7);
        QByteArray sca_type="NPS-SCS:NDJ300-V1.0";  // 将String赋给字节数组
        memcpy(con_packet.data()+9, sca_type.data(), sca_type.length());
        m_pUdpSocket->writeDatagram(con_packet.data(),41,ha,port);
    }else{
        QByteArray con_packet(41,0);
        con_packet[0]=0xFF;
        con_packet[1]=1;
        memcpy(con_packet.data()+2, &Time, 7);
        QByteArray sca_type="NPS-SCS:NDJ300-V1.0";  // 将String赋给字节数组
        memcpy(con_packet.data()+9, sca_type.data(), sca_type.length());
        m_pUdpSocket->writeDatagram(con_packet.data(),41,ha,port);
    }
    delete m_pUdpSocket;
}

void GateWay::SyncSocket(QTcpSocket* socket)
{
    foreach(TcpSocket* s,m_lstSocket)
    {
        s->CheckConnect(socket);
    }
}
