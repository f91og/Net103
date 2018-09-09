#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <QTcpSocket>
#include <QDateTime>
#include "netpacket.h"
#include <QTimer>

class GateWay;

class TcpSocket: public QObject
{
    Q_OBJECT
public:
    enum AppState{
        AppNotStart,
        AppStarting,
        AppStarted,
        AppRestarting
    };

public:
    explicit TcpSocket(GateWay *parent = 0);
    void Init(const QString& ip,ushort port,
              int index);

    void CheckConnect(QTcpSocket* socket);
    void TimerOut();
    QTcpSocket* GetTcpSocket();
    void SendPacket(const NetPacket& np);
    QString GetRemoteIP();

    void Close();

    void StartWait(int s);

    int GetIndex();
    bool IsConnected();
    void OnTimer();
private:
    void SendData(const QByteArray& data);
    void SendDataIn();
    void CheckReceive();
signals:
    void PacketReceived(const NetPacket& np,int index);
    void Closed(int index, QString m_remoteIP, uchar cpu_no);
public slots:
    void SlotReadReady();
    void SlotError(QAbstractSocket::SocketError socketError);
    void SlotConnected();
    void SlotReadChannelFinished();
    void SlotDisconnected();
    void SlotBytesWritten(qint64);

private:
    QTcpSocket *socket;
    QString m_remoteIP;
    ushort m_remotePort;
    ushort m_lastConnectTime;
    AppState m_appState;
    GateWay* m_gateWay;

    QByteArray m_sendData;
    QByteArray m_recvData;

    bool m_waitRecv;
    int m_waitTime;
    int m_index;
    uchar cpu_no;
    int m_udpBroadcastTime;
    int m_udpHandShakeTime;
    int link_t1;
    int link_t2;
};

#endif // TCPSOCKET_H
