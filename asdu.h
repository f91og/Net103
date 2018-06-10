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

    BYTE m_TYP;
    BYTE m_VSQ;
    BYTE m_COT;
    BYTE m_Addr;
    BYTE m_FUN;
    BYTE m_INF;

    QByteArray m_ASDUData;
};

class CAsdu07 : public CAsdu
{
public:
    CAsdu07();
    CAsdu07(CAsdu& a);
    virtual void BuildArray(QByteArray& Data);

    BYTE m_SCN;
};

//class CAsdu10 : public CAsdu
//{
//public:
//    CAsdu10();
//    CAsdu10(CAsdu& a);
//    ~CAsdu10();
//    void Init();
//    virtual BuildArray(QByteArray& Data);

//    BYTE m_RII;
//    NGD m_NGD;
//    QPtrList<DataSet> m_DataSets;
//};

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
