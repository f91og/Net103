#ifndef NETPACKET_H
#define NETPACKET_H
#include <QByteArray>
#include <QString>

class NetPacket
{
public:
    NetPacket();
    NetPacket(const QByteArray& ba);
    ushort GetLength()const;

    void SetDestAddr(ushort sta,uchar cpu_no);
    void GetDestAddr(ushort& sta,ushort& dev)const;

    void SetRemoteIP(QString ip);
    QString GetRemoteIP();

    void AddAppData(const QByteArray& data);
    QByteArray GetAppData()const;
private:
    QString m_remoteIP;
    ushort dev_addr;
    QByteArray m_data;
};

#endif // NETPACKET_H
