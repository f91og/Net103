#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <QObject>
#include "comm/probase.h"
#include "pro103app.h"
#include "dbobject/dbsession.h"
#include "dbobject/rtdb.h"
#include <QTimer>
namespace ns_p103{
class EDevice;
class CommandHandler : public QObject
{
    Q_OBJECT
public:
    explicit CommandHandler(EDevice *parent = 0);
    virtual void RecvGenData(const QByteArray& data,int num,int cot);
    virtual void GenDataHandle(const QByteArray& data,int cot)=0;
    virtual void WriteErrorHandle(const QString& reason);
    virtual void WriteSuccessHandle();
    virtual void ReadErrorHandle(const QString& reason);
public:
    EDevice* m_device;
};

class GroundingHandler: public CommandHandler
{
    Q_OBJECT
public:
    GroundingHandler(EDevice *parent = 0);
    void Read();
    void Jump(ProBase::ControlType ct);
    virtual void GenDataHandle(const QByteArray& data,int cot);
    virtual void WriteErrorHandle(const QString& reason);
    virtual void WriteSuccessHandle();
    virtual void RecvGenData(const QByteArray& data,int num,int cot);
    virtual void ReadErrorHandle(const QString& reason);

private:
    void GroundResult(bool result,
                      const QString& reason,
                      bool isRead,
                      const QVariant& resp=QVariant());

    void GroundResultIn(bool result,
                      const QString& reason,
                      bool isRead,
                      const QVariant& resp=QVariant());
private:
    void SendResult();
private:
    QVariantMap m_resp;
    bool m_busy;
};

class SettingHandler: public CommandHandler
{
    Q_OBJECT
public:
    SettingHandler(EDevice *parent = 0);
    virtual void GenDataHandle(const QByteArray& data,int cot);
    void ReadSetting(ProBase::SettingType st);
    void WriteSetting(const QVariant& list);
    virtual void RecvGenData(const QByteArray& data,int num,int cot);
    virtual void WriteErrorHandle(const QString& reason);
    int GetSettingValue(const QString& key);
    virtual void WriteSuccessHandle();
    virtual void ReadErrorHandle(const QString &reason);

    void SetZoon(uint val);
private:
    void ReadSettingSetting();
    void ReadSettingParam();
    void ReadSettingDescription();
    void SendResult();
    void DoRead();
    void AddGroupToCommand(int no);

    void SettingResultIn(bool result,
                       const QString& reason,
                       bool isRead,
                       const QVariant& resp=QVariant());

    void SettingResult(bool result,
                       const QString& reason,
                       bool isRead,
                       const QVariant& resp=QVariant());
public:
    QVariantList m_lstReadCmd;
    QVariantMap m_sg;
    QVariantList m_lstSettingValue;
    QString m_settingAreaKey;
    bool m_busy;
};


class ControlHander;
class P103ControlStatusObserver: public RtObserver
{
    Q_OBJECT
public:
    P103ControlStatusObserver(ControlHander* parent);
    virtual void HandleRtData(RtField &rf, const QVariant &old);
public:
    ControlHander* m_controlHandler;
};

class ControlHander : public CommandHandler
{
    Q_OBJECT
public:
    ControlHander(EDevice *parent = 0);
    virtual void GenDataHandle(const QByteArray& data,int cot);
    virtual void WriteErrorHandle(const QString& reason);
    virtual void WriteSuccessHandle();
    void DoControl(DbObjectPtr obj,ProBase::ControlType ct, const QVariant& val, uint check );

    void StatusChange(int value);
private slots:
    void TimerOut();
private:
    void Clear();
public:
    DbObjectPtr m_obj;
    QVariant m_value;
    QList<DbObjectPtr> m_lstStatus;
    int m_controlType;
    QPointer<P103ControlStatusObserver> m_statusObserver;
    QTimer* m_waiteStatusTimer;
    EDevice* m_device;
};
}

using namespace ns_p103;

#endif // COMMANDHANDLER_H
