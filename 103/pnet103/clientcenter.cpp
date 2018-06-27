#include "clientcenter.h"
#include <QSettings>

ClientCenter::ClientCenter(QObject *parent) :
    QObject(parent)
{
    QSettings setting("../etc/p103.ini",QSettings::IniFormat);
    setting.beginGroup("p103");
    m_remotePort = setting.value("port",1032).toUInt();
    m_longNumber = setting.value("long_number",0).toUInt()!=0;
    if(m_longNumber){
        qDebug()<<"使用长报文编号。";
    }
    else{
        qDebug()<<"使用短报文编号。";
    }
    setting.endGroup();
	server = new QTcpServer();
 //   m_localAddr= 0xfe01;
    startTimer(1000);
}

ushort ClientCenter::GetRemotePort()
{
    return m_remotePort;
}

void ClientCenter::SetLocalAddr(ushort addr)
{
    m_localAddr=addr;
}

ushort ClientCenter::GetLocalAddr()
{
    return m_localAddr;
}

bool ClientCenter::IsLongNumber()
{
    return m_longNumber;
}

void ClientCenter::SendAppData(ushort sta,ushort dev, const QByteArray& data)
{
    foreach (Device* d, m_lstDevice) {
        if( (sta==0xff || sta == d->m_stationAddr ) &&
                (dev==0xffff || dev == d->m_devAddr ) ){
            d->SendAppData(data);
        }
    }
}

Device* ClientCenter::GetDevice(ushort sta,ushort dev)
{
    quint32 addr = (((quint32)sta)<<16)+dev;
    Device* d = m_mapDevice.value(addr,0);
    if(d){
        return d;
    }
    d = new Device(this);
    d->m_stationAddr= sta;
    d->m_devAddr=dev;
    m_lstDevice.append(d);
    m_mapDevice[addr]=d;
    return d;
}

void ClientCenter::Clear()
{
    while(m_lstGateWay.count()>0){
        delete m_lstGateWay.takeFirst();
    }
    while(m_lstDevice.count()>0){
        delete m_lstDevice.takeFirst();
    }
    m_mapDevice.clear();
}

void ClientCenter::SetDeviceList(const QVariantList& list)
{
    foreach (QVariant v, list) {
        QVariantMap map = v.toMap();
        QVariantList ips = map.value("ips").toList();
        ushort sta = map.value("staAddr").toUInt();
        ushort dev = map.value("devAddr").toUInt();
        if(ips.isEmpty()){
            continue;
        }
        QStringList sips;
        foreach (QVariant ip, ips) {
            QString sip = ip.toString();
            if(!sip.isEmpty()){
                sips+=sip;
            }
        }
        if(sips.isEmpty()){
            continue;
        }
        GateWay* gw = new GateWay(this);
        gw->Init(sta,dev,sips);
        m_lstGateWay.append(gw);
    }
}

void ClientCenter::timerEvent(QTimerEvent *)
{
    foreach (GateWay* g, m_lstGateWay) {
        g->OnTime();
    }
    foreach (Device* d, m_lstDevice) {
        d->OnTimer();
    }
}
