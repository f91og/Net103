#include "gencommand.h"
#include "deviceprotocol.h"
#include <QtDebug>
#include "protocol_define.h"
#include "pro103appprivate.h"
#include "tool/toolapi.h"
#include "comm/promonitorapp.h"
#include "commandhandler.h"
#include "sys/sysapp.h"

GenCommand::GenCommand(DeviceProtocol *parent) :
    QObject(parent)
{
    m_protocal=parent;
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_RII = m_protocal->GetRIIAndInc() ;
    connect(m_timer,
            SIGNAL(timeout()),
            this,
            SLOT(TimerOut())
            );
    m_handler=0;
}

GenCommand::~GenCommand()
{
    m_timer->stop();
}

void GenCommand::StartWait(int sec)
{
    m_timer->stop();
    m_timer->setInterval(sec*1000);
    m_timer->start();
}

void GenCommand::SetHandler(CommandHandler* h)
{
    m_handler =h;
}

void GenCommand::TimerOut()
{

}

///////////////////////////////////////////////

ReadCatalogueCommand::ReadCatalogueCommand(DeviceProtocol *parent)
    :GenCommand(parent)
{
    m_count=0;
}

void ReadCatalogueCommand::RecvData(const QByteArray &data, uchar cot)
{
    qDebug()<<"receive catalog data";
    ProMonitorApp* mon = ProMonitorApp::GetInstance();
    if(cot!=COT_GRCR_VALID){
        mon->AddMonitor(ProIEC103,
                        MsgError,
                        m_protocal->m_device->GetAddrString(),
                        "读目录失败");
        this->deleteLater();
        return;
    }
    qDebug()<<"receive catalog data2";
    m_recvData+= data.mid(4);
    uchar byNGD = data[GENDATACOM_NGD];
    m_count+= (data[GENDATACOM_NGD] & 0x3f);
    //qDebug()<<m_count;
    if((byNGD & 0x80)!=0){
        qDebug()<<"receive catalog data3";
        return;
    }
    mon->AddMonitor(ProIEC103,MsgInfo,
                    m_protocal->m_device->GetAddrString(),
                    QString("读目录返回:组个数%1")
                    .arg(m_count));
    uchar* pHead = (uchar*)m_recvData.data();
    ushort wCalLen=0;
    qDebug()<<"读目录返回个数："<<m_count;
    for(int i=0; i<m_count; i++){
        ushort wLen;
        if(pHead[GENDATAREC_GDD] == 2)
            wLen = 6 + ((pHead[GENDATAREC_GDD + GDDTYPE_SIZ]+7)/8) * pHead[GENDATAREC_GDD + GDDTYPE_NUM];
        else wLen = 6 + pHead[GENDATAREC_GDD + GDDTYPE_SIZ] * pHead[GENDATAREC_GDD + GDDTYPE_NUM];
        uchar* pData = pHead;
        pHead += wLen;
        wCalLen += wLen;
        if(wCalLen > m_recvData.size() ) break;
        if(pData[GENDATAREC_KOD] != KOD_DESCRIPTION || pData[GENDATAREC_GDD] != 1) continue;
        ushort wDataSize = pData[GENDATAREC_GDD + GDDTYPE_SIZ] * pData[GENDATAREC_GDD + GDDTYPE_NUM];
        QByteArray name_c(wDataSize,0);
        memcpy(name_c.data(), &pData[GENDATAREC_GID], wDataSize);
        QString strName = g_103p->m_gbk->toUnicode(name_c);
        int groupType = JudgGroupType(pData[GENDATAREC_GIN]);
        if(groupType==RETERROR){
            qDebug()<<"组名:"<<strName<<"无效";
            continue;
        }
        uchar byGroup = pData[GENDATAREC_GIN];
        mon->AddMonitor(ProIEC103,MsgInfo,
                        m_protocal->m_device->GetAddrString(),
                        QString("组名:%1,组号:%2")
                        .arg(strName)
                        .arg(byGroup));
        m_protocal->m_device->SetGroupType(byGroup,groupType);
    }
    if(SysApp::GetInstance()->IsDuty()){
        m_protocal->SendGeneralQuery();
        m_protocal->ReadPulse();
    }
    this->deleteLater();
}

