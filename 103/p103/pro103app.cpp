#include "pro103app.h"
#include "comm/promgrapp.h"
#include "comm/pnet103app.h"
#include <QCoreApplication>
#include <QtDebug>
#include "sys/sysapp.h"
#include "pro103appprivate.h"

Pro103App::Pro103App(QObject *parent) :
    ProBase(parent)
{
    g_103p;
    ProMgrApp::GetInstance()
            ->AddPro("pro_103",this);
}

Pro103App* Pro103App::m_instance=0;

Pro103App* Pro103App::GetInstance()
{
    if(!m_instance){
        m_instance = new Pro103App(qApp);
    }
    return m_instance;
}

bool Pro103App::Start()
{
    return g_103p->Start();
}

void Pro103App::ReadSetting(quint64 devid,SettingType st,bool edit)
{
    qDebug()<<"开始读取定值...";
    Q_UNUSED(edit);
    EDevice* d = g_103p->FindDevice(devid);
    if(!d){
        qDebug()<<"装置不存在。...";
        EmitSettingResult(devid,false,"规约:装置不存在。");
        return;
    }
    if(!d->HasConnected()){
        qDebug()<<"通信中断。...";
        EmitSettingResult(devid,false,"规约:通信中断。");
        return;
    }
    qDebug()<<"ReadSetting...";
    d->ReadSetting(st);
}

void Pro103App::SendControl(DbObjectPtr obj, ControlType ty, const QVariant &val, uint check)
{
    DbObjectPtr dev = obj->RelationOne("device");
    if(!dev){
        EmitControlResult(false,"相关装置没找到");
        return;
    }
    EDevice* d = g_103p->FindDevice(dev->GetId());
    if(!d){
        EmitControlResult(false,"相关装置模型没找到");
        return;
    }
    if(!d->HasConnected()){
        EmitControlResult(false,"规约:通信中断。");
        return;
    }
    d->DoControl(obj,ty,val,check);
}

void Pro103App::ResetSignal(quint64 devid)
{
    EDevice* d = g_103p->FindDevice(devid);
    if(!d){
        return;
    }
    if(!d->HasConnected()){
        return;
    }
    d->ResetSignal();
}

void Pro103App::SetZoon(quint64 devid, bool edit, const QVariant &val)
{
    Q_UNUSED(edit);
    EDevice* d = g_103p->FindDevice(devid);
    if(!d){
        EmitSettingResult(devid,false,"规约:装置不存在。");
        return;
    }
    if(!d->HasConnected()){
        EmitSettingResult(devid,false,"规约:通信中断。");
        return;
    }
    d->SetZoon(val.toUInt());
}

void Pro103App::WriteSetting(quint64 devid,const QVariant& list)
{
    EDevice* d = g_103p->FindDevice(devid);
    if(!d){
        EmitSettingResult(devid,false,"规约:装置不存在。");
        return;
    }
    if(!d->HasConnected()){
        EmitSettingResult(devid,false,"规约:通信中断。");
        return;
    }
    d->WriteSetting(list);
}

void Pro103App::ReadGrounding(quint64 devid)
{
    EDevice* d = g_103p->FindDevice(devid);
    if(!d){
        EmitGroundingResult(devid,false,"规约:装置不存在。");
        return;
    }
    if(!d->HasConnected()){
        EmitGroundingResult(devid,false,"规约:通信中断。");
        return;
    }
    d->ReadGrounding();
}

void Pro103App::JumpGrounding(quint64 devid,ControlType ty)
{
    EDevice* d = g_103p->FindDevice(devid);
    if(!d){
        EmitGroundingResult(devid,false,"规约:装置不存在。");
        return;
    }
    if(!d->HasConnected()){
        EmitGroundingResult(devid,false,"规约:通信中断。");
        return;
    }
    d->JumpGrounding(ty);
}


