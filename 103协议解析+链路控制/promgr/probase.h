#ifndef PROBASE_H
#define PROBASE_H

#include <QObject>
#include "comm/promgr.h"
#include "dbobject/dbsession.h"
#include <QVariant>

class PROMGR_EXPORT ProBase : public QObject
{
    Q_OBJECT
public:
    enum ControlType
    {
        ControlSelect,
        ControlExec,
        ControlCancel
    };

    enum CheckType{
        CheckNormal=0x00,
        CheckTQ=0x01,
        CheckLock=0x02,
        CheckWY=0x04,
        CheckNo=0x08
    };

    enum SettingType{
        SettingSetting,
        SettingParam,
        SettingDescription
    };

public:
    explicit ProBase(QObject *parent = 0);
    virtual bool Start();


    // {遥控
public:
    uint GetWriteTimeOut();
    uint GetWaitStatusTimeOut();
    virtual void SendControl(DbObjectPtr obj,ControlType ty,const QVariant& val,uint check=0);
    virtual void EmitControlResult(bool result,const QString& reason);
signals:
    void ControlResult(bool result,const QString& reason);
    //} 遥控

    //{定值
public:
    virtual void ReadSetting(quint64 devid,SettingType st,bool edit);
    virtual void EmitSettingResult(quint64 devid,bool result,const QString& reason,const QVariant& resp=QVariant());

    virtual void SetZoon(quint64 devid,bool edit,const QVariant& val);
    virtual void WriteSetting(quint64 devid,const QVariant& list);
signals:
    void SettingResult(quint64 devid,bool result,const QString& reason,const QVariant& resp);
    //}定值

    //{复归
public:
    virtual void ResetSignal(quint64 devid);
    //}复归

    //{小电流选线
public:
    virtual void ReadGrounding(quint64 devid);
    virtual void JumpGrounding(quint64 devid,ControlType ty);
    virtual void EmitGroundingResult(quint64 devid,
                                     bool result,
                                     const QString& reason,
                                     const QVariant& resp=QVariant());
signals:
    void GroundingResult(quint64 devid,
                         bool result,
                         const QString& reason,
                         const QVariant& resp);
    //}小电流选线

private:
    uint m_writeTimeOut;
    uint m_waitStatusTimeOut;
};

#endif // PROBASE_H
