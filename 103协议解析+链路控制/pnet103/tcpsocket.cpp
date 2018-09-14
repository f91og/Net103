#include "tcpsocket.h"
#include <QHostAddress>
#include "gateway.h"
#include "netpacket.h"
#include "clientcenter.h"
#include <QtDebug>
#include "tool/toolapi.h"
#include "comm/promonitorapp.h"
#include <QSettings>

TcpSocket::TcpSocket(GateWay *parent):
    QObject(parent)
{
    m_appState=AppNotStart;
    m_gateWay = parent;
    m_waitRecv=false;
    m_index=0;
    socket=NULL;
    QSettings setting("../etc/p103.ini",QSettings::IniFormat);
    setting.beginGroup("p103");
    link_t1 = setting.value("link_t1",60).toUInt();
    link_t2 = setting.value("link_t2",30).toUInt();
    setting.endGroup();
    m_udpBroadcastTime=12;
    m_udpHandShakeTime=10;
    m_waitTime=link_t1;
    m_lastConnectTime=link_t2;
}

void TcpSocket::Init(const QString& ip, ushort port, int index)
{
    m_remoteIP = ip;
    m_remotePort = port;
    m_index = index;
}

void TcpSocket::CheckConnect(QTcpSocket* s)
{
    QString ip = s->peerAddress().toString();
    if(ip.contains(m_remoteIP)&&(socket==NULL)){
        socket = s;
     //   m_appState = AppStarting;
        connect(socket,&QTcpSocket::readyRead,this,&TcpSocket::SlotReadReady);
        connect(socket,&QTcpSocket::disconnected,this,&TcpSocket::SlotDisconnected);
        connect(socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(SlotError(QAbstractSocket::SocketError)));
    }
}

void TcpSocket::SendPacket(const NetPacket &np)
{
    SendData(np.GetAppData());
}

