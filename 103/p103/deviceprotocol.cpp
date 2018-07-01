#include "deviceprotocol.h"
#include "edevice.h"
#include "protocol_define.h"
#include "comm/pnet103app.h"
#include <QtDebug>
#include "sys/sysapp.h"
#include <QSysInfo>
#include "comm/promonitorapp.h"
#include "pro103appprivate.h"
#include <QBitArray>
#include "tool/toolapi.h"
#include "ipc/ipcclient.h"

DataType::DataType()
{
    m_type=0;
    m_size=0;
    m_num=0;
}

bool DataType::IsValid() const
{
    return !(m_size==0 && m_num==0);
}

uint DataType::GetLength()const
{
    if(m_type==2){
        return (m_size+7)/8 * m_num;
    }
    return m_size* m_num;
}

///////////////////////////////////////////////////////
DeviceProtocol::DeviceProtocol(EDevice *parent) :
    QObject(parent)
{
    m_device = parent;
    m_RII=0;
    m_readCatalogueCommand=0;
}

void DeviceProtocol::OnTimer()
{
    QDateTime now = QDateTime::currentDateTime();
    if(!m_generalQueryTime.isValid() ||m_generalQueryTime.secsTo(now)>g_103p->m_periodGeneral){
        SendGeneralQuery();
    }

    if(!m_readPulseTime.isValid() || m_readPulseTime.secsTo(now)>g_103p->m_periodPulse){
        ReadPulse();
    }
    //qDebug()<<now<<m_readRelayMesureTime<<g_103p->m_periodRelayMeasure;
    if(!m_readRelayMesureTime.isValid() || m_readRelayMesureTime.secsTo(now)>g_103p->m_periodRelayMeasure){
        ReadRelayMeasure();
    }

//    if(!m_readSyncTime.isValid() || m_readSyncTime.secsTo(now)>g_103p->m_periodSync){
//        SendAsduTimeSync();//test here dengby 170926
//    }
}

void DeviceProtocol::ReadCatalog()
{
    if(m_readCatalogueCommand){
        return;
    }
    m_readCatalogueCommand = new ReadCatalogueCommand(this);
    m_readCatalogueCommand->DoCommand();
}

void DeviceProtocol::AddDataType(const QString& key,const DataType& dt)
{
    if(!m_mapDataType.contains(key)){
        m_mapDataType[key]=dt;
    }
}

void DeviceProtocol::ResetSignal()
{
    QByteArray asdu(8,0);
    asdu[ASDU_TYPE] = TYPC_COMMAND;
    asdu[ASDU_VSQ] = VSQ_COMMON1;
    asdu[ASDU_COT] = COTC_COMMAND;
    asdu[ASDU_ADDR] = m_device->GetDevAddr();
    asdu[ASDU_FUN] = 255;
    asdu[ASDU_INF] = 19;
    asdu[ASDU20_CC] = DCO_ON;
    asdu[ASDU20_RII] = GetRIIAndInc();
    SendAsdu(asdu);
}

void DeviceProtocol::GroundingJump(ProBase::ControlType ct, CommandHandler *h)
{
    GroundJumpCommand* cc = new GroundJumpCommand(this);
    cc->Init(ct);
    cc->SetHandler(h);
    cc->DoCommand();
}

void DeviceProtocol::DoControl(DbObjectPtr obj, ProBase::ControlType ct, const QVariant &val, uint check, CommandHandler *h)
{
    ControlCommand* cc = new ControlCommand(this);
    cc->SetHandler(h);
    cc->Init(obj,ct,val,check);
    cc->DoCommand();
}

DataType DeviceProtocol::GetDataType(const QString& key)
{
    return m_mapDataType.value(key,DataType());
}

GenCommand* DeviceProtocol::GetCommand(int rii)
{
    foreach (GenCommand* cmd, findChildren<GenCommand*>()) {
        if(cmd->m_RII==rii){
            return cmd;
        }
    }
    return 0;
}

