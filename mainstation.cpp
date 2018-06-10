#include "mainstation.h"
#include <QDebug>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QHostAddress>
#include <QUdpSocket>
#include <QDateTime>

SendUdpBrocastThread::SendUdpBrocastThread()
{
}

SendUdpBrocastThread::~SendUdpBrocastThread()
{
}

void SendUdpBrocastThread::run()
{
    m_pTimer=new QTimer();
    connect(m_pTimer , &QTimer::timeout , this , &SendUdpBrocastThread::slot_time);
    qDebug()<<"start sending...\n";
    m_pTimer->start(10000) ;
    this->exec() ;
}

void SendUdpBrocastThread::slot_time(){
    QUdpSocket *m_pUdpServer = new QUdpSocket();
    BYTE connectpacket[41];
    memset(connectpacket, 0x00, sizeof(connectpacket));
    connectpacket[0]=0xFF;
    connectpacket[1]=1;
    QDateTime dt = QDateTime::currentDateTime();
    struct TIME_S
    {
        WORD	ms;
        BYTE	min;
        BYTE	hour;
        BYTE	day:5;
        BYTE	week:3;
        BYTE	month;
        BYTE	year;
    }Time;
    Time.year	= (BYTE)dt.date().year()-2000;
    Time.month	= (BYTE)dt.date().month();
    Time.day	= (BYTE)dt.date().day();
    Time.hour	= (BYTE)dt.time().hour();
    Time.min	= (BYTE)dt.time().minute();
    Time.ms		= dt.time().second()*1000+dt.time().msec();
    memcpy(&connectpacket[2], &Time, 7);
    qDebug()<<"send connecting packet";
    qDebug()<<connectpacket;
    m_pUdpServer->writeDatagram((char*)connectpacket,41,QHostAddress::Broadcast,1032);
}
