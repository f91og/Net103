#include <QtNetwork>
#include<windows.h>

int main(int argc, char *argv[])
{
    QUdpSocket *sender;
    sender=new QUdpSocket();
    QByteArray data;
    data.resize(41);
    data.fill(0);
    data[0]=0xff;
    while (1) {
       printf("send udp...");
       sender->writeDatagram(data.data(),data.size(),QHostAddress::Broadcast,1032);
       Sleep(30000);
    }
    return 0;
}