void DeviceProtocol::ReadGroup(int no, int readType, CommandHandler *h)
{
    ReadGroupValueCommand* cmd = new ReadGroupValueCommand(this);
    cmd->Init(no,readType);
    cmd->SetHandler(h);
    cmd->DoCommand();
}

void DeviceProtocol::GenWrite(const QVariantList& list,CommandHandler* h)
{
    GenWriteCommand* cmd = new GenWriteCommand(this);
    cmd->SetHandler(h);
    cmd->Init(list);
    cmd->DoCommand();
}

void DeviceProtocol::ReadRelayMeasure()
{
    //qDebug()<<"读保护测量";
    foreach (int no, m_device->GetGroupNo(enumGroupTypeRelayMeasure)) {
        ReadGroupValueCommand* cmd = new ReadGroupValueCommand(this);
        cmd->Init(no,KOD_ACTUALVALUE);
        cmd->DoCommand();
        //qDebug()<<"组号："<<no;
    }
    m_readRelayMesureTime = QDateTime::currentDateTime();
}

void DeviceProtocol::ReadPulse()
{
    foreach (int no, m_device->GetGroupNo(enumGroupTypeYM)) {
        ReadGroupValueCommand* cmd = new ReadGroupValueCommand(this);
        cmd->Init(no,KOD_ACTUALVALUE);
        cmd->DoCommand();
    }
    m_readPulseTime = QDateTime::currentDateTime();
}

void DeviceProtocol::AnaData(const QByteArray& data)
{
    switch((uchar)data.at(0)){
    case 5:
        HandleASDU5(data);
        break;
    case 6:
        HandleASDU6(data);
        break;
    case 10:
        HandleASDU10(data);
        break;
    case 11:
        HandleASDU11(data);
        break;
    case 23:
        HandleASDU23(data);
        break;
    case 26:
        HandleASDU26(data);
        break;
    case 28:
        HandleASDU28(data);
        break;
    case 29:
        HandleASDU29(data);
        break;
    case 31:
        HandleASDU31(data);
        break;
    case 27:
        HandleASDU27(data);
        break;
    case 30:
        HandleASDU30(data);
        break;
    case 222:
        //        if(AppCommu::Instance()->m_byNodeType != NODETYPE_DISPATCH && AppCommu::Instance()->m_bIsHostNode)
        //            m_pFileTrans->HandleASDU222(pData, wLength, wFacNo, wDevID);
        break;
    case 223:
        //        if(AppCommu::Instance()->m_byNodeType != NODETYPE_DISPATCH && AppCommu::Instance()->m_bIsHostNode)
        //            m_pFileTrans->HandleASDU223(pData, wLength, wFacNo, wDevID);
        break;
    case 224:
        //        if(AppCommu::Instance()->m_byNodeType != NODETYPE_DISPATCH && AppCommu::Instance()->m_bIsHostNode)
        //            m_pFileTrans->HandleASDU224(pData, wLength, wFacNo, wDevID);
        break;
    case 225:
        //        if(AppCommu::Instance()->m_byNodeType != NODETYPE_DISPATCH && AppCommu::Instance()->m_bIsHostNode)
        //            m_pFileTrans->HandleASDU225(pData, wLength, wFacNo, wDevID);
        break;
    case 226:
        //        if(AppCommu::Instance()->m_byNodeType != NODETYPE_DISPATCH && AppCommu::Instance()->m_bIsHostNode)
        //            m_pFileTrans->HandleASDU226(pData, wLength, wFacNo, wDevID);
        break;
    case 229:
        HandleASDU229(data);
        break;
    default:
        break;
    }
}

uchar DeviceProtocol::GetRIIAndInc()
{
    uchar rii = m_RII;
    IncRII();
    return rii;
}

void DeviceProtocol::IncRII()
{
    if(m_RII==255){
        m_RII=0;
    }
    else{
        m_RII++;
    }
}


