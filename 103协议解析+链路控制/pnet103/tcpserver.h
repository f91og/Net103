#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include "gateway.h"

class TcpServer : public QTcpServer
{
    Q_OBJECT
public:
    TcpServer(QObject *parent = 0);
    void SetGateWay(QList<GateWay*> listGateWay);

protected:
    virtual void incomingConnection(qintptr handle) Q_DECL_OVERRIDE;

private:
    QList<GateWay*> m_lstGateWay;
};

#endif // TCPSERVER_H

