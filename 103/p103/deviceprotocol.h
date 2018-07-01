#ifndef DEVICEPROTOCOL_H
#define DEVICEPROTOCOL_H

#include <QObject>
#include "gencommand.h"
#include <QPointer>
#include <QDateTime>
#include <QMap>
namespace ns_p103{

class EDevice;

class DataType
{
public:
    DataType();
    bool IsValid()const;
    uint GetLength()const;
public:
    uchar m_type;
    uchar m_size;
    uchar m_num;
};

class DeviceProtocol : public QObject
{
    Q_OBJECT
public:
    explicit DeviceProtocol(EDevice *parent = 0);
    void OnTimer();

    void ReadCatalog();
    void AnaData(const QByteArray& data);

    void ReadPulse();
    void ReadRelayMeasure();

    void ReadGroup(int no,int readType,CommandHandler* h);
    void GenWrite(const QVariantList& list,CommandHandler* h);

    GenCommand* GetCommand(int rii);
    void AddDataType(const QString& key,const DataType& dt);
    DataType GetDataType(const QString& key);

    void ResetSignal();

    void DoControl(DbObjectPtr obj,
                   ProBase::ControlType ct,
                   const QVariant& val,
                   uint check,
                   CommandHandler* h);

    void GroundingJump(ProBase::ControlType ct,
                       CommandHandler* h);
public:
    //命令
    void RecvGenData(const QByteArray& data,int num,int cot);
    void GenDataHandle(const QByteArray& data,int cot);
    void SendGeneralQuery();
    uchar GetRIIAndInc();
private:
    void IncRII();
public:
    //发送asdu
    void SendAsdu21(const QByteArray& data);
    void SendAsdu10(const QByteArray& data);

    void SendAsdu(const QByteArray& data);
    void SendAsduTimeSync(void);
private:
    //分析asdu
    void HandleASDU5(const QByteArray& data);
    void HandleASDU6(const QByteArray& data);
    void HandleASDU10(const QByteArray& data);
    void HandleASDU11(const QByteArray& data);
    void HandleASDU23(const QByteArray& data);
    void HandleASDU26(const QByteArray& data);
    void HandleASDU27(const QByteArray& data);
    void HandleASDU28(const QByteArray& data);
    void HandleASDU29(const QByteArray& data);
    void HandleASDU30(const QByteArray& data);
    void HandleASDU31(const QByteArray& data);
    void HandleASDU229(const QByteArray& data);
public:
    static ushort CalcuGrcDataLen(uchar* pgd, ushort wCount);
    static QVariant GDDValue(uchar* buf,int type,int size,int num);
    static QByteArray ValueToGDD(const DataType& dat, const QString& key,const QVariant& v);
public:
    EDevice* m_device;
    uchar m_RII;

    QPointer<ReadCatalogueCommand> m_readCatalogueCommand;
private:
    QDateTime m_generalQueryTime;
    QDateTime m_readPulseTime;
    QDateTime m_readRelayMesureTime;
    QDateTime m_readSyncTime;//for sync time dengby 20160926

    QMap<QString,DataType> m_mapDataType;
};

}
using namespace ns_p103;

#endif // DEVICEPROTOCOL_H