void DeviceProtocol::SendGeneralQuery()
{
    QByteArray asdu;
    asdu.resize(7);
    asdu[0]=0x07;
    asdu[1]=0x81;
    asdu[2]=0x09;
    asdu[3]=(uchar)m_device->GetDevAddr();
    asdu[4]=0xff;
    asdu[5]=0x00;
    asdu[6]=0xff;
    SendAsdu(asdu);

    ProMonitorApp::GetInstance()
            ->AddMonitor(ProIEC103,
                         MsgInfo,
                         m_device->GetAddrString(),
                         "发送总查询");

    m_generalQueryTime = QDateTime::currentDateTime();
}


void DeviceProtocol::GenDataHandle(const QByteArray& data,int cot)
{
    ProMonitorApp* mon = ProMonitorApp::GetInstance();
    uchar byGroup = data[GENDATAREC_GIN];
    uchar byInf = data[GENDATAREC_GIN+1];
    int groupType= m_device->GetGroupType(byGroup);
    if(groupType<0){
        mon->AddMonitor(ProIEC103,
                        MsgError,
                        m_device->GetAddrString(),
                        QString("未知组号:%1")
                        .arg(byGroup));
        return;
    }
    uchar kod = data[GENDATAREC_KOD];
    uchar byType = data[GENDATAREC_GDD];
    uchar byNum = data[GENDATAREC_GDD + GDDTYPE_NUM];
    uchar bySize = data[GENDATAREC_GDD + GDDTYPE_SIZ];
    uchar* GID = (uchar*)data.data()+GENDATAREC_GID;
    QString key = QString("GIN_%1_%2")
            .arg(byGroup)
            .arg(byInf);
    MeasPointInfo* mpi = m_device->GetMeasPointInfo(key);
    switch (groupType) {
    case enumGroupTypeYM:
    case enumGroupTypeTapPos:
    case enumGroupTypeRelayMeasure:
    case enumGroupTypeYC:
    {
        if(kod!=KOD_ACTUALVALUE){
            return;
        }
        //qDebug()<<"遥测";
        if(byType == 12 && bySize == 2 && byNum == 1 && data.size() >= 8 )
        {
            if(GID[0] & 0x02) return ;
            short nVal = ToolByteToNumber<short>(&GID[0]) / 8;
            double fVal = nVal*1.2/4096;
            if(mpi){
                mpi->SetValue(fVal);
            }
            mon->AddMonitor(ProIEC103,
                            MsgInfo,
                            m_device->GetAddrString(),
                            QString("遥测:%1,值：%2,码值：%3")
                            .arg(key)
                            .arg(fVal)
                            .arg(nVal));
        }
        //ieee标准754短实数
        else if(byType== 7 && bySize == 4 && byNum == 1 && data.size() >= 10 )
        {
            float fVal= ToolByteToNumber<float>(&GID[0]);
            if(mpi){
                mpi->SetValue(fVal);
            }
            mon->AddMonitor(ProIEC103,
                            MsgInfo,
                            m_device->GetAddrString(),
                            QString("遥测:%1,值：%2")
                            .arg(key)
                            .arg(fVal)
                            );
        }
        else if(byType==3 && byNum==1 ){
            double fVal=0;
            if(bySize==2 && data.size()>=6 ){
                fVal =  ToolByteToNumber<ushort>(&GID[0]);
            }
            else if(bySize==4 && data.size()>=8 ){
                fVal = ToolByteToNumber<uint>(&GID[0]);
            }
            else{
                return;
            }
            if(mpi){
                mpi->SetValue(fVal);
            }
            mon->AddMonitor(ProIEC103,
                            MsgInfo,
                            m_device->GetAddrString(),
                            QString("遥测:%1,值：%2")
                            .arg(key)
                            .arg(fVal)
                            );
        }
        else if(byType  == 201 && bySize == 5 && byNum == 1 && data.size() >= 11) {
            if(GID[4] & 0x80) return ;
            uint dwVal = ToolByteToNumber<uint>(GID);
            if(mpi){
                mpi->SetValue((double)dwVal,false);
            }
            mon->AddMonitor(ProIEC103,
                            MsgInfo,
                            m_device->GetAddrString(),
                            QString("遥脉:%1,值：%2")
                            .arg(key)
                            .arg(dwVal));
        }
        else if( byType == 202 && bySize == 8 && data.size() >= 14 && byNum == 1 )
        {
            if(GID[3] & 0xf0) return ;
            uchar byBIT0 = GID[1] & 0x0f;
            uchar byBIT1 = (GID[1] & 0xf0 ) >> 4;
            uchar byBIT2 = GID[2] & 0x0f;
            uchar byBIT3 = (GID[2] & 0xf0 ) >> 4;
            uint dwVal = byBIT0 + byBIT1 * 10 + byBIT2 * 100 + byBIT3 * 1000;

            uchar* pTime = &GID[4];
            uchar hour = pTime[SHORTTIME_HOUR];
            uchar minute = pTime[SHORTTIME_MIN];
            ushort msec = ToolByteToNumber<ushort>(&pTime[SHORTTIME_MSEC]);
            QTime t(hour,minute);
            t = t.addMSecs(msec);
            QDate d = QDate::currentDate();
            QDateTime dt(d,t);
            if(mpi){
                mpi->SetValue((double)dwVal,false,dt);
            }
            mon->AddMonitor(ProIEC103,
                            MsgInfo,
                            m_device->GetAddrString(),
                            QString("档位:%1,值：%2")
                            .arg(key)
                            .arg(dwVal));
        }
    }
        break;
    case enumGroupTypeYX:
    case enumGroupTypeNetState:
    case enumGroupTypeActElement:
    case enumGroupTypeDevCheckSelf:
    case enumGroupTypeRunWarn:
    case enumGroupTypeSoftSwitch:
    case enumGroupTypeHardSwitch:
    {
        if(kod!=KOD_ACTUALVALUE){
            return;
        }
        //qDebug()<<"遥信" ;
        if( ( byType == 19 && data.size() >= 16 && bySize == 10  )
                || ( byType == 18 && data.size() >= 12 && bySize == 6  )
                || ( byType == 203 && data.size() >= 15 && bySize == 9  )
                || ( byType == 204 && data.size() >= 19 && bySize == 13  )
                || ( byType == 3 && data.size() >= 7 && bySize == 1  )
                || ( byType == 9 && data.size() >= 7 && bySize == 1  ) ) {

            uchar byVal = GID[0] & 0x03;
            uchar val=0;
            QDateTime dt;
            if(byType != 3){
                if(byVal == 2) {
                    val = 1;
                }
                else if(byVal == 1) {
                    val = 0;
                }
                else {
                    mon->AddMonitor(ProIEC103,
                                    MsgError,
                                    m_device->GetAddrString(),
                                    QString("遥信:%1,值：%2，错误,类型:%3")
                                    .arg(key)
                                    .arg(byVal)
                                    .arg(byType)
                                    );
                    return;
                }
            }
            else {
                if(byVal != 0 && byVal != 1) {
                    mon->AddMonitor(ProIEC103,
                                    MsgError,
                                    m_device->GetAddrString(),
                                    QString("遥信:%1,值：%2，错误,类型:%3")
                                    .arg(key)
                                    .arg(byVal)
                                    .arg(byType)
                                    );
                    return;
                }
                val = byVal;
            }
            if( byType == 19 || byType == 18 ){
                uchar* pTime;
                if(byType == 19) {
                    pTime = &GID[5];
                }
                else pTime = &GID[1];
                uchar hour = pTime[SHORTTIME_HOUR];
                uchar minute = pTime[SHORTTIME_MIN];
                ushort msec = ToolByteToNumber<ushort>(&pTime[SHORTTIME_MSEC]);
                QTime t(hour,minute);
                t = t.addMSecs(msec);
                QDate d = QDate::currentDate();
                dt = QDateTime(d,t);
            }
            else if( byType == 204 || byType == 203){
                uchar* pTime;
                if(byType == 204) {
                    pTime = &GID[5];
                }
                else pTime = &GID[1];
                uchar year = pTime[LONGTIME_YEAR];
                uchar month = pTime[LONGTIME_MON];
                uchar day = pTime[LONGTIME_DAY];
                uchar hour = pTime[LONGTIME_HOUR];
                uchar minute = pTime[LONGTIME_MIN];
                ushort ms = ToolByteToNumber<ushort>(&pTime[LONGTIME_MSEC]);
                QDate d(year,month,day);
                QTime t(hour,minute);
                t=t.addMSecs(ms);
                dt = QDateTime(d,t);
            }
            bool change = (cot==1);
            if(mpi){
                mpi->SetValue(val,change,dt);
            }
            mon->AddMonitor(ProIEC103,
                            MsgInfo,
                            m_device->GetAddrString(),
                            QString("遥信:%1,值：%2,时间：%3,原因:%4")
                            .arg(key)
                            .arg(val)
                            .arg(dt.toString("yyyy-MM-dd hh:mm:ss.zzz"))
                            .arg(cot));
        }
        break;
    default:
            break;
        }
    }
}

