#ifndef MAINSTATION_H
#define MAINSTATION_H

#include <QObject>
#include <QtCore/QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include "103struct.h"

void* UDPThread(void *lp);//要使用pthread_create的话返回和参数只能是这种形式

#endif // MAINSTATION_H
