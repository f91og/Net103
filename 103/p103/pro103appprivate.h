#ifndef PRO103APPPRIVATE_H
#define PRO103APPPRIVATE_H

#include <QObject>
#include "edevice.h"
#include <QTextCodec>
#include "dbobject/dbsession.h"

#define g_103p (Pro103AppPrivate::GetInstance())

class Pro103AppPrivate : public QObject
{
    Q_OBJECT
public:
    explicit Pro103AppPrivate(QObject *parent = 0);
    bool Start();

    EDevice* FindDevice(ushort sta,ushort dev);
    EDevice* FindDevice(quint64 id);
    static Pro103AppPrivate* GetInstance();

    void AddControlStatusMap(DbObjectPtr sta,DbObjectPtr ctl);
    QList<DbObjectPtr> GetStatusListForControl(DbObjectPtr ctl);

protected:
    void timerEvent(QTimerEvent *);
private slots:
    void SlotDeviceChange(ushort sta,ushort dev,bool add);
    void SlotRecvASDU(ushort sta,ushort dev, const QByteArray& data);
    void SlotNetStateChange(ushort sta,ushort dev,uchar net, uchar state);
public:
    QList<EDevice*> m_lstDevice;
    QTextCodec* m_gbk;
    QMap<quint64,QList<DbObjectPtr> > m_mapControlToStatus;
    QMap<uint,EDevice*> m_mapAddrDevice;
    QMap<quint64,EDevice*> m_mapIdDevice;
    int m_periodPulse;
    int m_periodGeneral;
    int m_periodRelayMeasure;

    DbObjectPtr m_setting;
private:
    static Pro103AppPrivate* m_instance;
};

#endif // PRO103APPPRIVATE_H
