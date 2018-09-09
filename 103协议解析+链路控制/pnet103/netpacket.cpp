#include "netpacket.h"
#include <QByteArray>
#include "tool/toolapi.h"
#include<QDebug>

NetPacket::NetPacket()
{
}

NetPacket::NetPacket(const QByteArray& ba)
{
    m_data=ba;
}


ushort NetPacket::GetLength() const
{
    return ToolByteToNumber<ushort>((uchar*)m_data.data()+1);
}

void NetPacket::SetDestAddr(ushort sta,uchar cpu_no)
{
    QString ip_last=m_remoteIP.section(".",3,3);
    dev_addr=ip_last.toUShort()*0x100+cpu_no;
}

void NetPacket::GetDestAddr(ushort& sta,ushort& dev) const
{
    sta=0;
    dev=dev_addr;
}

void NetPacket::SetRemoteIP(QString ip)
{
    m_remoteIP=ip;
}

QString NetPacket::GetRemoteIP(){
    return m_remoteIP;
}

void NetPacket::AddAppData(const QByteArray& data)
{
    m_data=data;
}

QByteArray NetPacket::GetAppData()const
{
    return m_data;
}
