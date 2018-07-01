#include "netpacket.h"
#include <QByteArray>
#include "tool/toolapi.h"

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

void NetPacket::SetDestAddr(ushort sta,ushort dev)
{
    dev_addr=dev;
}

void NetPacket::GetDestAddr(ushort& sta,ushort& dev) const
{
    sta=0;
    QString ip_last=m_remoteIP.section(".",3,3);
    dev=ip_last.toUShort()*0x100+01;
//    dev=171*0x100+01;
}

void NetPacket::AddAppData(const QByteArray& data)
{
    m_data=data;
}

QByteArray NetPacket::GetAppData()const
{
    return m_data;
}