void ReadCatalogueCommand::TimerOut()
{
    if(!m_protocal->m_device->HasConnected()){
        this->deleteLater();
    }
    else{
        m_RII = m_protocal->GetRIIAndInc();
        DoCommand();
    }
}

int ReadCatalogueCommand::JudgGroupType(const uchar& group)
{
    if(group == 0x02) return enumGroupTypeSettingArea;//定值区号,应该是通过组号和条目号来确认的
    else if(group == 0x03) return enumGroupTypeSetting;//定值
    else if(group == 0x04) return enumGroupTypeActElement;//动作元件，这个是报告？
    else if(group == 0x05)return enumGroupTypeRunWarn;//运行告警
    else if(group == 0x0e) return enumGroupTypeSoftSwitch;//软压板
    else if(group == 0x40) return enumGroupTypeHardSwitch;//硬压板,没有硬压板
    else if((group == 0x08)||(group == 0x09)) return enumGroupTypeYX;//遥信，但是这里有带时标和不带时标的区别
    else if(group == 0x06) return enumGroupTypeRelayMeasure;//测量
    else if(group == 0x07) return enumGroupTypeYC;//遥测
    else if(group == 0x0a) return enumGroupTypeYM;//遥脉
    else if(group == 0x40) return enumGroupTypeYK;//遥控，这个也找不到对应的组号，遥控开关和遥控分头？
    else if(group == 0x0d) return enumGroupTypeYT;//遥调
    else return RETERROR;
}

void ReadCatalogueCommand::DoCommand()
{
    m_recvData.clear();
    m_count=0;
    QByteArray data(4,0);
    data[GENREQCOM_FUN]= 254;
    data[GENREQCOM_INF]= 240;
    data[GENREQCOM_RII]= m_RII;
    data[GENREQCOM_NOG]= 0;
    m_protocal->SendAsdu21(data);
    StartWait(30);
}



////////////////////////////////////////////////////////////

ReadGroupValueCommand::ReadGroupValueCommand(DeviceProtocol *parent)
    :GenCommand(parent)
{
    m_count=0;
    m_readType=0;
    m_groupNo=-1;
}

void ReadGroupValueCommand::Init(int groupNo,int readType)
{
    m_groupNo=groupNo;
    m_readType = readType;
}

void ReadGroupValueCommand::RecvData(const QByteArray &data, uchar cot)
{
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProIEC103,
                         MsgInfo,
                         m_protocal->m_device->GetAddrString(),
                         QString("读组数据返回"));
    if(cot == COT_GRCR_VALID){
        m_recvData+= data.mid(4);
        m_count+= (data[GENDATACOM_NGD] & 0x3f);
    }
    else{
        if(m_handler){
            m_handler->ReadErrorHandle(QString("读取组属性失败：组号：%1，属性：%2")
                                   .arg(m_groupNo)
                                   .arg(m_readType));
        }
        this->deleteLater();
        return;
    }
    uchar byNGD = data[GENDATACOM_NGD];
    if((byNGD & 0x80)!=0){
        return;
    }
    if(m_handler){
        m_handler->RecvGenData(m_recvData,m_count,cot);
    }
    else{
        m_protocal->RecvGenData(m_recvData,m_count,cot);
    }
    this->deleteLater();
}

void ReadGroupValueCommand::DoCommand()
{
    m_recvData.clear();
    m_count=0;
    QByteArray data(7,0);
    data[GENREQCOM_FUN] = 254;
    data[GENREQCOM_INF] = 241;
    data[GENREQCOM_RII] = m_RII;
    data[GENREQCOM_NOG] = 1;
    uchar* GIR = (uchar*)data.data()+GENREQCOM_GIR;
    GIR[GENID_GIN] = m_groupNo;
    GIR[GENID_GIN+1] = 0;
    GIR[GENID_KOD] = m_readType;
    m_protocal->SendAsdu21(data);
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProIEC103,
                         MsgInfo,
                         m_protocal->m_device->GetAddrString(),
                         QString("读组内数据,组号:%1,KOD:%2")
                         .arg(m_groupNo)
                         .arg(m_readType) );

    StartWait(30);
}

void ReadGroupValueCommand::TimerOut()
{
    if(m_handler){
        m_handler->ReadErrorHandle(QString("读取组%1超时")
                               .arg(m_groupNo));
    }
    this->deleteLater();
}

