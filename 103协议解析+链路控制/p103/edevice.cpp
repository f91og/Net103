#include "edevice.h"
#include <QVariant>
#include "pro103appprivate.h"
#include <QDateTime>
#include "sys/sysapp.h"
#include "comm/p103app.h"
#include <QtDebug>
#include "dbmeta/dbmetadefine.h"

MeasPointInfo::MeasPointInfo(EDevice *dev)
{
    m_device=dev;
    m_valueInted=false;
}

void MeasPointInfo::SetObject(DbObjectPtr obj)
{
    m_obj = obj;
    RtObject rto;
    if(!rto.Attach(obj)){
        return;
    }
    m_key = obj->Get("reference").toString();
    m_o_value = rto.GetRtField("o_value");
    m_refresh_time=rto.GetRtField("refresh_time");
    m_refresh_time_msec = rto.GetRtField("refresh_time_msec");
    m_quality = rto.GetRtField("quality");
    if(obj->GetDbClass()->GetName()=="Status"){
        m_u_value = rto.GetRtField("u_value");
        m_is_change = rto.GetRtField("is_change");
    }
}

void MeasPointInfo::SetValue(const QVariant &val, bool change, const QDateTime &time)
{
    QDateTime dt = time;
    if(dt.isNull()){
        dt = QDateTime::currentDateTime();
    }

    if(!SysApp::GetInstance()->IsDuty()){
        if(change){
            //qDebug()<<"------------";
            m_device->AddToBakupChange(this,
                                       val,
                                       dt);
        }
        return;
    }

    m_refresh_time.SetValue(dt);
    m_refresh_time_msec.SetValue(dt.time().msec());
    if(m_valueInted){
        m_o_value.SetValue(val,RtField::ChangeNotify);
    }
    else{
        m_o_value.SetValue(val,RtField::AlwaysNotify);
    }
    if(change){
       m_is_change.SetValue(m_u_value.GetValue(),
                            RtField::AlwaysNotify);
    }
    m_valueInted=true;
}
///////////////////////////////////////////////

BakupChangeData::BakupChangeData()
{
    m_var=0;
}

///////////////////////////////////////////////
EDevice::EDevice(QObject *parent) :
    QObject(parent)
{
    m_staAddr=0;
    m_devAddr=0;

    m_protocal = new DeviceProtocol(this);
    m_net0=0;
    m_net1=0;

    m_netstat0=false;
    m_netstat1=false;

    connect(SysApp::GetInstance(),SIGNAL(DutyChange(bool)),
            this,SLOT(DutyChange(bool))
            );

    m_settingHandler = new SettingHandler(this);
    m_controlHandler = new ControlHander(this);
    m_groundHandler = new GroundingHandler(this);
}

bool EDevice::SetObj(DbObjectPtr obj)
{
    m_obj=obj;
    DbObjectPtr sta = obj->RelationOne("station");
    if(!sta){
        return false;
    }
    m_staAddr = sta->Get("addr").toUInt();
    QString s = obj->Get("server").toString();
    bool ok;
    m_devAddr = s.toUInt(&ok);
    DbSession* ss = DbSession::GetInstance();
    foreach (DbObjectPtr mp, m_obj->RelationMany("meas_points")) {
        MeasPointInfo* mpi = new MeasPointInfo(this);
        mpi->SetObject(mp);
        if(mp->GetDbClass()->GetName()=="Status"){
            quint64 ctl_id = mp->Get("ctl_id").toULongLong();
            if(ctl_id!=0){
                //m_mapCtlToStatus[ctl_id]=mp;
                DbObjectPtr ctl = ss->GetObject("ControlPoint",ctl_id);
                if(ctl){
                    g_103p->AddControlStatusMap(mp,ctl);
                }
            }
        }
        if(mpi->m_key.isEmpty()){
            delete mpi;
        }
        else{
            m_mapMeasPoint[mpi->m_key]=mpi;
            if(mpi->m_key=="netstat0"){
                m_net0=mpi;
            }
            else if(mpi->m_key=="netstat1"){
                m_net1=mpi;
            }
            else{
                RtObject rto;
                if(rto.Attach(mp)){
                    m_lstMeasState.append( rto.GetRtField(dbc_MeasPoint::state_flag));
                }
            }
        }
    }

    return ok;
}

ushort EDevice::GetStationAddr()
{
    return m_staAddr;
}

ushort EDevice::GetDevAddr()
{
    return m_devAddr;
}