void TcpSocket::SendData(const QByteArray& data)
{
    if(socket==NULL||socket->state()!= QTcpSocket::ConnectedState){
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
    //粘包处理
    while(m_recvData.length()>=10){
        ProMonitorApp::GetInstance()
                ->AddMonitor(ProNetLink103,
                             MsgRecv,
                             m_remoteIP,
                             QString(),
                             m_recvData);
        uchar firt_byte=m_recvData[0];
        while(m_recvData.length()>0 && firt_byte!=0x0a && firt_byte!=0xc9 && firt_byte!=0xc8 && firt_byte!=0x05){
            m_recvData=m_recvData.mid(1);
            firt_byte=m_recvData[0];
        }
        QByteArray packet;
        QByteArray data;
        bool isValid=false;
        uchar vsq=m_recvData[1];
        uchar cot=m_recvData[2];
        uchar fun=m_recvData[4];
        uchar inf=m_recvData[5];
        qDebug()<<"vsq"<<vsq<<"cot"<<cot<<"fun"<<fun<<"inf"<<inf;
        if(firt_byte==0xc9 && m_recvData.length()>=17){ // ASDU201
            if(vsq!=0x81 || cot!=9 || fun!=0xff || inf!=0){
                m_recvData=m_recvData.mid(1);
                return;
            }
            ushort listNum=m_recvData[16]<<8 | m_recvData[15];
            packet=m_recvData.left(17);
            QByteArray data_all=m_recvData.mid(17);
            while (data_all.length()>=42 && listNum>0) {
                data.append(data_all.left(42));
                data_all=data_all.mid(42);
                listNum--;
            }
            if(listNum==0) isValid=true;
        }else if(firt_byte==0x05 && m_recvData.length()>=19){   // ASDU5
            if(vsq!=0x81 || (fun!=0xfe && fun!=0xff) || inf!=2){
                m_recvData=m_recvData.mid(1);
                return;
            }
            packet=m_recvData.left(19);
            isValid=true;
        }else if(firt_byte==0xc8){  // ASDU200
            if(vsq!=0x81 || cot!=9 || fun!=0xff || inf!=0){
                m_recvData=m_recvData.mid(1);
                return;
            }
            if(m_recvData.size()<67) return; // 长度不够
            QByteArray temp;
            temp[0]=0xc8;
            temp[1]=0x81;
            temp[2]=0x09;
            temp[3]=m_recvData[3];
            int index=m_recvData.indexOf(temp,4);
            int lastIndex=m_recvData.lastIndexOf(temp);
            qDebug()<<"index--><"<<index;
            isValid=true;
            if(lastIndex==0){
                if(m_recvData[10]==0){
                    packet=m_recvData;
                }else{
                    return;
                }
            }else{
               packet=m_recvData.left(index);
            }
        }else if(firt_byte==0x0a){
            if(vsq!=0x81 || (fun!=0xfe && fun!=0xff)){
                m_recvData=m_recvData.mid(1);
                return;
            }
            if((uchar)m_recvData[2]==0x2b){    // 2b表示读命令的无效数据响应，固定返回10个字节
                packet=m_recvData.left(10);
                isValid=true;
            }else{
                packet=m_recvData.left(8);
                QByteArray data_all=m_recvData.mid(8);
                ushort ngd_o=packet[7]; // 这里一定要先将uchar转为整数再进行取余运算
                uchar ngd=ngd_o%64;
                qDebug()<<"ngd-->"<<ngd;
                while(data_all.length()>=6 && ngd>0){
                    uchar data_size=data_all[4];
                    uchar num=data_all[5];
                    if(data_all.length()-6>=data_size*num){
                        data.append(data_all.left(6+data_size*num));
                        data_all=data_all.mid(6+data_size*num);
                        ngd--;
                    }else{
                        break;
                    }
                }
                qDebug()<<"ngd-->"<<ngd;
                if(ngd==0) isValid=true;
            }
        }
        if(isValid==false){
            break;
        }
        packet.append(data);
        ProMonitorApp::GetInstance()
                ->AddMonitor(ProNetLink103,
                             MsgRecv,
                             m_remoteIP,
                             QString(),
                             packet);

        m_recvData=m_recvData.mid(packet.length());
        NetPacket np(packet);
        cpu_no=packet[3];
        np.SetRemoteIP(m_remoteIP);
        np.SetDestAddr(0,cpu_no);
        emit PacketReceived(np,m_index);
        m_appState=AppStarted;
        StartWait(link_t1);
    }
}

void TcpSocket::SendDataIn()
{
    if(m_sendData.isEmpty()){
        return;
    }
    if(socket==NULL)
        return;
    if(socket->state()!=QTcpSocket::ConnectedState)
        return;
    int ret = socket->write(m_sendData);
    socket->flush();
    if(ret<0){
        qDebug()<<"发送错误:"<<m_remoteIP;
        Close();
        return;
    }
    //m_sendData = m_sendData.mid(ret);
}

QString TcpSocket::GetRemoteIP()
{
    return m_remoteIP;
}

QTcpSocket* TcpSocket::GetTcpSocket()
{
    return socket;
}

void TcpSocket::SlotReadReady()
{
    while(1){
        QByteArray data=socket->readAll();
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
    //m_appState = AppStarting;
    //StartWait(START_WAIT_T1);// socket连接成功后，等待数据包传送，有数据包传送才能认为是应用真正启动了
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
    TimerOut();
}

void TcpSocket::TimerOut()
{
    qDebug()<<"定时器标记";
    if(m_appState==AppStarted){
        qDebug()<<"m_udpHandShakeTime"<<m_udpHandShakeTime;
        if(m_udpHandShakeTime>0){
            m_udpHandShakeTime--;
            if(m_udpHandShakeTime==0){
                m_gateWay->SendUdp(m_remoteIP,m_index,true);
                m_udpHandShakeTime=10;
            }
        }
        qDebug()<<"m_waitTime"<<m_waitTime;
        if(m_waitTime>0){
            m_waitTime--;
            return;
        }
        ProMonitorApp::GetInstance()
                ->AddMonitor(ProNetLink103,
                             MsgError,
                             m_remoteIP,
                             QString("连接接收超时"));
        Close();
    }else{
        qDebug()<<"m_udpBroadcastTime"<<m_udpBroadcastTime;
        m_udpBroadcastTime--;
        if(m_udpBroadcastTime==0){
            m_gateWay->SendUdp(m_remoteIP,m_index,false);
            m_udpBroadcastTime=12;
        }
    }
    if(m_appState==AppRestarting){
        m_lastConnectTime--;
        qDebug()<<"m_lastConnectTime"<<m_lastConnectTime;
        if(m_lastConnectTime==0){
            QByteArray send(20,0);
            uchar array[14]={0x0a,0x81,0x01,cpu_no,0xfe,
                             0xf4,0x01,0x01,0x05,0x25,0x01,0x12,0x06,0x01};
            memcpy(send.data(), &array, 14); //利用memcpy将数组赋给QByteArray，可以指定复制的起始位和复制多少个
            send[14]=2;
            if(m_index==1) send[9]=0x26;
            QDateTime now = QDateTime::currentDateTime();
            struct TIME_S
            {
                ushort	ms;
                uchar	min;
                uchar	hour;
            }Time;
            Time.hour	= (uchar)now.time().hour();
            Time.min	= (uchar)now.time().minute();
            Time.ms		= now.time().second()*1000+now.time().msec();
            memcpy(send.data()+15, &Time, 4);
            NetPacket np(send);
            np.SetRemoteIP(m_remoteIP);
            np.SetDestAddr(0,cpu_no);
            emit PacketReceived(np,m_index);
        }
    }
}

int TcpSocket::GetIndex()
{
    return m_index;
}

bool TcpSocket::IsConnected(){
    if(socket==NULL) return false;
    return socket->state()==QTcpSocket::ConnectedState;
}

void TcpSocket::Close()
{
    if(socket==NULL) return;
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProNetLink103,
                         MsgError,
                         m_remoteIP,
                         QString("连接关闭"));
    m_appState=AppNotStart;
    m_recvData.clear();
    m_sendData.clear();
    m_waitRecv=false;
    m_waitTime=link_t1;
    if(socket->isOpen()){
        socket->close();
        socket=NULL;
        emit Closed(m_index, m_remoteIP, cpu_no);
        ProMonitorApp::GetInstance()
                ->AddMonitor(ProNetLink103,
                             MsgInfo,
                             m_remoteIP,
                             QString("重新连接中"));
        m_lastConnectTime=link_t2;
        m_gateWay->SendUdp(m_remoteIP,m_index,false);
        m_appState=AppRestarting;
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
