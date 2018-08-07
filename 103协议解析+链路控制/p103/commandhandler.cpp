#include "commandhandler.h"
#include "edevice.h"
#include "pro103app.h"
#include "protocol_define.h"
#include <QVariant>
#include "deviceprotocol.h"
#include <QtDebug>
#include "comm/promonitorapp.h"
#include "pro103appprivate.h"

CommandHandler::CommandHandler(EDevice *parent) :
    QObject(parent)
{
    m_device = parent;
}

void CommandHandler::ReadErrorHandle(const QString &reason)
{
    Q_UNUSED(reason);
}

void CommandHandler::WriteErrorHandle(const QString &reason)
{
    Q_UNUSED(reason);
}

void CommandHandler::WriteSuccessHandle()
{

}

void CommandHandler::RecvGenData(const QByteArray &data, int num, int cot)
{
    ushort wLen = 0;
    uchar* pHead = (uchar* )data.data();
    for(uchar i = 0; i<num; i++){
        uchar* pCurRec = (uchar* )pHead;
        ushort wDataSize;
        if(pCurRec[GENDATAREC_GDD] == 2)// 成组位串
            wDataSize = ((pCurRec[GENDATAREC_GDD+GDDTYPE_SIZ]+7)/8) * pCurRec[GENDATAREC_GDD+GDDTYPE_NUM] + 6;
        else wDataSize = pCurRec[GENDATAREC_GDD+GDDTYPE_SIZ] * pCurRec[GENDATAREC_GDD+GDDTYPE_NUM] + 6;
        pHead += wDataSize;
        wLen += wDataSize;
        if(wLen > data.size()){
            return;
        }
        QByteArray ba(wDataSize,0);
        memcpy(ba.data(), pCurRec, wDataSize);
        GenDataHandle(ba,cot);
    }
}

////////////////////////////////////////////////


GroundingHandler::GroundingHandler(EDevice *parent)
    :CommandHandler(parent)
{
    m_busy=false;
}

void GroundingHandler::Read()
{
    if(m_busy){
        GroundResultIn(false,"规约：服务忙",true);
        return;
    }
    m_busy=true;
    m_resp.clear();

    QList<int> gos = m_device->GetGroupNo(enumGroupTypeRelayJD);
    if(gos.isEmpty()){
        ReadErrorHandle("装置没有接地数据。");
        return;
    }
    m_device->GetProtocal()
            ->ReadGroup(gos.at(0),
                        KOD_ACTUALVALUE,
                        this
                        );
}

void GroundingHandler::Jump(ProBase::ControlType ct)
{
    if(m_busy){
        GroundResultIn(false,"规约：服务忙",true);
        return;
    }
    m_busy=true;
    m_device->GetProtocal()
            ->GroundingJump(ct,this);
}

void GroundingHandler::GenDataHandle(const QByteArray& data,int cot)
{
    Q_UNUSED(cot);
    uchar byInf = data[GENDATAREC_GIN+1];
    if(byInf==0){
        return;
    }
    uchar kod = data[GENDATAREC_KOD];
    if(kod!= KOD_ACTUALVALUE){
        return;
    }
    uchar byType = data[GENDATAREC_GDD];
    uchar byNum = data[GENDATAREC_GDD + GDDTYPE_NUM];
    uchar bySize = data[GENDATAREC_GDD + GDDTYPE_SIZ];
    uchar* GID = (uchar*)data.data()+GENDATAREC_GID;
    QVariant val = DeviceProtocol::GDDValue(GID,byType,bySize,byNum);

    QString msg = QString("类型:%1,大小:%2,个数:%3,值:%4")
            .arg(byType)
            .arg(bySize)
            .arg(byNum)
            .arg(val.toString());

    ProMonitorApp::GetInstance()
            ->AddMonitor(ProIEC103,MsgInfo,
                         m_device->GetAddrString(),
                         msg);

    switch (byInf) {
    case 1:
        m_resp["i0_dir"]=val;
        break;
    case 2:
        m_resp["i5_dir"]=val;
        break;
    case 3:
        m_resp["i0_current"]=val;
        break;
    case 4:
        m_resp["i5_current"]=val;
        break;
    default:
        break;
    }
}

void GroundingHandler::RecvGenData(const QByteArray& data,int num,int cot)
{
    CommandHandler::RecvGenData(data,num,cot);
    SendResult();
}

void GroundingHandler::ReadErrorHandle(const QString& reason)
{
    GroundResult(false,
                 reason,
                 true);
}

void GroundingHandler::GroundResult(bool result, const QString &reason, bool isRead, const QVariant &resp)
{
    GroundResultIn(result,reason,isRead,resp);
    m_busy=false;
}


