#include "gateway.h"
#include "clientcenter.h"
#include <QtDebug>
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
    //qDebug()<<"------"<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
    //       <<"-------";
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
                SIGNAL(Closed(int)),
                this,
                SLOT(Closed(int))
                );
        tcp->CheckConnect();
        m_lstSocket.append(tcp);
    }
}

void GateWay::PacketReceived(const NetPacket &np, int index)
{
    //qDebug()<<"Recv";
    Device* d=0;
    ushort sta ;
    ushort dev;
    np.GetDestAddr(sta,dev);
    qDebug()<<"sta"<<sta<<"dev"<<dev;
    QByteArray data = np.GetAppData();
    PNet103App::GetInstance()
            ->EmitRecvASDU(sta,dev,data);
    //下面是要改的，因为地址不一样
    if(sta != 0xff && dev!=0xffff ){
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