void DeviceProtocol::RecvGenData(const QByteArray &data, int num, int cot)
{

    if(IpcClient::GetInstance()->IsSimu()){
        return;
    }
    ushort wLen = 0;
    uchar* pHead = (uchar* )data.data();
    for(uchar i = 0; i<num; i++){
        uchar* pCurRec = (uchar* )pHead;
        ushort wDataSize;
        if(pCurRec[GENDATAREC_GDD] == 2)// 成组位串
            wDataSize = ((pCurRec[GENDATAREC_GDD+GDDTYPE_SIZ]+7)/8) * pCurRec[GENDATAREC_GDD+GDDTYPE_NUM] + 6;
        else wDataSize = pCurRec[GENDATAREC_GDD+GDDTYPE_SIZ] * pCurRec[GENDATAREC_GDD+GDDTYPE_NUM] + 6;
        pHead += wDataSize;
        wLen += wDataSize;
        if(wLen > data.size()){
            return;
        }
        QByteArray ba(wDataSize,0);
        memcpy(ba.data(), pCurRec, wDataSize);
        GenDataHandle(ba,cot);
    }
}


void DeviceProtocol::SendAsduTimeSync(void)
{
    QByteArray asdu(4,0);
    asdu[ASDU_TYPE] = (char) TYPC_TIMESYNC;
    asdu[ASDU_VSQ] = (char) VSQ_COMMON1;
    asdu[ASDU_COT] = (char) COTC_TIMESYNC;
    asdu[ASDU_ADDR] = (uchar)(m_device->GetDevAddr());

    QTime tm = QTime::currentTime();
    ushort hour = tm.hour();
    ushort min = tm.minute();
    ushort ms = tm.msec();
    ushort seconds = tm.second();
    seconds *= 1000;
    ms += seconds;
    //int ms = 40153;//test
    ushort ms_h = (ms>>8) & 0xff;
    ushort ms_l = ms & 0x00ff;
    QDate date = QDate::currentDate();
    ushort yr = date.year()%100;
    ushort mon = date.month();
    ushort day = date.day();
    ushort dayOfWeek = date.dayOfWeek();
    ushort daymonth_wk = (dayOfWeek << 4) | day;

    QByteArray timeDataBody(9,0);
    int idx = GENDATACOM_FUN;
    timeDataBody[idx] = 255;
    idx = GENDATACOM_INF;
    timeDataBody[idx++] = 0;
    timeDataBody[idx++] = ms_l;
    timeDataBody[idx++] = ms_h;
    //timeDataBody[idx++] = seconds;
    timeDataBody[idx++] = min;
    timeDataBody[idx++] = hour;//summer time
    timeDataBody[idx++] = day;//daymonth_wk;//day of month day of week
    timeDataBody[idx++] = mon;//
    timeDataBody[idx++] = yr;//

    asdu += timeDataBody;
    SendAsdu(asdu);
    m_readSyncTime = QDateTime::currentDateTime();

    ProMonitorApp::GetInstance()
            ->AddMonitor(ProIEC103,
                         MsgInfo,
                         m_device->GetAddrString(),
                         "发送对时");
}