/////////////////////////////////////////////

GenWriteCommand::GenWriteCommand(DeviceProtocol *parent)
    :GenCommand(parent)
{
    m_countBit=false;
}

void GenWriteCommand::Init(const QVariantList &list)
{
    m_list = list;
}

void GenWriteCommand::RecvData(const QByteArray &data, uchar cot)
{
    uchar inf = data[GENDATACOM_INF];
    if(cot == 41){
        if(m_handler){
            QString reason;
            if(inf == 249){
                reason = "写值失败。";
            }
            else{
                reason = "确认失败。";
            }
            m_handler->WriteErrorHandle(reason);
        }
        this->deleteLater();
        return;
    }
    if(inf==250){
        if(m_handler){
            m_handler->WriteSuccessHandle();
        }
        this->deleteLater();
        return;
    }
    if(data!=m_writeBuf){
        if(m_handler){
            m_handler->WriteErrorHandle("写值反校失败。");
        }
        this->deleteLater();
        return;
    }
    if(m_list.isEmpty()){
        SendAffirm();
    }
    else{
        SendWrite();
    }
    StartWait(20);
}

void GenWriteCommand::SendAffirm()
{
    //对于定值的的发送带执行的写条目要单独处理量的
    uchar byGroup=m_writeBuf[4];
    if(m_writeBuf.size()>4 && (byGroup==0x00||byGroup==0x03||byGroup==0x0e)){
        QByteArray settingAffirm;
        settingAffirm.append(m_writeBuf);
        settingAffirm[GENDATACOM_FUN] = 254;
        settingAffirm[GENDATACOM_INF] = 250;
        settingAffirm[GENDATACOM_RII] = m_RII;
        m_protocal->SendAsdu10(settingAffirm);
        return;
    }

    QByteArray head(4,0);
    head[GENDATACOM_FUN] = 254;
    head[GENDATACOM_INF] = 250;
    head[GENDATACOM_RII] = m_RII;
    uchar ngd=0;
    if(m_countBit){
       ngd |= 0x40;
    }
    m_countBit=!m_countBit;
    head[GENDATACOM_NGD] = ngd;
    m_protocal->SendAsdu10(head);
}

void GenWriteCommand::SendWrite()
{
    QByteArray head(4,0);
    head[GENDATACOM_FUN] = 254;
    head[GENDATACOM_INF] = 249;
    head[GENDATACOM_RII] = m_RII;
    QByteArray data;
    int max_count=60;
    int count=0;
    while(!m_list.isEmpty()) {
        QVariant v = m_list.at(0);
        m_list.pop_front();
        QVariantMap map = v.toMap();
        QString key = map.value("reference").toString();
        DataType dat = m_protocal->GetDataType(key);
        if(!dat.IsValid()){
            qDebug()<<"写值数据类型没找到："<<key;
            continue;
        }
        qDebug()<<"key"<<key<<"map.value"<<map.value("value");
        data+= m_protocal->ValueToGDD(dat,key,map.value("value"));
        count++;
        if(count==max_count){
            break;
        }
    }
    uchar ngd = count;
    if(m_countBit){
        ngd |= 0x40;
    }
    m_countBit=!m_countBit;
    if(!m_list.isEmpty()){
        ngd |= 0x80;
    }
    head[GENDATACOM_NGD] = ngd;
    m_writeBuf = head+data;
    m_protocal->SendAsdu10(m_writeBuf);
}

void GenWriteCommand::DoCommand()
{
    m_countBit=false;
    SendWrite();
    StartWait(20);
}

void GenWriteCommand::TimerOut()
{
    if(m_handler){
        m_handler->WriteErrorHandle(QString("写值超时"));
    }
    this->deleteLater();
}

/////////////////////////////////////////////

GroundJumpCommand::GroundJumpCommand(DeviceProtocol *parent)
    :GenCommand(parent)
{
    m_controlType=-1;
}

void GroundJumpCommand::Init(ProBase::ControlType ct)
{
    m_controlType=ct;
}

