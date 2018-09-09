#include <QSettings>
#include "tcpserver.h"

TcpServer::TcpServer(QObject *parent) :
    QTcpServer(parent)
{
}

void TcpServer::incomingConnection(qintptr handle)
{
    QTcpSocket* socket=new QTcpSocket();
    socket->setSocketDescriptor(handle);
    foreach(GateWay* g, m_lstGateWay)
    {
        g->SyncSocket(socket);
    }
}

void TcpServer::SetGateWay(QList<GateWay *> listGateWay){
    m_lstGateWay=listGateWay;
}
