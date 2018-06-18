#include "asdu.h"
#include <QDebug>
#include <QFile>
#include <time.h>

CAsdu::CAsdu()
{
    m_ASDUData.resize(0);
    m_TYP=0;
    m_VSQ=0;
    m_COT=0;
    m_Addr=0;
    m_FUN=0;
    m_INF=0;

    m_iResult=0;
}

CAsdu::~CAsdu()
{
    m_ASDUData.clear();
    m_ASDUData.resize(0);
}

void CAsdu::SaveAsdu(QByteArray &Data)//将转来的Data解析，放入未知类型的Asdu中
{
    if(Data.size()<6)  //如果是损坏的包则不解析传来的包
    {
        m_iResult=0;
        Data.resize(0);
        return;
    }

    m_TYP=Data[0];
    m_VSQ=Data[1];
    m_COT=Data[2];
    m_Addr=Data[3];
    m_FUN=Data[4];
    m_INF=Data[5];
    // 将Data中封装的数据赋给Asdu中的m_ASDUData
    m_ASDUData=Data.mid(6);
    Data.resize(0);
    m_iResult=1;
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

void CAsdu07::BuildArray(QByteArray &Data)  //组装asdu到Data中，要不然socket不能发送我们自己封装的asdu
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
CAsdu10::CAsdu10()
{
    m_TYP=0x0a;
    m_VSQ=0x81;
    m_COT=0x28;//通用分类写命令
    m_FUN=0xfe;
}

CAsdu10::CAsdu10(CAsdu &a)
{
    m_TYP=a.m_TYP;
    m_VSQ=a.m_VSQ;
    m_COT=a.m_COT;
    m_Addr=a.m_Addr;
    m_FUN=a.m_FUN;
    m_INF=a.m_INF;

    m_ASDUData.append(a.m_ASDUData);
    a.m_ASDUData.resize(0);
}

CAsdu10::~CAsdu10()
{
    m_DataSets.clear();
}

void CAsdu10::BuildArray(QByteArray &Data)
{   
    Data.append(m_TYP);
    Data.append(m_VSQ);
    Data.append(m_COT);
    Data.append(m_Addr);
    Data.append(m_FUN);
    Data.append(m_INF);
    Data.append(m_RII);
    Data.append(m_NGD.byte);
    DataSet data=m_DataSets.at(0);
    Data.append(data.gin.GROUP);
    Data.append(data.gin.ENTRY);
    Data.append(data.gid);
}

void CAsdu10::ExplainAsdu(int iProcessType) //解析收到的asdu10，将收到的数据封装
{
    if(iProcessType==0)
    {
        m_RII=m_ASDUData[0];//此处赋值m_RII
        m_NGD.byte=m_ASDUData[1];
        m_ASDUData=m_ASDUData.mid(2);

        DataSet* pDataSet=NULL;
        for(int i=0;i<m_NGD.byte;i++)
        {
            if(m_ASDUData.size()>(int)(6*sizeof(BYTE)))
            {
                pDataSet=new DataSet;
                pDataSet->gin.GROUP=m_ASDUData[0];//填入每一组中组号
                pDataSet->gin.ENTRY=m_ASDUData[1];//填入每一组中的条目号
                pDataSet->kod=m_ASDUData[2];//填入每一组的描述类别kod
                //填入每一组中的通用分类数据秒数gdd
                memcpy(pDataSet->gdd.byte, m_ASDUData.data()+3*sizeof(BYTE), sizeof(pDataSet->gdd.byte));
                //填入通入分类表示数据gid，首先得知道其长度，长度是gdd中的数据宽度乘以gdd中的数目
                pDataSet->gid.resize(pDataSet->gdd.gdd.DataSize*pDataSet->gdd.gdd.Number*sizeof(BYTE));
                //知道gid的长度后便可以填入相应的gid数据
                memcpy(pDataSet->gid.data(), m_ASDUData.data()+3*sizeof(BYTE)+sizeof(pDataSet->gdd.byte), pDataSet->gid.size()*sizeof(BYTE));
                m_DataSets.append(*pDataSet);
                int len=3*sizeof(BYTE)+sizeof(pDataSet->gdd.byte)+pDataSet->gdd.gdd.DataSize*pDataSet->gdd.gdd.Number*sizeof(BYTE);
                m_ASDUData=m_ASDUData.mid(len);//这里的数据截取我总感觉不对
            }
            else
            {
                m_iResult=0;
                break;
            }
        }
    }
}

/////////// ASDU10Link /////////
CAsdu10Link::CAsdu10Link(CAsdu &a)
{
    gin_h.GROUP=0x04;
    gin_h.ENTRY=a.m_ASDUData.at(2);
    reportArgNum=a.m_ASDUData.at(5);
    d1.gid=a.m_ASDUData.mid(7,reportArgNum);
    int start_index=16+reportArgNum;
    a.m_ASDUData=a.m_ASDUData.mid(start_index);
    ASdu10LinkDataSet *pDataSet=NULL;
    for(int i=0;i<reportArgNum*3;i++)
    {
        pDataSet=new ASdu10LinkDataSet;
        pDataSet->kod=a.m_ASDUData.at(0);
        pDataSet->datatype=a.m_ASDUData.at(1);
        pDataSet->datasize=a.m_ASDUData.at(2);
        pDataSet->number=a.m_ASDUData.at(3);
        pDataSet->gid=a.m_ASDUData.mid(4,pDataSet->datasize);
        int len=4*sizeof(BYTE)+sizeof(pDataSet->datasize*pDataSet->number);
        a.m_ASDUData=a.m_ASDUData.mid(len+1);
        m_DataSet.append(*pDataSet);
    }
    a.m_ASDUData.resize(0);
}

CAsdu10Link::~CAsdu10Link()
{
    m_DataSet.clear();
}


/////////// ASDU21 /////////
CAsdu21::CAsdu21()
{
    m_TYP=0x15;
    m_VSQ=0x81;
    m_COT=0x2a;
    m_FUN=0xfe;

    m_DataSets.clear();
}

CAsdu21::~CAsdu21()
{
    m_DataSets.clear();
}

void CAsdu21::BuildArray(QByteArray &Data)
{
    if(m_NOG!=m_DataSets.size())
    {
        m_iResult=0;
        return;
    }
    m_iResult=1;
    Data.resize(0);
    Data.append(m_TYP);
    Data.append(m_VSQ);
    Data.append(m_COT);
    Data.append(m_Addr);
    Data.append(m_FUN);
    Data.append(m_INF);
    Data.append(m_RII);
    Data.append(m_NOG);
    for(int i=0;i<m_NOG;i++)
    {
        Data.append(m_DataSets.at(i).gin.GROUP);
        Data.append(m_DataSets.at(i).gin.ENTRY);
        Data.append(m_DataSets.at(i).kod);
    }
}

CAsdu201::CAsdu201()
{
    m_TYP=201;
    m_VSQ=0x81;
    m_COT=20;
    m_Addr=01;//要改的
    m_FUN=255;
    m_INF=0;
    end_time=time(NULL);
    start_time=end_time-3600*24*30;
}

CAsdu201::CAsdu201(CAsdu &a)
{
    m_TYP=a.m_TYP;
    m_VSQ=a.m_VSQ;
    m_COT=a.m_COT;
    m_Addr=a.m_Addr;
    m_FUN=a.m_FUN;
    m_INF=a.m_INF;
    //start_time和end_time需要处理吗？
    //newValue = (value1<<8) | value2;
    QByteArray tmp=a.m_ASDUData;
    listNum=tmp[10]<<8 | tmp[9];//取值这里终于搞明白了额
    FileAsdu201 *file=NULL;
    QByteArray waveFile=a.m_ASDUData.mid(11);
    for(int i=0;i<listNum;i++)
    {
        file=new FileAsdu201;
        file->m_addr=0x01;
        file->lubo_num=waveFile[2]<<8 | waveFile[1];
        file->file_name=waveFile.mid(3,32);
        file->fault_time=waveFile.mid(35,7);
        waveFile=waveFile.mid(42);
        m_DataSets.append(*file);
    }
    a.m_ASDUData.resize(0);
}

void CAsdu201::BuildArray(QByteArray &Data)
{
    Data.resize(14);
    Data[0]=m_TYP;
    Data[1]=m_VSQ;
    Data[2]=m_COT;
    Data[3]=m_Addr;
    Data[4]=m_FUN;
    Data[5]=m_INF;
    memcpy(Data.data()+6,&start_time,4);//data()方法取地址
    memcpy(Data.data()+10,&end_time,4);
}

CAsdu200::CAsdu200()
{
    m_TYP=200;
    m_VSQ=0x81;
    m_COT=20;
    m_Addr=01;
    m_FUN=255;
    m_INF=0;
}

CAsdu200::CAsdu200(CAsdu &a)
{
    m_TYP=a.m_TYP;
    m_VSQ=a.m_VSQ;
    m_COT=a.m_COT;
    m_Addr=a.m_Addr;
    m_FUN=a.m_FUN;
    m_INF=a.m_INF;

    offset=a.m_ASDUData.mid(6,4);
    followTag=a.m_ASDUData.at(10);
    file_name=a.m_ASDUData.mid(7,32);
    all_packet.append(m_TYP);
    all_packet.append(m_VSQ);
    all_packet.append(m_COT);
    all_packet.append(m_Addr);
    all_packet.append(m_FUN);
    all_packet.append(m_INF);
    all_packet.append(a.m_ASDUData);
    a.m_ASDUData.resize(0);
}

void CAsdu200::BuildArray(QByteArray &Data)
{
    Data.resize(0);
    Data.append(m_TYP);
    Data.append(m_VSQ);
    Data.append(m_COT);
    Data.append(m_Addr);
    Data.append(m_FUN);
    Data.append(m_INF);
    Data.append(file_name);
}

