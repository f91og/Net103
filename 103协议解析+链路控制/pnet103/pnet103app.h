#ifndef PNET103APP_H
#define PNET103APP_H

#include <QObject>
#include "comm/pnet103.h"
#include <QVariant>
#include <QVariantList>

class PNet103AppPrivate;

class PNET103_EXPORT PNet103App : public QObject
{
    Q_OBJECT
public:
    explicit PNet103App(QObject *parent = 0);
    virtual ~PNet103App();
    static PNet103App* GetInstance();
    void SetDeviceList(const QVariantList& list);

    void SendAppData(ushort sta,ushort dev,const QByteArray& data);

    void SetLocalAddr(ushort addr);
public:
    void EmitDeviceChange(ushort sta,ushort dev,bool add);
    void EmitRecvASDU(ushort sta,ushort dev, const QByteArray& data);
    void EmitNetStateChange(ushort sta,ushort dev,uchar net, uchar state);
signals:
    void DeviceChange(ushort sta,ushort dev,bool add);
    void RecvASDU(ushort sta,ushort dev, const QByteArray& data);
    void NetStateChange(ushort sta,ushort dev,uchar net, uchar state);
private:
    static PNet103App* m_instance;
    PNet103AppPrivate* d;
};

#endif // PNET103APP_H
