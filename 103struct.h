#ifndef STRUCT103_H
#define STRUCT103_H

#include <QByteArray>

typedef uchar BYTE;
typedef ushort WORD;

#pragma pack(1)
typedef union tag103NGD
{
    struct tagngd{
        BYTE No	:6;//UI[1~6]<1~63>：数目
        BYTE Count	:1;//BS1[7]<0~1>：具有相同的通用分类标识序号（RII）的应用服务数据单元的一位计数器位，计数器位初始值为0
        BYTE Cont	:1;//BS1[8]<0~1>：0：后面未跟随具有相同的返回标志符（RII）的应用服务数据单元。
    };
    tagngd ngd;
    BYTE byte;
}NGD;//通用分类数据集数目（NGD）：CP8{数目（No），计数器位（Count），后续状态位（Cont）}

typedef union tag103GDD
{
    struct taggdd{// P22 7.2.6.32
        BYTE DataType;		//数据类型（DATATYPE）
        BYTE DataSize;		//数据宽度（DATASIZE）
        BYTE Number	:7;	//数目（NUMBER）
        BYTE Cont	:1;	//后续状态位（CONT）
    };
    taggdd gdd;
    BYTE byte[3];
}
GDD;//通用分类数据描述（GDD）：CP24{数据类型（DATATYPE），数据宽度（DATASIZE），数目（NUMBER），后续状态位（CONT）}

typedef struct tagGIN
{
    BYTE GROUP;//组号
    BYTE ENTRY;//条目
}GIN;//通用分类标识序号-7.2.6.3

typedef struct tagDataSet
{
    GIN gin;
    BYTE kod;
    GDD gdd;
    QByteArray gid;
}DataSet;//通用分类服务数据集

typedef struct tagASdu10LinkDataSet
{
    BYTE kod;
    BYTE datatype;
    BYTE datasize;
    BYTE number;
    QByteArray gid;
}ASdu10LinkDataSet;

typedef struct tagASdu10LinkData
{
    BYTE datatype;
    BYTE datasize;
    BYTE number;
    QByteArray gid;
}ASdu10LinkData;

typedef union tagMEA
{
    struct _tagMEA
    {
        WORD OV		:1;//溢出位：0无溢出，1溢出
        WORD ER		:1;//差错位：0被测值（MVAL）有效，1无效
        WORD res 		:1;//未用
        WORD MVAL	:13;//被测值
    }mea;
    WORD word;
}MEA;

typedef union tagCP32TIME
{
    struct tagTime{
        WORD Milliseconds;
        BYTE Minutes;
        BYTE Hours	:5;
        BYTE Weeks	:3;
    };
    tagTime Time;
    BYTE byte[4];
}CP32Time2a;

typedef struct tagFileAsdu201
{
    BYTE m_addr;
    WORD lubo_num;
    QByteArray file_name;
    QByteArray fault_time;
}FileAsdu201;

#pragma pack()

#endif