void DeviceProtocol::SendAsdu21(const QByteArray& data)
{
    QByteArray asdu(4,0);
    asdu[ASDU_TYPE] = (char) TYPC_GRC_COMMAND;
    asdu[ASDU_VSQ] = (char) VSQ_COMMON1;
    asdu[ASDU_COT] = (char) COTC_GRC_READ;
    asdu[ASDU_ADDR] = (uchar)(m_device->GetDevAddr());
    asdu+= data;
    SendAsdu(asdu);
}

void DeviceProtocol::SendAsdu(const QByteArray& data)
{
    if(ProMonitorApp::GetInstance()->HasMonitor()){
        ProMonitorApp::GetInstance()
                ->AddMonitor(ProIEC103,
                             MsgSend,
                             m_device->GetAddrString(),
                             QString(),
                             data);
    }
    PNet103App::GetInstance()
            ->SendAppData(m_device->GetStationAddr(),
                          m_device->GetDevAddr(),
                          data);
}

void DeviceProtocol::SendAsdu10(const QByteArray& data)
{
    QByteArray asdu(4,0);
    asdu[ASDU_TYPE] = (char) TYPC_GRC_DATA;
    asdu[ASDU_VSQ] = (char) VSQ_COMMON1;
    asdu[ASDU_COT] = (char) COTC_GRC_WRITE;
    asdu[ASDU_ADDR] = (uchar)(m_device->GetDevAddr());
    asdu+= data;
    SendAsdu(asdu);
}