void GroundingHandler::GroundResultIn(bool result, const QString &reason, bool isRead, const QVariant &resp)
{
    QVariantMap map = resp.toMap();
    map["dev_id"] = m_device->m_obj->GetId();
    map["is_read"]=isRead;
    Pro103App::GetInstance()
            ->EmitGroundingResult(
                                m_device->m_obj->GetId(),
                                result,
                                reason,
                                map);
}

void GroundingHandler::SendResult()
{
    if(m_resp.count()==4){
        GroundResult(true,
                     "读取接地数据成功",
                     true,
                     m_resp);
    }
    else{
        GroundResult(false,
                     QString("读取数据失败,读到数据个数:%1")
                     .arg(m_resp.count()),
                     true);

    }
}

void GroundingHandler::WriteErrorHandle(const QString& reason)
{
    GroundResult(false,
                 reason,
                 false);
}

void GroundingHandler::WriteSuccessHandle()
{
    GroundResult(true,
                 "成功",
                 false);
}


////////////////////////////////////////////////

SettingHandler::SettingHandler(EDevice *parent)
    :CommandHandler(parent)
{
    m_busy=false;
}

void SettingHandler::WriteSetting(const QVariant& list)
{
    if(m_busy){
        SettingResultIn(false,"规约：服务忙",false);
        return;
    }
    m_busy=true;
    m_device->GetProtocal()
            ->GenWrite(list.toList(),this);
}

void SettingHandler::ReadSetting(ProBase::SettingType st)
{
    if(m_busy){
        SettingResultIn(false,"规约：服务忙",true);
        return;
    }
    m_busy=true;
    ReadSettingSetting();//读装置定值
//    if(st == ProBase::SettingSetting){
//        ReadSettingSetting();//读装置定值
//    }
//    else if(st == ProBase::SettingParam){
//        ReadSettingParam();//读装置参数
//    }
//    else{
//        ReadSettingDescription();//读装置描述
//    }
}

int SettingHandler::GetSettingValue(const QString &key)
{
    for(int i=0;i<m_lstSettingValue.count();i++){
        const QVariantMap& map = m_lstSettingValue.at(i).toMap();
        if(map.value("reference").toString() == key){
            return i;
        }
    }
    QVariantMap map;
    map["reference"] = key;
    m_lstSettingValue.append(map);
    return m_lstSettingValue.count()-1;
}

void SettingHandler::RecvGenData(const QByteArray& data,int num,int cot)
{
    CommandHandler::RecvGenData(data,num,cot);
    if(!m_lstReadCmd.isEmpty()){
        DoRead();
    }
    else{
        SendResult();
    }
}

void SettingHandler::SetZoon(uint val)
{
    qDebug()<<"切换定值区";
    if(m_busy){
        SettingResultIn(false,"规约：服务忙",false);
        return;
    }
    m_busy=true;
    //发Asdu21读取运行定值区号
//    CAsdu21 a21;
//    a21.m_Addr=m_device->GetDevAddr();
//    a21.m_RII=0x08;
//    a21.m_INF=0xf4;
//    a21.m_NOG=0x01;
//    a21.m_DataSets.clear();
//    DataSet *pDataSet=new DataSet;
//    pDataSet->gin.GROUP=0x00;
//    pDataSet->gin.GROUP=0x03;
//    pDataSet->kod=0x01;
//    a21.m_DataSets.append(*pDataSet);
//    QByteArray sData;
//    a21.BuildArray(sData);
//    m_device->GetProtocal()->SendAsdu(sData);
//    delete pDataSet;

    //用asdu10带确认的写运行值区号
    QVariantList list;
    QVariantMap map;
    //map["reference"]=m_settingAreaKey;
    map["reference"]="GIN_00_03";
    map["value"]=val;
    list.append(map);
    m_device->GetProtocal()
            ->GenWrite(list,this);
}

void SettingHandler::WriteSuccessHandle()
{
    SettingResult(true,
                  "成功",
                  false);
}

void SettingHandler::ReadErrorHandle(const QString &reason)
{
    SettingResult(false,
                  reason,
                  true);
}

void SettingHandler::WriteErrorHandle(const QString& reason)
{
    SettingResult(false,
                  reason,
                  false);
}

