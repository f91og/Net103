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
        AppStarted
    };

public:
    explicit TcpSocket(GateWay *parent = 0);
    void Init(const QString& ip,ushort port,
              int index);

    void CheckConnect();

    void SendPacket(const NetPacket& np);
    QString GetRemoteIP();

    void Close();

    void StartWait(int s);

    int GetIndex();
    void OnTimer();
private:
    void SendData(const QByteArray& data);
    void SendDataIn();
    void CheckReceive();
//    void TimerOut();
signals:
    void PacketReceived(const NetPacket& np,int index);
    void Closed(int index);
public slots:
    void SlotReadReady();
    void SlotError(QAbstractSocket::SocketError socketError);
    void SlotConnected();
    void SlotReadChannelFinished();
    void SlotDisconnected();
    void SlotBytesWritten(qint64);
public:
    QTcpSocket *socket;
    QString m_remoteIP;
    ushort m_remotePort;
    QDateTime m_lastConnectTime;
    AppState m_appState;
    GateWay* m_gateWay;

    QByteArray m_sendData;
    QByteArray m_recvData;

    bool m_waitRecv;
    int m_waitTime;
    int m_index;
};

#endif // TCPSOCKET_H
