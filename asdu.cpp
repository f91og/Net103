#include "asdu.h"

CAsdu::CAsdu()
{
    m_ASDUData.resize(0);
    m_TYP=0;
    m_VSQ=0;
    m_COT=0;
    m_Addr=0;
    m_FUN=0;
    m_INF=0;
}

CAsdu::~CAsdu()
{
    m_ASDUData.clear();
    m_ASDUData.resize(0);
}

void CAsdu::SaveAsdu(QByteArray &Data)//将转来的Data解析，放入未知类型的Asdu中
{
    m_TYP=Data[0];
    m_VSQ=Data[1];
    m_COT=Data[2];
    m_Addr=Data[3];
    m_FUN=Data[4];
    m_INF=Data[5];
}

/////////// ASDU07 /////////
CAsdu07::CAsdu07()
{
    m_TYP=0x07;
    m_VSQ=0x81;
    m_COT=0x09;
    m_FUN=0xff;
    m_INF=0x00;
    m_SCN=0xff;
}

void CAsdu07::BuildArray(QByteArray &Data)  //组装asdu到Data中
{
    Data.resize(7);
    Data[0]=m_TYP;
    Data[1]=m_VSQ;
    Data[2]=m_COT;
    Data[3]=m_Addr;
    Data[4]=m_FUN;
    Data[5]=m_INF;
    Data[6]=m_SCN;
}

/////////// ASDU10 /////////
//CAsdu10::CAsdu10()
//{
//    m_TYP=0x0a;
//    m_VSQ=0x81;
//    m_COT=0x28;//通用分类写命令
//    m_FUN=0xfe;
//}

//CAsdu10::~CAsdu10()
//{
//    m_DataSets.clear();
//}

