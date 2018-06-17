#ifndef ASDU_H
#define ASDU_H

#include "103struct.h"
#include <QByteArray>
#include <qstringlist.h>
#include <QList>

class CAsdu
{
public:
    CAsdu();
    virtual ~CAsdu();

//    virtual void BuildArray(QByteArray& Data);
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
    virtual void ExplainAsdu(int iProcessType = 0);
    virtual void BuildArray(QByteArray& Data);

    BYTE m_RII;
    NGD m_NGD;
    QList<DataSet> m_DataSets;
};

class CAsdu10Link:public CAsdu
{
public:
    CAsdu10Link();
    CAsdu10Link(QByteArray& Data);
    ~CAsdu10Link();
public:
    BYTE m_RII;
    BYTE m_NOG;
    GIN gin_h;
    BYTE kod;
    ASdu10LinkData d1;
    int reportArgNum;
    QList<ASdu10LinkDataSet> m_DataSet;
};

class CAsdu21 : public CAsdu
{
public:
    CAsdu21();
    ~CAsdu21();
    virtual void BuildArray(QByteArray& Data);
public:
    BYTE m_RII;
    BYTE m_NOG;
    QList<DataSet> m_DataSets;
};

class CAsdu201 : public CAsdu
{
public:
    CAsdu201();
    CAsdu201(CAsdu &a);
//    ~CAsdu201(); 没有析构函数可以嘛
    virtual void BuildArray(QByteArray& Data);
public:
    BYTE tag;//后续标志位
    time_t start_time;
    time_t end_time;
    WORD listNum;
    QList<FileAsdu201> m_DataSets;
};

class CAsdu200 : public CAsdu
{
public :
    CAsdu200();
    CAsdu200(CAsdu& a);
    virtual void BuildArray(QByteArray& Data);
public:
    QByteArray offset;
    BYTE followTag;
    QByteArray file_name;
    QByteArray file_content;
};

#endif // ASDU_H
