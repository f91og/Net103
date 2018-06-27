#include "pnet103app.h"
#include <QCoreApplication>
#include "clientcenter.h"

class PNet103AppPrivate
{
public:
    ClientCenter* m_clientCenter;
};

PNet103App* PNet103App::m_instance=0;

PNet103App::PNet103App(QObject *parent) :
    QObject(parent)
{
    d = new PNet103AppPrivate;
    d->m_clientCenter = new ClientCenter(this);
}

PNet103App::~PNet103App()
{
    delete d;
}

PNet103App* PNet103App::GetInstance()
{
    if(!m_instance){
        m_instance = new PNet103App(qApp);
    }
    return m_instance;
}

void PNet103App::SetDeviceList(const QVariantList& list)
{
    d->m_clientCenter->SetDeviceList(list);
}

void PNet103App::EmitDeviceChange(ushort sta,ushort dev,bool add)
{
    emit DeviceChange(sta,dev,add);
}

void PNet103App::EmitRecvASDU(ushort sta,ushort dev, const QByteArray& data)
{
    emit RecvASDU(sta,dev,data);
}

void PNet103App::EmitNetStateChange(ushort sta,ushort dev,uchar net, uchar state)
{
    emit NetStateChange(sta,dev,net,state);
}

void PNet103App::SendAppData(ushort sta, ushort dev, const QByteArray &data)
{
    d->m_clientCenter->SendAppData(sta,dev,data);
}

void PNet103App::SetLocalAddr(ushort addr)
{
    d->m_clientCenter->SetLocalAddr(addr);
}
