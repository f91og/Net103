#ifndef PRO103APP_H
#define PRO103APP_H
#include "../promgr/probase.h"
#include "comm/p103.h"

class P103_EXPORT Pro103App : public ProBase
{
    Q_OBJECT
public:
    explicit Pro103App(QObject *parent = 0);
    static Pro103App* GetInstance();
    bool Start();

    virtual void ReadSetting(quint64 devid,SettingType st,bool edit);
    virtual void WriteSetting(quint64 devid,const QVariant& list);
    virtual void SetZoon(quint64 devid, bool edit, const QVariant &val);
    virtual void SendControl(DbObjectPtr obj, ControlType ty, const QVariant &val, uint check=0);
    virtual void ResetSignal(quint64 devid);

    //{小电流选线
    virtual void ReadGrounding(quint64 devid);
    virtual void JumpGrounding(quint64 devid,ControlType ty);
    //}小电流选线
private:
    static Pro103App* m_instance;
};

#endif // PRO103APP_H
