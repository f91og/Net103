#include "tcpsocket.h"
#include <QHostAddress>
#include <QUdpSocket>
#include "gateway.h"
#include "netpacket.h"
#include "clientcenter.h"
#include <QtDebug>
#include "tool/toolapi.h"
#include "comm/promonitorapp.h"

TcpSocket::TcpSocket(GateWay *parent)
    :QTcpSocket(parent)
{
    m_appState=AppNotStart;
    m_gateWay = parent;
    m_waitRecv=false;
    m_waitTime=0;
    m_index=0;
    connect(this,
            SIGNAL(readChannelFinished()),
            this,
            SLOT(SlotReadChannelFinished())
            );
    connect(this,
            SIGNAL(readyRead()),
            this,
            SLOT(SlotReadReady())
            );
    connect(this,
            SIGNAL(disconnected()),
            this,
            SLOT(SlotDisconnected())
            );
    connect(this,
            SIGNAL(error(QAbstractSocket::SocketError)),
            this,
            SLOT(SlotError(QAbstractSocket::SocketError))
            );
//    connect(this,
//            SIGNAL(connected()),
//            this,
//            SLOT(SlotConnected())
//            );

    connect(this,
            SIGNAL(bytesWritten(qint64)),
            this,
            SLOT(SlotBytesWritten(qint64))
            );
}

void TcpSocket::Init(const QString& ip, ushort port, int index)
{
    m_remoteIP=ip;
    m_remotePort=port;
    m_index = index;
}

void TcpSocket::CheckConnect()
{
    if(this->state() != QTcpSocket::UnconnectedState){
        return;
    }
    QDateTime now = QDateTime::currentDateTime();
    if(m_lastConnectTime.isValid() ){
        if(m_lastConnectTime.secsTo(now)<10){
            return;
        }
    }

    //发点对点广播
	QUdpSocket *m_pUdpSocket = new QUdpSocket();
	uchar con_packet[41];
	memset(con_packet, 0x00, sizeof(con_packet));
    con_packet[0]=0xFF;
    con_packet[1]=1;
    struct TIME_S
        {
            WORD	ms;
            BYTE	min;
            BYTE	hour;
            BYTE	day;
            BYTE	month;
            BYTE	year;
        }Time;
    Time.year	= (uchar)now.date().year()-2000;
    Time.month	= (uchar)now.date().month();
    Time.day	= (uchar)now.date().day();
    Time.hour	= (uchar)now.time().hour();
    Time.min	= (uchar)now.time().minute();
    Time.ms		= now.time().second()*1000+now.time().msec();
	memcpy(&con_packet[2], &Time, 7);
    QHostAddress ha;
    ha.setAddress(m_remoteIP);
	m_pUdpSocket->writeDatagram((char*)con_packet,41,ha,1032);
//  this->connectToHost(ha,m_remotePort);
    m_lastConnectTime = now;
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProNetLink103,
                         MsgInfo,
                         m_remoteIP,
                         QString("正在连接。。。"));
}

void TcpSocket::Start()
{
    NetPacket np;
    np.SetUType(NetPacket::U_Start_Enable);
    np.SetSourceAddr(0,m_gateWay->GetCenter()
                     ->GetLocalAddr());
    np.SetDestAddr(m_gateWay->GetStationAddr(),
                   m_gateWay->GetDevAddr());

    //    if(m_gateWay->IsClearNumber()){
    //        np.SetNumClear(true);
    //        m_gateWay->SetClearNumber(false);
    //    }
    //    else{
    //        np.SetNumClear(false);
    //    }

    SendPacket(np);
    m_appState = AppStarting;
    StartWait(START_WAIT_T1);
}

void TcpSocket::TestAck(ushort sta, ushort dev)
{
    NetPacket np;
    np.SetUType(NetPacket::U_Test_Enable);
    np.SetSourceAddr(0,m_gateWay->GetCenter()
                     ->GetLocalAddr());
    np.SetDestAddr(sta,dev);
    SendPacket(np);
}

void TcpSocket::Test()
{
    NetPacket np;
    np.SetUType(NetPacket::U_Test_Enable);
    np.SetSourceAddr(0,m_gateWay->GetCenter()
                     ->GetLocalAddr());
    np.SetDestAddr(m_gateWay->GetStationAddr(),
                   m_gateWay->GetDevAddr());
    SendPacket(np);
    m_waitRecv=true;
    StartWait(START_WAIT_T1);
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProNetLink103,
                         MsgInfo,
                         m_remoteIP,
                         QString("发送测试报文")
                         );
}

void TcpSocket::SendPacket(const NetPacket &np)
{
    if(state()!= ConnectedState){
        return;
    }
    if(m_appState!=AppStarted){
        if(np.GetType() != NetPacket::Packet_U){
            return;
        }
        if(np.GetUType() != NetPacket::U_Start_Enable){
            return;
        }
    }
    SendData(np.m_data);
    StartWait(IDEL_WAIT_T3);
}

void TcpSocket::SendData(const QByteArray& data)
{
    if(state()!= ConnectedState){
        return;
    }
    m_sendData+=data;
    SendDataIn();
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProNetLink103,
                         MsgSend,
                         m_remoteIP,
                         QString(),
                         data);
}