void EDevice::NetState(int index, bool conn)
{
    bool oc = HasConnected();
    MeasPointInfo* mpi=0;
    if(index==0){
        mpi=m_net0;
        m_netstat0=conn;
    }
    else{
        mpi=m_net1;
        m_netstat1=conn;
    }
    if(SysApp::GetInstance()->IsDuty()){
        if(mpi){
            mpi->SetValue(conn,false);
        }
        if(oc!=HasConnected()){
            qDebug()<<"测点通信状态变化";
            SetCommState();
        }
    }
    if(!HasConnected()){
        m_lstBakupChange.clear();
    }

}

void EDevice::DeviceChange(bool add)
{
    if(add){
        qDebug()<<"read catalog";
        m_protocal->ReadCatalog();
    }
}

void EDevice::RecvASDU(const QByteArray& data)
{
    m_protocal->AnaData(data);
}

void EDevice::SetGroupType(int no,int type)
{
    m_mapGroup[no]=type;
}

int EDevice::GetGroupType(int no)
{
    return m_mapGroup.value(no,-1);
}

QList<int> EDevice::GetGroupNo(int type)
{
    QList<int> nos;
    QMap<int,int>::Iterator i;
    for(i=m_mapGroup.begin();i!=m_mapGroup.end();i++){
        if(i.value() == type){
            nos.append(i.key());
        }
    }
    return nos;
}

MeasPointInfo* EDevice::GetMeasPointInfo(const QString &key)
{
    return m_mapMeasPoint.value(key,0);
}

void EDevice::OnTimer()
{
    if(HasConnected()){
        m_protocal->OnTimer();
    }
}

void EDevice::ReadSetting(ProBase::SettingType st)
{
    m_settingHandler->ReadSetting(st);
}

bool EDevice::HasConnected()
{
    return m_netstat0||m_netstat1;
}

void EDevice::SetZoon(uint val)
{
    m_settingHandler->SetZoon(val);
}

void EDevice::ResetSignal()
{
    m_protocal->ResetSignal();
}

void EDevice::DoControl(DbObjectPtr obj, ProBase::ControlType ct, const QVariant &val, uint check)
{
    m_controlHandler->DoControl(obj,
                                ct,
                                val,
                                check);
}

void EDevice::ReadGrounding()
{
    m_groundHandler->Read();
}

void EDevice::SetCommState()
{
    for(int i=0;i<m_lstMeasState.count();i++){
        RtField& rf=m_lstMeasState[i];
        rf.SetBitM(dbb_ScadaStateFlag::v_commerr,
                   !HasConnected(),
                   RtField::ChangeNotify);
    }
}

void EDevice::JumpGrounding(ProBase::ControlType ct)
{
    m_groundHandler->Jump(ct);
}

void EDevice::WriteSetting(const QVariant &list)
{
    m_settingHandler->WriteSetting(list);
}

DeviceProtocol* EDevice::GetProtocal()
{
    return m_protocal;
}

QString EDevice::GetAddrString()
{
    return QString("s%1d%2")
            .arg(m_staAddr)
            .arg(m_devAddr);
}

void EDevice::SetNetState()
{
    if(m_net0){
       m_net0->SetValue(m_netstat0);
    }
    if(m_net1){
       m_net1->SetValue(m_netstat1);
    }
}

void EDevice::DutyChange(bool v)
{
    //qDebug()<<"--------------------";
    QDateTime now = QDateTime::currentDateTime();
    if(v){
        while(!m_lstBakupChange.isEmpty()){
            BakupChangeData& front = m_lstBakupChange.front();
            if(front.m_time.secsTo(now)<=10){
                front.m_var->SetValue(front.m_value,true,front.m_changeTime);
            }
            m_lstBakupChange.pop_front();
        }
        SetNetState();
        if(HasConnected()){
            m_protocal->SendGeneralQuery();
            m_protocal->ReadPulse();
            m_protocal->ReadRelayMeasure();
        }
        SetCommState();
    }
}

void EDevice::AddToBakupChange(MeasPointInfo *var, const QVariant &value, const QDateTime chgTime)
{
    BakupChangeData bcd;
    bcd.m_time = QDateTime::currentDateTime();
    bcd.m_value=value;
    bcd.m_var=var;
    bcd.m_changeTime = chgTime;
    m_lstBakupChange.append(bcd);
    while(!m_lstBakupChange.isEmpty()){
        BakupChangeData& front = m_lstBakupChange.front();
        if(front.m_time.secsTo(bcd.m_time)>10){
            m_lstBakupChange.pop_front();
        }
        else{
            break;
        }
    }
}
