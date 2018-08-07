#ifndef EDEVICE_H
#define EDEVICE_H

#include <QObject>
#include "dbobject/dbsession.h"
#include "deviceprotocol.h"
#include <QMap>
#include "dbobject/rtdb.h"
#include <QDateTime>
#include <QVariant>
#include "commandhandler.h"
#include "comm/probase.h"

namespace ns_p103{
class EDevice;
class MeasPointInfo
{
public:
    MeasPointInfo(EDevice* dev);
    void SetObject(DbObjectPtr obj);

    void SetValue(const QVariant& val,
                  bool change=false,
                  const QDateTime& time=QDateTime());
public:
    QString m_key;
    RtField m_o_value;
    RtField m_refresh_time;
    RtField m_refresh_time_msec;
    RtField m_quality;
    RtField m_u_value;
    RtField m_is_change;
    DbObjectPtr m_obj;
    EDevice* m_device;
    bool m_valueInted;
};

class BakupChangeData
{
public:
    BakupChangeData();
public:
    QDateTime m_time;
    QVariant m_value;
    QDateTime m_changeTime;
    MeasPointInfo* m_var;
};

class EDevice : public QObject
{
    Q_OBJECT
public:
    explicit EDevice(QObject *parent = 0);
    bool SetObj(DbObjectPtr obj);
    ushort GetStationAddr();
    ushort GetDevAddr();

    void NetState(int index, bool conn);
    void DeviceChange(bool add);
    void RecvASDU(const QByteArray& data);

    void SetGroupType(int no,int type);
    int GetGroupType(int no);
    QList<int> GetGroupNo(int type);

    MeasPointInfo* GetMeasPointInfo(const QString& key);
    void AddToBakupChange(MeasPointInfo* var,
                          const QVariant& value,
                          const QDateTime chgTime);

    void SetNetState();
    void OnTimer();

    QString GetAddrString();

    DeviceProtocol* GetProtocal();

    void ReadSetting(ProBase::SettingType st);
    void WriteSetting(const QVariant& list);

    bool HasConnected();
    void SetZoon(uint val);

    void ResetSignal();

    void DoControl(DbObjectPtr obj,ProBase::ControlType ct,const QVariant& val,uint check);

    void ReadGrounding();
    void JumpGrounding(ProBase::ControlType ct);
    void SetCommState();

private slots:
    void DutyChange(bool v);
public:
    DbObjectPtr m_obj;
private:
    ushort m_staAddr;
    ushort m_devAddr;

    DeviceProtocol* m_protocal;
    QMap<int,int> m_mapGroup;
    QMap<QString,MeasPointInfo*> m_mapMeasPoint;
    QList<RtField> m_lstMeasState;
    MeasPointInfo* m_net0;
    MeasPointInfo* m_net1;
    bool m_netstat0;
    bool m_netstat1;
    QList<BakupChangeData> m_lstBakupChange;

    SettingHandler* m_settingHandler;
    ControlHander* m_controlHandler;
    GroundingHandler* m_groundHandler;
};
}
using namespace ns_p103;
#endif // EDEVICE_H