void TcpSocket::CheckReceive()
{
    while(m_recvData.length()>=15){
        if((uchar)m_recvData.at(0)!=0x68){
            ProMonitorApp::GetInstance()
                    ->AddMonitor(ProNetLink103,
                                 MsgRecv,
                                 m_remoteIP,
                                 QString(),
                                 m_recvData);
            ProMonitorApp::GetInstance()
                    ->AddMonitor(ProNetLink103,
                                 MsgError,
                                 m_remoteIP,
                                 QString("报文启动字节错误"));
            qDebug()<<"报文启动字节错误:"<<m_remoteIP;
            Close();
            return;
        }
        QByteArray head = m_recvData.left(15);
        NetPacket nph(head);
        ushort len = nph.GetLength();
        //QString ss =  ByteArrayToString(m_recvData);
        //qDebug()<<m_remoteIP<<" 接收缓存:"<<ss;
        if(len>1015){
            ProMonitorApp::GetInstance()
                    ->AddMonitor(ProNetLink103,
                                 MsgRecv,
                                 m_remoteIP,
                                 QString(),
                                 m_recvData);
            ProMonitorApp::GetInstance()
                    ->AddMonitor(ProNetLink103,
                                 MsgError,
                                 m_remoteIP,
                                 QString("报文长度超长错误:%1")
                                 .arg(len));

            qDebug()<<"报文长度超长错误:"<<m_remoteIP
                   <<"长度:"<<len;
            Close();
            return;
        }
        len+=3;
        if(len>m_recvData.count()){
            break;
        }
        QByteArray packetData = m_recvData.left(len);

        ProMonitorApp::GetInstance()
                ->AddMonitor(ProNetLink103,
                             MsgRecv,
                             m_remoteIP,
                             QString(),
                             packetData);

        NetPacket np(packetData);
        if(np.GetType() == NetPacket::Packet_U){
            switch (np.GetUType()) {
            case NetPacket::U_Start_Ack:
            {
                m_appState = AppStarted;
            }
                break;
            case NetPacket::U_Test_Enable:
            {
                ushort dev;
                ushort sta;
                np.GetDestAddr(sta,dev);
                TestAck(sta,dev);
            }
                break;
            default:
                break;
            }
        }
        emit PacketReceived(np,m_index);
        m_recvData = m_recvData.mid(len);
        m_waitRecv=false;
        StartWait(IDEL_WAIT_T3);

    }
}

void TcpSocket::SendDataIn()
{
    if(m_sendData.isEmpty()){
        return;
    }
    int ret = write(m_sendData);
    if(ret<0){
        qDebug()<<"发送错误:"<<m_remoteIP;
        Close();
        return;
    }
    m_sendData = m_sendData.mid(ret);
}

QString TcpSocket::GetRemoteIP()
{
    return m_remoteIP;
}

void TcpSocket::SlotReadReady()
{
    //qDebug()<<"收到链路层";
    while(1){
        QByteArray data = readAll();
        m_recvData+=data;
        if(data.isEmpty()){
            break;
        }
    }
    CheckReceive();
}

void TcpSocket::SlotError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProNetLink103,
                         MsgError,
                         m_remoteIP,
                         QString("链路发生错误:%1")
                         .arg(this->errorString()));
    Close();
}

void TcpSocket::SlotConnected()
{
	this = server->nextPendingConnection();
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProNetLink103,
                         MsgInfo,
                         m_remoteIP,
                         QString("连接成功。"));
    Start();
}

void TcpSocket::SlotReadChannelFinished()
{
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProNetLink103,
                         MsgError,
                         m_remoteIP,
                         QString("读通道被关闭")
                         );
    Close();
}

void TcpSocket::StartWait(int s)
{
    m_waitTime =s;
}

void TcpSocket::OnTimer()
{
    CheckConnect();
    TimerOut();
}

int TcpSocket::GetIndex()
{
    return m_index;
}

void TcpSocket::Close()
{
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProNetLink103,
                         MsgError,
                         m_remoteIP,
                         QString("连接关闭"));
    m_appState=AppNotStart;
    m_recvData.clear();
    m_sendData.clear();
    m_waitRecv=false;
    m_waitTime=0;
    if(isOpen()){
        close();
        emit Closed(m_index);
    }
}

void TcpSocket::SlotDisconnected()
{
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProNetLink103,
                         MsgError,
                         m_remoteIP,
                         QString("连接断开")
                         );
    Close();
}

void TcpSocket::SlotBytesWritten(qint64)
{
    SendDataIn();
}

void TcpSocket::TimerOut()
{
    if(m_appState == AppNotStart){
        return;
    }
    if(m_waitTime>0){
        m_waitTime--;
        return;
    }
    if(m_appState == AppStarting){
        ProMonitorApp::GetInstance()
                ->AddMonitor(ProNetLink103,
                             MsgError,
                             m_remoteIP,
                             QString("初始化应用超时")
                             );
        Close();
        return;
    }
    if(!m_waitRecv){
        Test();
        return;
    }
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProNetLink103,
                         MsgError,
                         m_remoteIP,
                         QString("连接接收时")
                         );
    Close();
    return;
}