void GroundJumpCommand::RecvData(const QByteArray &data, uchar cot)
{
    if(cot==41){
        QString reason="接地试跳操作失败";
        if(data.size()>17){
            uchar* GDR = (uchar*)data.data()+GENDATACOM_GDR;
            if(GDR[GENDATAREC_GDD] == 23 && GDR[GENDATAREC_GDD+1]>7){
                uchar* GID = GDR+GENDATAREC_GID;//10
                if(GID[4]==1){
                    uchar len = GID[5];
                    if(len!=0 && GID[6]==1 && (len+17)<=data.size() ){
                        QByteArray msg(len,0);
                        memcpy(msg.data(),GID+7,len);
                        reason = g_103p->m_gbk->toUnicode(msg);
                    }
                }
            }
        }
        if(m_handler){
            m_handler->WriteErrorHandle(reason);
        }
    }
    else{
        if(data!=m_sendBuf){
            if(m_handler){
                m_handler->WriteErrorHandle("接地试跳反校失败");
            }
        }
        else{
            if(m_handler){
                m_handler->WriteSuccessHandle();;
            }
        }
    }
    this->deleteLater();
}

void GroundJumpCommand::DoCommand()
{
    QByteArray sndBuf;
    uchar uc=254;
    sndBuf.append(uc);
    if(m_controlType==ProBase::ControlSelect){
        uc=249;
    }
    else if(m_controlType==ProBase::ControlExec){
        uc=250;
    }
    else if(m_controlType==ProBase::ControlCancel){
        uc=251;
    }
    else{
        if(m_handler){
            m_handler->WriteErrorHandle("接地试跳类型错误");
        }
        return;
    }
    sndBuf.append(uc);
    sndBuf.append(m_RII);
    uc=1;// NOG;
    sndBuf.append(uc);
    QList<int> gos = m_protocal->m_device
            ->GetGroupNo(enumGroupTypeRelayJDST);
    if(gos.isEmpty()){
        if(m_handler){
            m_handler->WriteErrorHandle("接地试跳组不存在");
        }
        return;
    }
    uchar groupNo = gos.at(0);
    uchar inf  = 1;
    sndBuf.append(groupNo);
    sndBuf.append(inf);
    uc = KOD_ACTUALVALUE; //KOD
    sndBuf.append(uc);
    //GDD
    uchar num=1;
    uchar type=9;
    uchar size=1;
    uchar byVal=DCO_OFF;
    sndBuf.append(type);
    sndBuf.append(size);
    sndBuf.append(num);
    sndBuf.append(byVal);
    m_protocal->SendAsdu10(sndBuf);
    m_sendBuf = sndBuf;

    if(m_controlType==ProBase::ControlCancel){
        if(m_handler){
            m_handler->WriteSuccessHandle();
        }
        this->deleteLater();
    }
    else{
        StartWait( Pro103App::GetInstance()
                   ->GetWriteTimeOut());
    }
}

void GroundJumpCommand::TimerOut()
{
    if(m_handler){
        if(m_controlType==ProBase::ControlSelect){
            m_handler->WriteErrorHandle("接地试跳选择操作超时。");
        }
        else if(m_controlType==ProBase::ControlExec){
            m_handler->WriteErrorHandle("接地试跳执行操作超时。");
        }
        else{
            m_handler->WriteErrorHandle("接地试跳取消操作超时。");
        }
    }
    this->deleteLater();
}


/////////////////////////////////////////////

ControlCommand::ControlCommand(DeviceProtocol *parent)
    :GenCommand(parent)
{
    m_controlType=-1;
    m_check=0;
}

void ControlCommand::Init(DbObjectPtr obj, ProBase::ControlType ct, const QVariant &val, uint check)
{
    m_obj=obj;
    m_check=check;
    m_value = val;
    m_controlType=ct;
}

