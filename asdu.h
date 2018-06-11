#ifndef ASDU_H
#define ASDU_H

#include "103struct.h"
#include <QByteArray>
#include <qstringlist.h>

class CAsdu
{
public:
    CAsdu();
    virtual ~CAsdu();

    virtual void BuildArray(QByteArray& Data);
    void SaveAsdu(QByteArray& Data);

    BYTE m_TYP; //ASDU 类型标识
    BYTE m_VSQ; //ASDU 可变结构限定词
    BYTE m_COT; //ASDU 传送原因
    BYTE m_Addr; //ASDU 应用服务单元公共地址
    BYTE m_FUN; //ASDU 功能类型
    BYTE m_INF; //ASDU 信息序号

    QByteArray m_ASDUData; //ASDU中的数据

    int m_iResult; //标识是否是合法ASDU?
};

class CAsdu07 : public CAsdu
{
public:
    CAsdu07();
    CAsdu07(CAsdu& a);
    virtual void BuildArray(QByteArray& Data);

    BYTE m_SCN;
};

class CAsdu10 : public CAsdu
{
public:
    CAsdu10();//主站发子站的话就用这个
    CAsdu10(CAsdu& a);//主站收子站的话就用这个
    ~CAsdu10();
    void Init();
    virtual void BuildArray(QByteArray& Data);
    virtual void ExplainAsdu();
    virtual void ExplainGID(QByteArray& Gid);

    BYTE m_RII;
    NGD m_NGD;
    QPtrList<DataSet> m_DataSets;
};

//class CAsdu20 ： public CAsdu
//{

//};

//class CAsdu21 : public CAsdu
//{

//};

//class CAsdu201 : public CAsdu
//{

//};

//class CAsdu200 : public CAsdu
//{

//};

#endif // ASDU_H