void DeviceProtocol::HandleASDU5(const QByteArray& data)
{
    Q_UNUSED(data);
}

void DeviceProtocol::HandleASDU6(const QByteArray& data)
{
    Q_UNUSED(data);
}


QByteArray DeviceProtocol::ValueToGDD(const DataType &dat, const QString &key, const QVariant &v)
{
    uint len = dat.GetLength();
    QByteArray ba(len+6,0);
    uchar groupno = key.section("_",1,1).toUInt();
    uchar inf = key.section("_",2,2).toUInt();
    ba[GENDATAREC_GIN] = groupno;
    ba[GENDATAREC_GIN+1] = inf;
    ba[GENDATAREC_KOD] = KOD_ACTUALVALUE;
    ba[GENDATAREC_GDD] = dat.m_type;
    ba[GENDATAREC_GDD+1] = dat.m_size;
    ba[GENDATAREC_GDD+2] = dat.m_num;
    uchar* buf = (uchar*)ba.data()+6;
    switch (dat.m_type) {
    case 1:
    {
        QByteArray cstring = g_103p->m_gbk->fromUnicode(v.toString());
        memcpy(buf,cstring.data(),qMin((int)dat.m_size,cstring.length()));
    }
        break;
    case 2:
    {
        QBitArray ba = v.toBitArray();
        for(int i=0;i<dat.m_size;i++){
            int byte = i/8;
            int bit = i%8;
            uchar& bits = buf[byte];
            bool val = ba.testBit(i);
            if(val){
                bits |= (i<<bit);
            }
        }
    }
        break;
    case 3:
    {
        QVariantList list;
        if(v.type() == QVariant::List){
            list = v.toList();
        }
        else{
            list += v.toUInt();
        }
        uchar* p = buf;
        for(int i=0;i<dat.m_num;i++,p+=dat.m_size){
            if(i>=list.count()){
                break;
            }
            uint val=list.at(i).toUInt();
            if(dat.m_size==1){
                uchar v = val;
                ToolNumberToByte(p,v);
            }
            else if( dat.m_size ==2){
                ushort v = val;
                ToolNumberToByte(p,v);
            }
            else if( dat.m_size == 4){
                uint v = val;
                ToolNumberToByte(p,v);
            }
        }
    }
        break;
    case 4:
    {
        QVariantList list;
        if(v.type() == QVariant::List){
            list = v.toList();
        }
        else{
            list += v.toInt();
        }
        uchar* p = buf;
        for(int i=0;i<dat.m_num;i++,p+=dat.m_size){
            if(i>=list.count()){
                break;
            }
            int val=list.at(i).toInt();
            if( dat.m_size ==1){
                char v = val;
                ToolNumberToByte(p,v);
            }
            else if( dat.m_size ==2){
                short v = val;
                ToolNumberToByte(p,v);
            }
            else if( dat.m_size ==4){
                int v = val;
                ToolNumberToByte(p,v);
            }
        }
    }
        break;
    case 7:
    {
        QVariantList list;
        if(v.type() == QVariant::List){
            list = v.toList();
        }
        else{
            list += v.toFloat();
        }
        uchar* p = buf;
        for(int i=0;i<dat.m_num;i++,p+=dat.m_size){
            if(i>=list.count()){
                break;
            }
            float val=list.at(i).toFloat();
            if(dat.m_size==4){
                float v = val;
                ToolNumberToByte(p,v);
            }
        }
    }
        break;
    default:
        qDebug()<<"未处理类型："<<dat.m_type<<",num:"<<dat.m_num
               <<",size:"<<dat.m_size;
        break;
    }
    return ba;
}

