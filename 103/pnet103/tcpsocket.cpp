#include "tcpsocket.h"
#include <QHostAddress>
#include <QUdpSocket>
#include "gateway.h"
#include "netpacket.h"
#include "clientcenter.h"
#include <QtDebug>
#include "tool/toolapi.h"
#include "comm/promonitorapp.h"

TcpSocket::TcpSocket(GateWay *parent):
    QObject(parent)
{
    m_appState=AppNotStart;
    m_gateWay = parent;
    m_waitRecv=false;
    m_waitTime=0;
    m_index=0;
    socket=new QTcpSocket();

//    connect(socket,
//            SIGNAL(readChannelFinished()),
//            this,
//            SLOT(SlotReadChannelFinished())
//            );
//    connect(socket,
//            SIGNAL(readyRead()),
//            this,
//            SLOT(SlotReadReady())
//            );
//    connect(socket,
//            SIGNAL(disconnected()),
//            this,
//            SLOT(SlotDisconnected())
//            );
//    connect(socket,
//            SIGNAL(error(QAbstractSocket::SocketError)),
//            this,
//            SLOT(SlotError(QAbstractSocket::SocketError))
//            );

//    connect(socket,
//            SIGNAL(bytesWritten(qint64)),
//            this,
//            SLOT(SlotBytesWritten(qint64))
//            );
}

void TcpSocket::Init(const QString& ip, ushort port, int index)
{
    m_remoteIP=ip;
    m_remotePort=port;
    m_index = index;
}

void TcpSocket::CheckConnect()
{
    if(socket->state()!=QAbstractSocket::UnconnectedState){
        return;
    }

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
    ha.setAddress(m_remoteIP);
	m_pUdpSocket->writeDatagram((char*)con_packet,41,ha,1032);
    m_lastConnectTime = now;
    QTcpServer* server=m_gateWay->GetCenter()->GetTcpServer();
    while(1){
        if(!server->waitForNewConnection(-1)) break;
        socket=server->nextPendingConnection();
        m_appState = AppStarting;
        connect(socket,&QTcpSocket::readyRead,this,&TcpSocket::SlotReadReady);
//        connect(socket,&QTcpSocket::readChannelFinished,this,&TcpSocket::SlotReadChannelFinished);
        connect(socket,&QTcpSocket::disconnected,this,&TcpSocket::SlotDisconnected);
        connect(socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(SlotError(QAbstractSocket::SocketError)));
        connect(socket,SIGNAL(bytesWritten(qint64)),this,SLOT(SlotBytesWritten(qint64)));
        break;
    }
}

void TcpSocket::TimerOut()
{
    if(socket->state()!=QAbstractSocket::ConnectedState){
        return;
    }
    qDebug()<<"发送心跳报文";
    QByteArray data;
    data.resize(9);
    data.fill(0);//暂且不对时
    data[0]=0x88;
    SendData(data);
}

void TcpSocket::SendPacket(const NetPacket &np)
{
    if(socket->state()!= QTcpSocket::ConnectedState){
        return;
    }
    SendData(np.m_data);
}

void TcpSocket::SendData(const QByteArray& data)
{
    if(socket->state()!= QTcpSocket::ConnectedState){
        return;
    }
    m_sendData.clear();
    m_sendData=data;
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
    qDebug()<<"收到的报文长度："<<m_recvData.length();
     if(m_recvData.length()>=6){
        ushort len = m_recvData.size();
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

        ProMonitorApp::GetInstance()
                ->AddMonitor(ProNetLink103,
                             MsgRecv,
                             m_remoteIP,
                             QString(),
                             m_recvData);

//        //暂时不
        NetPacket np(m_recvData);
        //np.SetDestAddr(socket->peerAddress().toString(),m_recvData[3]);
        np.m_remoteIP=m_remoteIP;
        emit PacketReceived(np,m_index);
        StartWait(IDEL_WAIT_T3);
    }
}

void TcpSocket::SendDataIn()
{
    if(m_sendData.isEmpty()){
        return;
    }
    int ret = socket->write(m_sendData);
//    socket->flush();
    if(ret<0){
        qDebug()<<"发送错误:"<<m_remoteIP;
        Close();
        return;
    }
//    m_sendData = m_sendData.mid(ret);
}

QString TcpSocket::GetRemoteIP()
{
    return m_remoteIP;
}

void TcpSocket::SlotReadReady()
{
    QByteArray data = socket->readAll();
    m_recvData.clear();
    m_recvData=data;
    if(data.isEmpty()){
        return;
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
                         .arg(this->socket->errorString()));
    Close();
}

void TcpSocket::SlotConnected()
{
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProNetLink103,
                         MsgInfo,
                         m_remoteIP,
                         QString("连接成功。"));
    m_appState = AppStarting;
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
    TimerOut();//在这个方法里发心跳报文以维持连接
}

int TcpSocket::GetIndex()
{
    return m_index;
}

void TcpSocket::Close()
{
//    ProMonitorApp::GetInstance()
//            ->AddMonitor(ProNetLink103,
//                         MsgError,
//                         m_remoteIP,
//                         QString("连接关闭"));
//    m_appState=AppNotStart;
//    m_recvData.clear();
//    m_sendData.clear();
//    m_waitRecv=false;
//    m_waitTime=0;
//    if(socket->isOpen()){
//        socket->close();
//        emit Closed(m_index);
//    }
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
