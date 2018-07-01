#include "pro103appprivate.h"
#include "sys/sysapp.h"
#include <QtDebug>
#include "comm/pnet103app.h"
#include "dbobject/dbsession.h"
#include <QCoreApplication>
#include "comm/promonitorapp.h"
#include "tool/toolapi.h"
#include "scada/scadaobject.h"
#include <QHostInfo>
#include <QSettings>
#include "dbmeta/dbmetadefine.h"

Pro103AppPrivate* Pro103AppPrivate::m_instance=0;

Pro103AppPrivate* Pro103AppPrivate::GetInstance()
{
    if(!m_instance){
        m_instance = new Pro103AppPrivate(qApp);
    }
    return m_instance;
}

Pro103AppPrivate::Pro103AppPrivate(QObject *parent) :
    QObject(parent)
{
    PNet103App* net103 = PNet103App::GetInstance();

    connect(net103,
            SIGNAL(DeviceChange(ushort,ushort,bool)),
            this,
            SLOT(SlotDeviceChange(ushort,ushort,bool))
            );

    connect(net103,
            SIGNAL(NetStateChange(ushort,ushort,uchar,uchar)),
            this,
            SLOT(SlotNetStateChange(ushort,ushort,uchar,uchar))
            );

    connect(net103,
            SIGNAL(RecvASDU(ushort,ushort,QByteArray)),
            this,
            SLOT(SlotRecvASDU(ushort,ushort,QByteArray))
            );

    m_gbk = QTextCodec::codecForName("GBK");

    QSettings st("../etc/p103.ini",QSettings::IniFormat);
    st.beginGroup("Period");
    m_periodGeneral=st.value("general",600).toUInt();
    m_periodPulse=st.value("pulse",60).toUInt();
    m_periodRelayMeasure=st.value("relay_measure",5).toUInt();
    m_periodSync = st.value("sync_period",307).toInt();
    st.endGroup();
    qDebug()<<"总查询周期："<<m_periodGeneral
            <<"遥脉查询周期："<<m_periodPulse
            <<"保护遥测查询周期"<<m_periodRelayMeasure
            <<"对时周期："<<m_periodSync;

    QList<DbObjectPtr> list = DbSession::GetInstance()
            ->Query(dbc_Pro103Setting::_class_name);
    if(!list.isEmpty()){
        m_setting=list.at(0);
    }
    if(m_setting){
        m_periodGeneral = m_setting->Get(dbc_ProSetting::gi_period).toUInt()*60;
        m_periodPulse = m_setting->Get(dbc_Pro103Setting::pulse_period).toUInt()*60;
        m_periodRelayMeasure = m_setting->Get(dbc_Pro103Setting::bhyc_period).toUInt();
    }
}

void Pro103AppPrivate::SlotDeviceChange(ushort sta,ushort dev,bool add)
{
    EDevice* d = FindDevice(sta,dev);
    if(d){
        d->DeviceChange(add);
    }
}

void Pro103AppPrivate::SlotRecvASDU(ushort sta,ushort dev, const QByteArray& data)
{
    ProMonitorApp::GetInstance()
            ->AddMonitor(ProIEC103,
                         MsgRecv,
                         QString("s%1d%2").arg(sta).arg(dev),
                         QString(),
                         data);
    EDevice* d = FindDevice(sta,dev);
    if(d){
        d->RecvASDU(data);
    }
}

void Pro103AppPrivate::AddControlStatusMap(DbObjectPtr sta, DbObjectPtr ctl)
{
    m_mapControlToStatus[ctl->GetId()].append(sta);
}

QList<DbObjectPtr> Pro103AppPrivate::GetStatusListForControl(DbObjectPtr ctl)
{
    return m_mapControlToStatus.value(ctl->GetId(),QList<DbObjectPtr>());
}

void Pro103AppPrivate::timerEvent(QTimerEvent *)
{
    foreach (EDevice* d, m_lstDevice) {
        d->OnTimer();
    }
}

void Pro103AppPrivate::SlotNetStateChange(ushort sta,ushort dev,uchar net, uchar state)
{
    qDebug()<<"SlotNetStateChange"<<"sta"<<sta<<"dev"<<dev<<"net"<<net<<"state"<<state;
    EDevice* d = FindDevice(sta,dev);
    if(d){
        d->NetState(net,state);
    }
}

EDevice* Pro103AppPrivate::FindDevice(quint64 id)
{
    return m_mapIdDevice.value(id,0);
}

EDevice* Pro103AppPrivate::FindDevice(ushort sta, ushort dev)
{
    uint addr=(((uint)sta)<<16)|dev;
    return m_mapAddrDevice.value(addr,0);
}

bool Pro103AppPrivate::Start()
{
    foreach (EDevice* d, m_lstDevice) {
        delete d;
    }
    m_lstDevice.clear();
    m_mapAddrDevice.clear();
    m_mapIdDevice.clear();
    ProMonitorApp::GetInstance()->Init();


    ushort localAddr = 0xfe01;
    if(!SysApp::GetInstance()->IsMain()){
        localAddr++;
    }

    QList<DbObjectPtr> nodes = DbSession::GetInstance()
            ->Query("SysNode",QString("name='%1'")
                    .arg(QHostInfo::localHostName()));
    if(nodes.count()>0){
        DbObjectPtr node = nodes.at(0);
        uint addr = node->Get("addr").toUInt();
        if(addr!=0){
            localAddr= addr;
        }
    }

    PNet103App::GetInstance()
            ->SetLocalAddr(localAddr);
    QVariantList list;
    foreach (DbObjectPtr dev, ScadaObject::GetInstance()
             ->DeviceList()) {
        if(dev->Get("protype").toInt()!=2){
            continue;
        }
        EDevice* ed = new EDevice(this);
        if(!ed->SetObj(dev)){
            delete ed;
        }
        else{
            m_lstDevice.append(ed);
        }
        DbObjectPtr sta = dev->RelationOne("station");
        if(!sta){
            continue;
        }
        uint staAddr = sta->Get("addr").toUInt();
        QString server = dev->Get("server").toString();
        bool ok;
        uint devAddr = server.toUInt(&ok);
        if(!ok){
            continue;
        }
        quint32 addr=(staAddr<<16)|devAddr;
        m_mapAddrDevice[addr]=ed;
        m_mapIdDevice[dev->GetId()]=ed;
        //qDebug()<<"103:"<<dev->Get("pathname");
        QVariantMap map;
        QVariantList ips;
        QString ipa = dev->Get("ipa").toString();
        if(!ipa.isEmpty()){
            ips +=ipa;
        }
        QString ipb = dev->Get("ipb").toString();
        if(!ipb.isEmpty()){
            ips +=ipb;
        }
        if(ips.isEmpty()){
            continue;
        }
        map["staAddr"]=staAddr;
        map["devAddr"]=devAddr;
        map["ips"]=ips;
        list.append(map);
    }
    startTimer(1000);
    PNet103App::GetInstance()->SetDeviceList(list);
    return true;
}
