#include "asdu.h"
#include <QDebug>
#include <QFile>
#include <Qdir>

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

void CAsdu10::ExplainAsdu(int iProcessType) //解析收到的asdu10，将收到的数据封装
{
    if(iProcessType==0)
    {
        m_RII=m_ASDUData[0];
        m_NGD.byte=m_ASDUData[1];
        m_ASDUData=m_ASDUData.mid(2);

        int i=0;
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
                m_ASDUData=m_ASDUData.mid(len);
            }
        }
    }
    QString addr=QString(m_Addr);
    int i_addr=m_Addr;  // 这里uchar转int直接赋值就可以了
    qDebug()<<i_addr;
    QFile file("E:\\Net103\\192.168.0.171-01.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString str="组号  条目  描述类别  数据";
    in<<str<<"\n";
    for(int i=0;i!=m_DataSets.size();i++)
    {
        DataSet data=m_DataSets.at(i);
        QString group;
        int entry=data.gin.ENTRY;
        int kod=data.kod;
        QByteArray gid=data.gid;
        switch (data.gin.GROUP) {
        case 0x03:case 0x02:
            group="定值";
            break;
        case 0x05:
            group="告警";
            break;
        case 0x08:case 0x09:
            group="遥信";
            break;
        case 0x07:
            group="运动测量";
            break;
        default:
            break;
        }
        in<<group<<"   "<<entry<<"     "<<kod<<"   "<<gid<<"\n";
    }
    file.close();
}