void SettingHandler::GenDataHandle(const QByteArray& data,int cot)
{
    Q_UNUSED(cot);
    uchar byGroup = data[GENDATAREC_GIN];
    uchar byInf = data[GENDATAREC_GIN+1];
    if(byInf==0){
        return;
    }
    uchar kod = data[GENDATAREC_KOD];
    uchar byType = data[GENDATAREC_GDD];
    uchar byNum = data[GENDATAREC_GDD + GDDTYPE_NUM];
    uchar bySize = data[GENDATAREC_GDD + GDDTYPE_SIZ];
    uchar* GID = (uchar*)data.data()+GENDATAREC_GID;
    QString key = QString("GIN_%1_%2")
            .arg(byGroup)
            .arg(byInf);
    if(kod == KOD_ACTUALVALUE){
        DataType dat;
        dat.m_type = byType;
        dat.m_num = byNum;
        dat.m_size = bySize;
        m_device->GetProtocal()->AddDataType(key,dat);
    }

    //将运行定值区加入到map中
    DataType dat;
    dat.m_num = 1;
    dat.m_type = 3;
    dat.m_size=1;
    m_device->GetProtocal()->AddDataType("GIN_00_03",dat);

    QVariant val = DeviceProtocol::GDDValue(GID,byType,bySize,byNum);
    int groupType= m_device->GetGroupType(byGroup);
    switch (groupType) {
    case enumGroupTypeSettingArea:
    {
        if(kod == KOD_ACTUALVALUE){
            m_sg["EditSG"] = val;
            m_sg["ActSG"] = val;
        }
        else if(kod == KOD_RANGE){
            QVariantList list = val.toList();
            if(list.count()>=2){
                m_sg["NumOfSG"] = list[1];
                m_sg["minSG"] = list[0];
            }
        }
        m_settingAreaKey = key;
    }
        break;
    default:
    {
        int pos = GetSettingValue(key);
        QVariantMap map = m_lstSettingValue.at(pos).toMap();
        if(kod==KOD_ACTUALVALUE){
            map["value"] = val;
        }
        else if(kod==KOD_DESCRIPTION){
            map["name"] = val;
        }
        else if(kod==KOD_RANGE){
            QVariantList list = val.toList();
            if(list.count()>=3){
                QVariantMap cfg = map.value("cfg").toMap();
                cfg["minVal"] = list.at(0);
                cfg["maxVal"] = list.at(1);
                cfg["stepSize"] = list.at(2);
                map["cfg"]=cfg;
            }
        }
        else if(kod == KOD_DIMENSION){
            //qDebug()<<"KOD_DIMENSION:"<<val;
            QVariantMap cfg = map.value("cfg").toMap();
            cfg["dimension"] = val;
            map["cfg"] = cfg;
        }
        else if(kod == KOD_PRECISION){
            //qDebug()<<"KOD_PRECISION:"<<val;
            QVariantMap cfg = map.value("cfg").toMap();
            cfg["prec"] = val;
            map["cfg"] = cfg;
        }
        m_lstSettingValue[pos]=map;
    }
        break;
    }
}

void SettingHandler::ReadSettingDescription()
{
    m_sg.clear();
    m_lstSettingValue.clear();
    m_lstReadCmd.clear();
    foreach (int no, m_device->GetGroupNo(enumGroupTypeDevDescription)) {
        AddGroupToCommand(no);
    }
    if(m_lstReadCmd.isEmpty()){
        SettingResult(false,
                      "无描述组",
                      true);
        return;
    }
    DoRead();
}

void SettingHandler::ReadSettingParam()
{
    m_sg.clear();
    m_lstSettingValue.clear();
    m_lstReadCmd.clear();
    foreach (int no, m_device->GetGroupNo(enumGroupTypeDevParam)) {
        AddGroupToCommand(no);
    }
    if(m_lstReadCmd.isEmpty()){
        SettingResult(false,
                      "无参数组",
                      true);
        return;
    }
    DoRead();
}

void SettingHandler::ReadSettingSetting()
{
    m_sg.clear();
    m_lstSettingValue.clear();
    m_lstReadCmd.clear();
    QVariantMap map;
    AddGroupToCommand(0x03);
    map["no"]=0x0e;
    map["readType"]=KOD_ACTUALVALUE;
    m_lstReadCmd.append(map);
    map["readType"] = KOD_DESCRIPTION;
    m_lstReadCmd.append(map);
    qDebug()<<"m_lstReadCmd:"<<m_lstReadCmd;
    DoRead();
}

void SettingHandler::SettingResult(bool result, const QString &reason, bool isRead, const QVariant &resp)
{
    SettingResultIn(result,reason,isRead,resp);
    m_busy=false;
}

void SettingHandler::SettingResultIn(bool result, const QString &reason, bool isRead, const QVariant &resp)
{
    QVariantMap map = resp.toMap();
    map["is_read"]=isRead;
    Pro103App::GetInstance()
            ->EmitSettingResult(
                m_device->m_obj->GetId(),
                result,
                reason,
                map);
}