QVariant DeviceProtocol::GDDValue(uchar* buf,int type,int size,int num)
{
    switch (type) {
    case 1:
    {
        QByteArray ba(size,0);
        memcpy(ba.data(),buf,size);
        return g_103p->m_gbk->toUnicode(ba);
    }
        break;
    case 2:
    {
        QBitArray ba(size);
        for(int i=0;i<size;i++){
            int byte = i/8;
            int bit = i%8;
            uchar bits = buf[byte];
            bool val = (bits>>bit)!=0;
            ba.setBit(i,val);
        }
        //qDebug()<<buf[0]<<buf[1]<<buf[2];
        return ba;
    }
        break;
    case 3:
    {
        QVariantList list;
        uchar* p = buf;
        for(int i=0;i<num;i++,p+=size){
            uint val=0;
            if(size==1){
                val = (uchar)p[0];
            }
            else if(size==2){
                val = ToolByteToNumber<ushort>(p);
            }
            else if(size==4){
                val = ToolByteToNumber<uint>(p);
            }
            list.append(val);
        }
        if(num==1){
            return list[0];
        }
        else{
            return list;
        }
    }
        break;
    case 4:
    {
        QVariantList list;
        uchar* p = buf;
        for(int i=0;i<num;i++,p+=size){
            int val=0;
            if(size==1){
                val = (char)p[0];
            }
            else if(size==2){
                val = ToolByteToNumber<short>(p);
            }
            else if(size==4){
                val = ToolByteToNumber<int>(p);
            }
            list.append(val);
        }
        if(num==1){
            return list[0];
        }
        else{
            return list;
        }
    }
        break;
    case 7:
    {
        QVariantList list;
        uchar* p = buf;
        for(int i=0;i<num;i++,p+=size){
            float val=0;
            if(size==4){
                val = ToolByteToNumber<float>(p);
            }
            list.append(val);
        }
        if(num==1){
            return list[0];
        }
        else{
            return list;
        }
    }
        break;
    default:
        qDebug()<<"未处理类型："<<type<<",num:"<<num;
        break;
    }
    return QVariant();
}

