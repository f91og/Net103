#include "mainstation.h"
#include <QDebug>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QHostAddress>
#include <QUdpSocket>
#include <QDateTime>
#include <pthread.h>
#include <unistd.h>
#include "asdu.h"

void* UDPThread(void* lp)   // 想要使用pthread_create的话，这个函数的参数形式一定要是这样的
{
    while(true)
    {
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
            BYTE	day;
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
        m_pUdpServer->writeDatagram((char*)connectpacket,41,QHostAddress::Broadcast,1032);
        sleep(30);
    }
}