void SettingHandler::AddGroupToCommand(int no)
{
    QVariantMap map;
    map["no"]=no;
    QList<int> kods;
    kods<<KOD_DESCRIPTION;
    kods<<KOD_RANGE;
    kods<<KOD_ACTUALVALUE;
    kods<<KOD_PRECISION;
    kods<<KOD_DIMENSION;
    foreach (int kod, kods) {
        map["readType"] = kod;
        m_lstReadCmd.append(map);
    }
}

void SettingHandler::SendResult()
{
    QVariantMap map;
    map["sg"]=m_sg;
    map["list"]=m_lstSettingValue;
    SettingResult(true,
                  "成功",
                  true,
                  map);
    m_lstSettingValue.clear();
}

void SettingHandler::DoRead()
{
    if(m_lstReadCmd.isEmpty()){
        return;
    }
    QVariantMap map = m_lstReadCmd.at(0).toMap();
    m_lstReadCmd.pop_front();
    m_device->GetProtocal()
            ->ReadGroup(map["no"].toInt(),
                        map["readType"].toInt(),
                        this
                        );
}

////////////////////////////////////////////////////

P103ControlStatusObserver::P103ControlStatusObserver(ControlHander *parent)
    :RtObserver(parent)
{
    m_controlHandler=parent;
}

void P103ControlStatusObserver::HandleRtData(RtField &rf, const QVariant &old)
{
    //qDebug()<<"变位";
    Q_UNUSED(old);
    DbAttr* a = rf.GetAttr();
    if(a->GetName()=="u_value"){
        m_controlHandler->StatusChange( rf.GetValue().toInt());
    }
}

////////////////////////////////////////////////////
ControlHander::ControlHander(EDevice *parent)
    :CommandHandler(parent)
{
    m_device = parent;
    m_controlType=-1;
    m_waiteStatusTimer = new QTimer(this);
    m_waiteStatusTimer->setSingleShot(true);
    m_waiteStatusTimer->setInterval(Pro103App::GetInstance()
                                    ->GetWaitStatusTimeOut()*1000);
    connect(m_waiteStatusTimer,
            SIGNAL(timeout()),
            this,
            SLOT(TimerOut())
            );
}

void ControlHander::GenDataHandle(const QByteArray &data, int cot)
{
    Q_UNUSED(data);
    Q_UNUSED(cot);
}

void ControlHander::WriteSuccessHandle()
{
    if(m_controlType== ProBase::ControlExec){
        if(!m_lstStatus.isEmpty()){
            if(m_statusObserver){
                delete m_statusObserver;
                m_statusObserver=0;
            }
            m_statusObserver = new P103ControlStatusObserver(this);
            foreach (DbObjectPtr st, m_lstStatus) {
                //qDebug()<<"相关遥信"<<st->Get("pathname");
                RtDb::GetInstance()->AddObserver(st,m_statusObserver);
            }
            m_waiteStatusTimer->start();
            return;
        }
    }
    Clear();
    QString msg;
    switch (m_controlType) {
    case ProBase::ControlExec:
        msg="执行成功";
        break;
    case ProBase::ControlSelect:
        msg="选择成功";
        break;
    case ProBase::ControlCancel:
        msg="取消成功";
        break;
    default:
        msg="成功";
        break;
    }
    Pro103App::GetInstance()
            ->EmitControlResult(true,msg);
}

void ControlHander::WriteErrorHandle(const QString &reason)
{
    Clear();
    Pro103App::GetInstance()
            ->EmitControlResult(false,reason);
}

void ControlHander::StatusChange(int value)
{
    //qDebug()<<value<<"---"<<m_value;
    if( (value&0x1) == (m_value.toInt()&0x1) ){
        Pro103App::GetInstance()
                ->EmitControlResult(true,"遥控成功。");
        Clear();
    }
}

void ControlHander::TimerOut()
{
    Clear();
    Pro103App::GetInstance()
            ->EmitControlResult(false,
                                "等待遥信变位超时");
}

void ControlHander::DoControl(DbObjectPtr obj, ProBase::ControlType ct, const QVariant &val, uint check)
{
    if(m_obj){
        Pro103App::GetInstance()
                ->EmitControlResult(false,"规约：装置遥控冲突。");
        return;
    }
    m_obj = obj;
    m_value = val;

    m_lstStatus = g_103p->GetStatusListForControl(m_obj);

    m_controlType=ct;
    m_device->GetProtocal()
            ->DoControl(obj,ct,val,check,this);
}

void ControlHander::Clear()
{
    m_obj=0;
    m_lstStatus.clear();
    if(m_statusObserver){
        delete m_statusObserver;
        m_statusObserver=0;
    }
    m_waiteStatusTimer->stop();
}
