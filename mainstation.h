#ifndef MAINSTATION_H
#define MAINSTATION_H

#include <QObject>
#include <QtCore/QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include "103struct.h"

void* UDPThread(void *lp);

#endif // MAINSTATION_H
