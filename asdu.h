#ifndef ASDU_H
#define ASDU_H

#include "103struct.h"
#include <QByteArray>

class CAsdu
{
public:
    CAsdu();
    virtual ~CAsdu();

    virtual void BuildArray(QByteArray& Data);
    void SaveAsdu(QByteArray& Data); //把引用作为参数

    BYTE m_TYP;   //类型标识
    BYTE m_VSQ;   //可变结构限定词
    BYTE m_COT;   //传送原因
    BYTE m_Addr;  //应用服务单元公共地址
    BYTE m_FUN;   //功能类型
    BYTE m_INF;   //信息序号

    QByteArray ASDUData; //原始数据
};

class CAsdu07 : public CAsdu
{
public:
    CAsdu07();
    CAsdu07(CAsdu& a);  //把引用作为参数
    virtual void BuildArray(QByteArray& Data);

    BYTE m_SCN;   //扫描序号
};

class CAsdu10 : public CAsdu
{
public:
    CAsdu10();
    CAsdu10(CAsdu& a);
    ~CAsdu10();
    void Init();
    virtual BuildArray(QByteArray& Data);

    BYTE m_RII;  //返回信息表示符
    NGD m_NGD;

};

class CAsdu20 ： public CAsdu
{

};

class CAsdu21 : public CAsdu
{

};

class CAsdu201 : public CAsdu
{

};

class CAsdu200 : public CAsdu
{

};

#endif // ASDU_H
