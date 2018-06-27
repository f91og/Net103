#include "netpacket.h"
#include <QByteArray>
#include "tool/toolapi.h"

NetPacket::NetPacket()
{
    m_data.resize(15);
    m_data.fill(0);
    m_data[0]=0x68;
    SetLength(12);
}

NetPacket::NetPacket(const QByteArray& ba)
{
    m_data=ba;
}

void NetPacket::SetUType(U_Type ut)
{
    m_data[3]=ut;
}

void NetPacket::SetType(Type tp)
{
    switch (tp) {
    case Packet_S:
        m_data[3]=0x01;
        break;
    case Packet_U:
        m_data[3]=0x03;
        break;
    case Packet_I:
        m_data[3]=0x00;
        break;
    default:
        break;
    }
}

void NetPacket::SetLength(ushort len)
{
    ToolNumberToByte((uchar*)m_data.data()+1,len);
}

ushort NetPacket::GetLength() const
{
    return ToolByteToNumber<ushort>((uchar*)m_data.data()+1);
}

void NetPacket::SetSourceAddr(ushort sta,ushort dev)
{
    m_data[7] = sta;
    ToolNumberToByte((uchar*)m_data.data()+8,dev);
}

void NetPacket::GetSourceAddr(ushort& sta,ushort& dev) const
{
    sta = m_data[7];
    dev = ToolByteToNumber<ushort>((uchar*)m_data.data()+8);
}

void NetPacket::SetDestAddr(ushort sta,ushort dev)
{
    m_data[10] = sta;
    ToolNumberToByte((uchar*)m_data.data()+11,dev);
}

void NetPacket::GetDestAddr(ushort& sta,ushort& dev) const
{
    sta = m_data[10];
    dev = ToolByteToNumber<ushort>((uchar*)m_data.data()+11);
}

NetPacket::Type NetPacket::GetType() const
{
    uchar c1 = m_data[3];
    if( (c1 & 0x01) ==0 ){
        return Packet_I;
    }
    if( (c1 & 0x02) == 0 ){
        return Packet_S;
    }
    return Packet_U;
}

NetPacket::U_Type NetPacket::GetUType() const
{
    return (U_Type)m_data.at(3);
}

ushort NetPacket::GetNumber(int index) const
{
    if(index!=0){
        index=1;
    }
    int pos = 3+index*2;    
    return (ToolByteToNumber<ushort>((uchar*)m_data.data()+pos))>>1;
}

bool NetPacket::GetNumClear() const
{
    return m_data.at(4)!=0;
}

void NetPacket::SetNumClear(bool nc)
{
    m_data[4]=(nc?1:0);
}

void NetPacket::SetNumber(int index,ushort num)
{
    if(index!=0){
        index=1;
    }
    int pos = 3+index*2;
    ushort nd = num<<1;
    ToolNumberToByte((uchar*)m_data.data()+pos,nd);
}

void NetPacket::AddAppData(const QByteArray& data)
{
    m_data+=data;
    SetLength(GetLength()+data.count());
}

QByteArray NetPacket::GetAppData()const
{
    if(m_data.count()>15){
        return m_data.mid(15);
    }
    return QByteArray();
}