ushort DeviceProtocol::CalcuGrcDataLen(uchar* pgd, ushort wCount)
{
    if(wCount<4) return 0;
    ushort wLen = 4;
    uchar* pHead = (uchar* )pgd;
    pHead += 4;
    uchar byNum = pgd[GENDATACOM_NGD] & 0x3f;
    if(byNum > 0){
        for(quint32 i=0; i<byNum; i++){
            if((wLen + GENDATAREC_SIZE) > wCount){
                pgd[GENDATACOM_NGD] = 0;
                wLen = 4;
                break;
            }
            //SGenDataRec *pgdr = (SGenDataRec *)pHead;
            if(pHead[GENDATAREC_GDD] == 2){	// 成组位串
                wLen += 6+((pHead[GENDATAREC_GDD + GDDTYPE_SIZ]+7)/8)*pHead[GENDATAREC_GDD + GDDTYPE_NUM];
                pHead += 6+((pHead[GENDATAREC_GDD + GDDTYPE_SIZ]+7)/8)*pHead[GENDATAREC_GDD + GDDTYPE_NUM];
            }
            else {
                wLen += 6+pHead[GENDATAREC_GDD + GDDTYPE_SIZ]*pHead[GENDATAREC_GDD + GDDTYPE_NUM];
                pHead += 6+pHead[GENDATAREC_GDD + GDDTYPE_SIZ]*pHead[GENDATAREC_GDD + GDDTYPE_NUM];
            }
            if(wLen>wCount) {
                pgd[GENDATACOM_NGD] = 0;
                wLen = 4;
                break;
            }
        }
    }
    return wLen;
}

void DeviceProtocol::HandleASDU10(const QByteArray& data)
{
    if(data.size() <= 4) return;
    ushort wLen = data.size()-4;
    uchar* pData = (uchar*)data.data();
    uchar* pDataCom = (uchar* )(pData+4);
    ushort wCalLen = CalcuGrcDataLen(pDataCom, wLen);
    if(wCalLen < 4) return;

    uchar byCot = pData[ASDU_COT];
    uchar byRII = pData[ASDU21_RII];
    uchar byNum = pDataCom[GENDATACOM_NGD] & 0x3f;
    if(byCot == COT_SPONTANEOUS || byCot == COT_CYCLIC || byCot == COT_GI || byCot == COT_TELECONTROL){
        QByteArray ba(wCalLen-4,0);
        memcpy(ba.data(),pDataCom+4,ba.size());
        RecvGenData(ba,byNum,byCot);
    }
    else {
        GenCommand* cmd = GetCommand(byRII);
        if(!cmd){
            return;
        }
        QByteArray ba(wCalLen,0);
        memcpy(ba.data(),pDataCom,wCalLen);
        cmd->RecvData(ba,byCot);
    }
}

void DeviceProtocol::HandleASDU11(const QByteArray& data)
{
    Q_UNUSED(data);
}

void DeviceProtocol::HandleASDU23(const QByteArray& data)
{
    Q_UNUSED(data);
}

void DeviceProtocol::HandleASDU26(const QByteArray& data)
{
    Q_UNUSED(data);
}


void DeviceProtocol::HandleASDU27(const QByteArray& data)
{
    Q_UNUSED(data);
}


void DeviceProtocol::HandleASDU28(const QByteArray& data)
{
    Q_UNUSED(data);
}


void DeviceProtocol::HandleASDU29(const QByteArray& data)
{
    Q_UNUSED(data);
}


void DeviceProtocol::HandleASDU30(const QByteArray& data)
{
    Q_UNUSED(data);
}


void DeviceProtocol::HandleASDU31(const QByteArray& data)
{
    Q_UNUSED(data);
}


void DeviceProtocol::HandleASDU229(const QByteArray& data)
{
    Q_UNUSED(data);
}