void ControlCommand::DoCommand()
{
    if(!m_obj){
        if(m_handler){
            m_handler->WriteErrorHandle("遥控对象不存在");
        }
        return;
    }
    QByteArray sndBuf;
    uchar uc=254;
    sndBuf.append(uc);
    if(m_controlType==ProBase::ControlSelect){
        uc=249;
    }
    else if(m_controlType==ProBase::ControlExec){
        uc=250;
    }
    else if(m_controlType==ProBase::ControlCancel){
        uc=251;
    }
    else{
        if(m_handler){
            m_handler->WriteErrorHandle("遥控类型错误");
        }
        return;
    }
    sndBuf.append(uc);
    sndBuf.append(m_RII);
    uc=1;// NOG;
    sndBuf.append(uc);
    QString key = m_obj->Get("reference").toString();
    uchar groupNo = key.section("_",1,1).toUInt();
    uchar inf  = key.section("_",2,2).toUInt();
    //sndBuf.append(groupNo);
    sndBuf.append(groupNo);
    sndBuf.append(inf);

    uc = KOD_ACTUALVALUE; //KOD
    sndBuf.append(uc);
    //GDD
    uchar num=1;
    uchar type;
    uchar size;
    QString ctlType = m_obj->GetEnum("type");
    QByteArray valBuf;
    if(ctlType=="val"){
        type=7;
        size=4;
        valBuf = QByteArray(4,0);
        float f_value = m_value.toDouble();
        ToolNumberToByte((uchar*)valBuf.data(),f_value);
    }
    else if(ctlType=="sta"){
        size=1;
        uchar byVal=0;
        if(m_check& ProBase::CheckTQ){
            type=206;
            uint uval = m_value.toUInt();
            if(uval==1){
                byVal = (DCO_ON<<3);
            }
            else{
                byVal = (DCO_OFF<<3);
            }
            byVal|=1;
            if(m_controlType==ProBase::ControlSelect){
                byVal|=0x80;
            }
            else if(m_controlType==ProBase::ControlCancel){
                byVal|=0x40;
            }
        }
        else{
            type=9;
            uint uval = m_value.toUInt();
            if(uval==1){
                byVal = DCO_ON;
            }
            else{
                byVal = DCO_OFF;
            }
        }
        valBuf.resize(1);
        valBuf[0]=byVal;
    }
    else{
        if(m_handler){
            m_handler->WriteErrorHandle(QString("103规约不支持该遥控类型:%1")
                                   .arg(m_obj->GetEnumDesc("type")));
        }
        return;
    }
    sndBuf.append(type);
    sndBuf.append(size);
    sndBuf.append(num);
    sndBuf.append(valBuf);

    m_protocal->SendAsdu10(sndBuf);
    m_sendBuf = sndBuf;

    if(m_controlType==ProBase::ControlCancel){
        if(m_handler){
            m_handler->WriteSuccessHandle();
        }
        this->deleteLater();
    }
    else{
        StartWait( Pro103App::GetInstance()
                   ->GetWriteTimeOut());
    }
}

void ControlCommand::RecvData(const QByteArray &data, uchar cot)
{
    if(cot==41){
        QString reason="遥控操作失败";
        if(data.size()>17){
            uchar* GDR = (uchar*)data.data()+GENDATACOM_GDR;
            if(GDR[GENDATAREC_GDD] == 23 && GDR[GENDATAREC_GDD+1]>7){
                uchar* GID = GDR+GENDATAREC_GID;//10
                if(GID[4]==1){
                    uchar len = GID[5];
                    if(len!=0 && GID[6]==1 && (len+17)<=data.size() ){
                        QByteArray msg(len,0);
                        memcpy(msg.data(),GID+7,len);
                        reason = g_103p->m_gbk->toUnicode(msg);
                    }
                }
            }
        }
        if(m_handler){
            m_handler->WriteErrorHandle(reason);
        }
    }
    else{
        qDebug()<<"cot:"<<cot<<" data:"<<data.toHex();
        //信号复归的收和发的报文不是完全一致的
        if(cot==0x28&&data[4]==0x00&&data[5]==0x06){
            if(m_handler){
                m_handler->WriteSuccessHandle();;
            }
        }
        else if(data!=m_sendBuf){
            if(m_handler){
                m_handler->WriteErrorHandle("遥控反校失败");
            }
        }
        else{
            if(m_handler){
                m_handler->WriteSuccessHandle();;
            }
        }
    }
    this->deleteLater();
}

void ControlCommand::TimerOut()
{
    if(m_handler){
        if(m_controlType==ProBase::ControlSelect){
            m_handler->WriteErrorHandle("遥控选择操作超时。");
        }
        else if(m_controlType==ProBase::ControlExec){
            m_handler->WriteErrorHandle("遥控执行操作超时。");
        }
        else{
            m_handler->WriteErrorHandle("遥控取消操作超时。");
        }
    }
    this->deleteLater();
}
