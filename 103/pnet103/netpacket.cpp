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

void NetPacket::SetLength(ushort len)
{
    ToolNumberToByte((uchar*)m_data.data()+1,len);
}

ushort NetPacket::GetLength() const
{
    return ToolByteToNumber<ushort>((uchar*)m_data.data()+1);
}

void NetPacket::SetDestAddr(ushort sta,ushort dev)
{
    dev_ip = sta;
    dev_cpu=dev;
}

void NetPacket::GetDestAddr(ushort& sta,ushort& dev) const
{
    sta = dev_ip;
    dev = dev_cpu;
}

void NetPacket::AddAppData(const QByteArray& data)
{
    m_data+=data;
    SetLength(GetLength()+data.count());
}

QByteArray NetPacket::GetAppData()const
{
    return m_data;
}
