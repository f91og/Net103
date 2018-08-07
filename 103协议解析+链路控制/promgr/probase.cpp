#include "probase.h"
#include "dbobject/dbsession.h"

ProBase::ProBase(QObject *parent) :
    QObject(parent)
{
    QList<DbObjectPtr> list = DbSession::GetInstance()
            ->Query("ControlSetting");
    if(!list.isEmpty()){
        DbObjectPtr cs = list.at(0);
        m_waitStatusTimeOut = cs->Get("wait_status_timeout").toUInt();
        m_writeTimeOut = cs->Get("write_timeout").toUInt();
    }
    else{
        m_waitStatusTimeOut = 40;
        m_writeTimeOut = 15;
    }
}

bool ProBase::Start()
{
    return false;
}

uint ProBase::GetWriteTimeOut()
{
    return m_writeTimeOut;
}

uint ProBase::GetWaitStatusTimeOut()
{
    return m_waitStatusTimeOut;
}

void ProBase::SendControl(DbObjectPtr obj, ControlType ty, const QVariant &val, uint check)
{
    Q_UNUSED(obj);
    Q_UNUSED(ty);
    Q_UNUSED(val);
    Q_UNUSED(check);
    EmitControlResult(false,"没有实现");
}

void ProBase::EmitControlResult(bool result, const QString &reason)
{
    emit ControlResult(result,reason);
}

void ProBase::ReadSetting(quint64 devid, SettingType st, bool edit)
{
    Q_UNUSED(devid);
    Q_UNUSED(st);
    Q_UNUSED(edit);
    EmitSettingResult(devid,false,"没有实现");
}

void ProBase::EmitSettingResult(quint64 devid, bool result, const QString &reason, const QVariant &resp)
{
    emit SettingResult(devid,result,reason,resp);
}

void ProBase::SetZoon(quint64 devid, bool edit, const QVariant &val)
{
    Q_UNUSED(devid);
    Q_UNUSED(edit);
    Q_UNUSED(val);
    EmitSettingResult(devid,false,"没有实现");
}

void ProBase::WriteSetting(quint64 devid, const QVariant &list)
{
    Q_UNUSED(devid);
    Q_UNUSED(list);
    EmitSettingResult(devid,false,"没有实现");
}

void ProBase::ResetSignal(quint64 devid)
{
    Q_UNUSED(devid);
}


void ProBase::ReadGrounding(quint64 devid)
{
    Q_UNUSED(devid);
    EmitGroundingResult(devid,false,"没有实现");
}

void ProBase::JumpGrounding(quint64 devid, ControlType ty)
{
    Q_UNUSED(devid);
    Q_UNUSED(ty);
    EmitGroundingResult(devid,false,"没有实现");
}

void ProBase::EmitGroundingResult(quint64 devid, bool result, const QString &reason, const QVariant &resp)
{
    emit GroundingResult(devid,result,reason,resp);
}
