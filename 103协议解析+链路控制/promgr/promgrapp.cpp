#include "promgrapp.h"
#include <QCoreApplication>

ProMgrApp::ProMgrApp(QObject *parent) :
    QObject(parent)
{
}

ProMgrApp* ProMgrApp::m_instance=0;

ProMgrApp* ProMgrApp::GetInstance()
{
    if(!m_instance){
        m_instance = new ProMgrApp(qApp);
    }
    return m_instance;
}

void ProMgrApp::AddPro(const QString &name, ProBase *pro)
{
    m_mapPro[name]=pro;
}

ProBase* ProMgrApp::GetPro(const QString &name)
{
    return m_mapPro.value(name,0);
}

QList<ProBase*> ProMgrApp::GetProList()
{
    return m_mapPro.values();
}
