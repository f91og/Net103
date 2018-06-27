#ifndef GENCOMMAND_H
#define GENCOMMAND_H

#include <QObject>
#include <QTimer>
#include <QVariant>
#include "dbobject/dbsession.h"
#include "comm/probase.h"
namespace ns_p103{

class DeviceProtocol;
class CommandHandler;
class GenCommand : public QObject
{
    Q_OBJECT
public:
    explicit GenCommand(DeviceProtocol *parent = 0);
    virtual ~GenCommand();
    virtual void RecvData(const QByteArray& data,uchar cot)=0;
    virtual void DoCommand()=0;
    void StartWait(int sec);
    void SetHandler(CommandHandler* h);
public slots:
    virtual void TimerOut();
public:
    DeviceProtocol* m_protocal;
    QTimer* m_timer;
    uchar m_RII;
    CommandHandler* m_handler;
};

class ReadCatalogueCommand: public GenCommand
{
    Q_OBJECT
public:
    ReadCatalogueCommand(DeviceProtocol *parent = 0);
    virtual void RecvData(const QByteArray& data,uchar cot);
    virtual void DoCommand();
    static int JudgGroupType(const QString& strName);
public slots:
    virtual void TimerOut();
public:
    QByteArray m_recvData;
    ushort m_count;
};

class ReadGroupValueCommand: public GenCommand
{
    Q_OBJECT
public:
    ReadGroupValueCommand(DeviceProtocol *parent = 0);
    void Init(int groupNo,int readType);
    virtual void RecvData(const QByteArray &data, uchar cot);
    virtual void DoCommand();
public slots:
    virtual void TimerOut();
public:
    QByteArray m_recvData;
    ushort m_count;
    int m_groupNo;
    int m_readType;
};

class GenWriteCommand: public GenCommand
{
    Q_OBJECT
public:
    GenWriteCommand(DeviceProtocol *parent = 0);
    void Init(const QVariantList& list);
    virtual void RecvData(const QByteArray &data, uchar cot);
    virtual void DoCommand();
    void SendWrite();
    void SendAffirm();
public slots:
    virtual void TimerOut();
public:
    QVariantList m_list;
    QByteArray m_writeBuf;
    bool m_countBit;
};

class GroundJumpCommand: public GenCommand
{
    Q_OBJECT
public:
    GroundJumpCommand(DeviceProtocol *parent = 0);
    void Init(ProBase::ControlType ct);
    virtual void RecvData(const QByteArray &data, uchar cot);
    virtual void DoCommand();
public slots:
    virtual void TimerOut();
public:
    int m_controlType;
    QByteArray m_sendBuf;
};

class ControlCommand: public GenCommand
{
    Q_OBJECT
public:
    ControlCommand(DeviceProtocol *parent = 0);
    void Init(DbObjectPtr obj,
              ProBase::ControlType ct,
              const QVariant& val,
              uint check);
    virtual void RecvData(const QByteArray &data, uchar cot);
    virtual void DoCommand();

public slots:
    virtual void TimerOut();
public:
    DbObjectPtr m_obj;
    int m_controlType;
    QVariant m_value;
    uint m_check;
    QByteArray m_sendBuf;
};

}

using namespace ns_p103;

#endif // GENCOMMAND_H
