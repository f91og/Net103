#ifndef PROMGRAPP_H
#define PROMGRAPP_H

#include <QObject>
#include "comm/promgr.h"
#include "comm/probase.h"
#include <QMap>

class PROMGR_EXPORT ProMgrApp : public QObject
{
    Q_OBJECT
public:
    explicit ProMgrApp(QObject *parent = 0);
    static ProMgrApp* GetInstance();
    void AddPro(const QString& name,
                     ProBase* pro);

    ProBase* GetPro(const QString& name);
    QList<ProBase*> GetProList();
private:
    QMap<QString,ProBase*> m_mapPro;
    static ProMgrApp* m_instance;
};

#endif // PROMGRAPP_H
