#ifndef NETPACKET_H
#define NETPACKET_H
#include <QByteArray>

class NetPacket
{
public:
    enum Type{
        Packet_U,
        Packet_S,
        Packet_I
    };
    enum U_Type{
        U_Test_Enable = 0x43,
        U_Test_Ack = 0x83,
        U_Stop_Enable = 0x13,
        U_Stop_Ack =0x23,
        U_Start_Enable=0x07,
        U_Start_Ack=0x0b
    };

public:
    NetPacket();
    NetPacket(const QByteArray& ba);
    void SetType(Type tp);
    void SetUType(U_Type ut);
    Type GetType()const;
    U_Type GetUType()const;
    void SetLength(ushort len);
    ushort GetLength()const;
    void SetSourceAddr(ushort sta,ushort dev);
    void GetSourceAddr(ushort& sta,ushort& dev)const;

    void SetDestAddr(ushort sta,ushort dev);
    void GetDestAddr(ushort& sta,ushort& dev)const;

    ushort GetNumber(int index)const;
    void SetNumber(int index,ushort num);

    bool GetNumClear()const;
    void SetNumClear(bool nc);

    void AddAppData(const QByteArray& data);
    QByteArray GetAppData()const;
public:
    QByteArray m_data;
};

#endif // NETPACKET_H
