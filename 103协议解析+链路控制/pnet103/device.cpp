#include "device.h"
#include "clientcenter.h"
#include "gateway.h"
#include "comm/pnet103app.h"
#include <QSettings>

Device::Device(ClientCenter *parent) :
    QObject(parent)
{
    m_stationAddr=0;
    m_devAddr=0;
    m_clientCenter = parent;

    QSettings setting("../etc/p103.ini",QSettings::IniFormat);
    setting.beginGroup("p103");
    link_t1 = setting.value("link_t1",60).toUInt();
    setting.endGroup();
    for(int i=0;i<2;i++){
        m_state[i]=false;
        m_testTimer[i]=link_t1;
    }
}

void Device::AddGateWay(GateWay *gw)
{
    if(!m_lstGateWay.contains(gw)){
        m_lstGateWay.append(gw);
    }
}

bool Device::HasConnect()
{
    for(int i=0;i<2;i++){
        if(m_state[i]){
            return true;
        }
    }
    return false;
}

void Device::OnTimer()
{
    for(int i=0;i<2;i++){
        OnTimer(i);
    }
}

void Device::SendAppData(const QByteArray& data)
{
    NetPacket np;
    np.SetDestAddr(m_stationAddr,m_devAddr);
    np.AddAppData(data);
    foreach (GateWay* g, m_lstGateWay) {
        g->SendAppData(np);
    }
}

void Device::Test(int index)
{
    NetPacket np;
    np.SetDestAddr(m_stationAddr,m_devAddr);

    foreach (GateWay* g, m_lstGateWay) {
        g->SendPacket(np,index);
    }
}

void Device::OnTimer(int index)
{
    if(!m_state[index]){
        return;
    }
    if(m_testTimer[index]>0){
        m_testTimer[index]--;
    }
    if(m_testTimer[index]==0){
        NetState(index,0);
    }
}

void Device::NetState(int index, bool conn)
{
    if(index!=0){
        index=1;
    }
    if(conn){
        m_testTimer[index]=link_t1;
    }
    if(m_state[index]!=conn){
        PNet103App::GetInstance()
            ->EmitNetStateChange(m_stationAddr,
                                 m_devAddr,
                                 index,
                                 conn);
    }
    bool hasConnect0 = HasConnect();
    m_state[index]=conn;
    bool hasConnect1 = HasConnect();
    if(hasConnect0 && !hasConnect1){
        PNet103App::GetInstance()
                ->EmitDeviceChange(m_stationAddr,
                                   m_devAddr,
                                   false);
    }

    if(!hasConnect0 && hasConnect1){
        PNet103App::GetInstance()
                ->EmitDeviceChange(m_stationAddr,
                                   m_devAddr,
                                   true);
    }
}
