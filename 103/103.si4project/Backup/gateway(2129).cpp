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
    InitNumber();
}

void GateWay::InitNumber()
{
    m_clearNumber=true;
    m_sendNumber=0;
    m_recvNumber=0;
    m_ackNumber=0;
    m_ackSendNumber=0;
    m_lstIPacketSend.clear();

    m_packetNotAck=0;
}

ClientCenter* GateWay::GetCenter()
{
    return m_clientCenter;
}

bool GateWay::HasConnect()
{
    foreach (TcpSocket* tcp, m_lstSocket) {
        if(tcp->state() == TcpSocket::ConnectedState){
            return true;
        }
    }
    return false;
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

bool GateWay::LessThan(ushort num1, ushort num2)
{
    if(num1<num2){
        return (num2-num1)<=GetMaxNumber()/2;
    }
    if(num1>num2){
        return (num1-num2)>=GetMaxNumber()/2;
    }
    return false;
}

ushort GateWay::GetMaxNumber()
{
    if(m_clientCenter->IsLongNumber()){
        //qDebug()<<"long";
        return MAX_PACKET_NUMBER_LONG;
    }
    else {
        return MAX_PACKET_NUMBER;
    }
}

QString GateWay::GetAddrString()
{
    return QString("s%1d%2")
            .arg(m_staionAddr)
            .arg(m_devAddr);
}

void GateWay::IncNumber(ushort &num)
{
    if(num == GetMaxNumber()){
        num=0;
    }
    else{
        num++;
    }
}

void GateWay::OnTime()
{
    //qDebug()<<"------"<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
    //       <<"-------";
    foreach (TcpSocket* tcp, m_lstSocket) {
        tcp->OnTimer();
    }
    SendAck();
    CheckAckRecv();
}

void GateWay::SendAppData(const NetPacket& np)
{
    ushort sendNumber = m_sendNumber;
    if(CanSend(sendNumber)){
        SendIPacket(np);
    }
    else{
        m_lstIPacketSend.append(np);
    }
}

bool GateWay::CanSend(ushort num)
{
    if(num>=m_ackNumber){
        return (num-m_ackNumber) <= GetMaxNumber();
    }
    return (m_ackNumber-num) >= GetMaxNumber()-MAX_PENDING_NUMBER;
}

void GateWay::CheckAckRecv()
{
    //qDebug()<<"发送编号:"<<m_sendNumber;
    //qDebug()<<"应答编号:"<<m_ackNumber;
    if(m_sendNumber == m_ackNumber){
        return;
    }
    QDateTime now = QDateTime::currentDateTime();
    if(m_recvAckTime.secsTo(now)>IPACKET_WAIT_ACK_T1){
        ProMonitorApp::GetInstance()
                ->AddMonitor(ProNetLink103,
                             MsgError,
                             GetAddrString(),
                             QString("应答超时,发送编号:%1,应答编号:%2,发送缓存清除。")
                             .arg(m_sendNumber)
                             .arg(m_ackNumber)
                             );
        m_lstIPacketSend.clear();
    }
}

bool GateWay::AcceptRecv(ushort num)
{
    if(num>=m_recvNumber){
        return num-m_recvNumber<=1000;
    }
    return m_recvNumber-num>= GetMaxNumber()-1000;
}

void GateWay::SendIPacket(const NetPacket& np)
{
    NetPacket nps=np;
    nps.SetNumber(1,m_recvNumber);
    nps.SetNumber(0,m_sendNumber);
    //qDebug()<<"报文发送编号:"<<m_sendNumber;
    m_ackSendNumber = m_recvNumber;
    m_sendAckTime = QDateTime();
    IncNumber(m_sendNumber);
    SendPacket(nps);
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
                //Qt::QueuedConnection
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

void GateWay::SendAck()
{
    if(m_recvNumber==m_ackSendNumber){
        return;
    }
    QDateTime now  = QDateTime::currentDateTime();
    if(m_packetNotAck<10){
        if(!m_sendAckTime.isNull() && m_sendAckTime.secsTo(now) < IPACKET_ACK_T2 ){
            return;
        }
    }
    NetPacket np;
    np.SetType(NetPacket::Packet_S);
    np.SetSourceAddr(0,m_clientCenter->GetLocalAddr());
    np.SetDestAddr(m_staionAddr,m_devAddr);
    np.SetNumber(1,m_recvNumber);
    SendPacket(np);
    m_ackSendNumber=m_recvNumber;
    m_sendAckTime = QDateTime();
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProNetLink103,
                         MsgInfo,
                         GetAddrString(),
                         QString("发送S应答，编号：%1")
                         .arg(m_recvNumber)
                         );
    m_packetNotAck=0;
    //qDebug()<<QString("发送S应答，编号：%1")
    //          .arg(m_recvNumber);
}

void GateWay::CheckSend()
{
    while(!m_lstIPacketSend.isEmpty()){
        ushort sendNumber = m_sendNumber;
        if(CanSend(sendNumber)){
            SendIPacket(m_lstIPacketSend.at(0));
            m_lstIPacketSend.pop_front();
        }
        else{
            break;
        }
    }
}

void GateWay::AckRecv(ushort num)
{
    if(LessThan(m_ackNumber,num)){
        m_recvAckTime = QDateTime::currentDateTime();
        m_ackNumber = num;
        CheckSend();
    }
    if(m_ackNumber==m_sendNumber){
        m_recvAckTime = QDateTime::currentDateTime();
    }
}

void GateWay::Closed(int index)
{
    Q_UNUSED(index);
    if(!HasConnect()){
        InitNumber();
    }
}

void GateWay::PacketReceived(const NetPacket &np, int index)
{
    //qDebug()<<"Recv";
    Device* d=0;
    ushort sta ;
    ushort dev;
    np.GetSourceAddr(sta,dev);

    NetPacket::Type tp = np.GetType();
    switch (tp) {
    case NetPacket::Packet_U:
    {
        //qDebug()<<"U";
        if(np.GetUType()==NetPacket::U_Start_Ack){
            if(m_clearNumber){
               m_clearNumber=false;
               m_sendNumber =0;
               m_recvNumber =np.GetNumber(1);
               ProMonitorApp::GetInstance()
                       ->AddMonitor(ProNetLink103,
                                    MsgInfo,
                                    GetAddrString(),
                                    QString("重置发送编号0，接收编号：%1")
                                    .arg(m_recvNumber)
                                    );
            }
        }
    }
        break;
    case NetPacket::Packet_S:
    {
        //qDebug()<<"S";
        ushort num = np.GetNumber(1);
        AckRecv(num);
    }
        break;
    case NetPacket::Packet_I:
    {
        ushort  rnum = np.GetNumber(1);
        AckRecv(rnum);
        ushort snum = np.GetNumber(0);
        //qDebug()<<"收到I格式报文,接收编号:"<<snum;

        ProMonitorApp::GetInstance()
                ->AddMonitor(ProNetLink103,
                             MsgInfo,
                             GetAddrString(),
                             QString("收到I格式报文,发送编号:%1，接收编号:%2")
                             .arg(snum).arg(rnum)
                             );
        if(AcceptRecv(snum)){
            // qDebug()<<"接收报文。";
            IncNumber(snum);
            m_packetNotAck++;
            m_recvNumber=snum;
            if(m_sendAckTime.isNull()){
                m_sendAckTime = QDateTime::currentDateTime();
            }
            QByteArray data = np.GetAppData();
            PNet103App::GetInstance()
                    ->EmitRecvASDU(sta,dev,data);
            SendAck();
        }
        else{
            ProMonitorApp::GetInstance()
                    ->AddMonitor(ProNetLink103,
                                 MsgInfo,
                                 GetAddrString(),
                                 QString("编号已经接收过，丢弃报文。")
                                 );
        }
    }
        break;
    default:
        break;
    }
    if(sta != 0xff && dev!=0xffff ){
        d = m_clientCenter->GetDevice(sta,dev);
        d->AddGateWay(this);
        d->NetState(index,true);
    }
}
