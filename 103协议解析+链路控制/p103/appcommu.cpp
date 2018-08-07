#include "appcommu.h"

#include "comm/LinkCommuImport.h"

#include <QDateTime>
#include <QMutex>
#include "protocol_define.h"
#include <QSysInfo>
#include <QTimerEvent>
#include <QCoreApplication>
#include "comm/pnet103app.h"
#include "sys/sysapp.h"

#define MAXSHOWBUF		5000

void Protocol_Monitor();
bool	g_bMonitorRun = true;

QMutex	g_mtxMonitor;
QList<tagShowMsg*> g_plShowItem;

/////////////////////////////////////////////////////////////////////////////
// AppCommu construction
AppCommu* AppCommu::m_pInstance = NULL;
AppCommu* AppCommu::Instance()
{
    if(!m_pInstance) m_pInstance = new AppCommu(qApp);
	return m_pInstance;
} 

void AppCommu::Exitance()
{
	if(m_pInstance) delete m_pInstance;
	m_pInstance = NULL;
}

AppCommu:: AppCommu(QObject *parent)
    :QObject(parent)
{
	m_pProtocol		= NULL;
	m_lpProCallBack = NULL;
	m_dwProtocolTimerID = -1;

	m_wPulseTimes = 360;
	m_wQuaryTimes = 300;
	m_wYKSelTimes = 20;
	m_wYKRunTimes = 20;
	m_bIsHostNode = false;
	m_strWaveDir = "";
    m_byNodeType = NODETYPE_MONITOR;

    PNet103App* net103 = PNet103App::GetInstance();
    connect(net103,
            SIGNAL(DeviceChange(ushort,ushort,bool)),
            this,
            SLOT(SlotDeviceChange(ushort,ushort,bool))
            );

    connect(net103,
            SIGNAL(NetStateChange(ushort,ushort,uchar,uchar)),
            this,
            SLOT(SlotNetStateChange(ushort,ushort,uchar,uchar))
            );

    connect(net103,
            SIGNAL(RecvASDU(ushort,ushort,QByteArray)),
            this,
            SLOT(SlotRecvASDU(ushort,ushort,QByteArray))
            );

    connect(SysApp::GetInstance(),
            SIGNAL(DutyChange(bool)),
            this,
            SLOT(SlotDutyChange(bool))
            );

}

void AppCommu::SlotDeviceChange(ushort sta,ushort dev,bool add)
{
    IDeviceAddOrDel(sta,dev,NODETYPE_RELAYCKDEV,(add?1:0) );
}

void AppCommu::SlotRecvASDU(ushort sta,ushort dev, const QByteArray& data)
{
    IAnalyse103Data(sta,dev,(char*)data.data(),data.size());
}

void AppCommu::SlotNetStateChange(ushort sta,ushort dev,uchar net, uchar state)
{
    INotifyNetDState(sta,dev,net,state);
}

void AppCommu::SlotDutyChange(bool v)
{
    SetNodeState(v);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only AppCommu object

void AppCommu::SetNodeState(bool bIsHostNode)
{
	m_bIsHostNode = bIsHostNode;
}

void AppCommu::ProtocolNetSend(ushort wFacNo, ushort wLogicalNodeId, char* byBuf, ushort wLen)
{
    QByteArray data;
    data.resize(wLen);
    memcpy(data.data(),byBuf,wLen);
    PNet103App::GetInstance()
            ->SendAppData(wFacNo,wLogicalNodeId,data);
}

void AppCommu::ProtocolNetBroadcast(ushort wFacNo, ushort wLogicalNodeId, char* byBuf, ushort wLen)
{    
    QByteArray data;
    data.resize(wLen);
    memcpy(data.data(),byBuf,wLen);
    PNet103App::GetInstance()
            ->SendAppData(wFacNo,wLogicalNodeId,data);
}

void AppCommu::ProtocolVirtualSend(ushort wFacNo, ushort wLogicalNodeId, ushort wVDevID, char* byBuf, ushort wLen)
{
    Q_UNUSED(wFacNo);
    Q_UNUSED(wLogicalNodeId);
    Q_UNUSED(wVDevID);
    Q_UNUSED(byBuf);
    Q_UNUSED(wLen);
//	tagShowMsg* pMsg = new tagShowMsg;
//	pMsg->wFacID = wFacNo;
//	pMsg->wDevID = wLogicalNodeId;
//	pMsg->byType = PROTOCOL_VIRTUALSND;
//	pMsg->wLen = wLen;
//	pMsg->pShowMsg = new char[wLen];
//	memcpy(pMsg->pShowMsg ,byBuf ,wLen);
//	AddMsgToShowList(pMsg);
//    LinkCommuVirtualSend(wFacNo, wLogicalNodeId, wVDevID, byBuf, wLen, MSGTYPE_103DATA);
}

//-----------------------------输出函数-------------------------------
bool AppCommu::IStart()
{
    m_nThreadGrp = 1100;
    g_bMonitorRun = true;
    m_pProtocol = new CProtocol;
    m_dwProtocolTimerID = startTimer(1000);
   return true;
}

bool AppCommu::IEnd()
{
	if(m_pProtocol) delete m_pProtocol;
	if(m_dwProtocolTimerID != -1) killTimer(m_dwProtocolTimerID);
	m_pProtocol = NULL;
	g_bMonitorRun = false;
	Exitance();
    return true;
}

bool AppCommu::ISetNetInf(char* strIP1, char* strIP2, ushort wFactoryID, 
                                 ushort wLocalID, QList<ushort>* pVirtualAddrList, uchar byNodeType, ushort wPort)
{
    Q_UNUSED(strIP1);
    Q_UNUSED(strIP2);
    Q_UNUSED(wFactoryID);
    Q_UNUSED(wLocalID);
    Q_UNUSED(pVirtualAddrList);
    Q_UNUSED(wPort);
    m_byNodeType = byNodeType;
    return true;
}


bool AppCommu::ISetCallBack(LPPROCALLBACK lpProCallBack)
{
	m_lpProCallBack = lpProCallBack;
	return true;
}

bool AppCommu::IAnalyse103Data(ushort wFacID, ushort wDevID, char* pData, ushort wLen)
{
	if(!m_pProtocol) return false;
	m_pProtocol->IDataUnpack(wFacID, wDevID, (uchar* )pData, wLen);
	return true;
}

bool AppCommu::IAnalyseInternalData(ushort wFacID, ushort wDevID, char* pData, ushort wLen)
{
	tagShowMsg* pMsg = new tagShowMsg;
	pMsg->wFacID = wFacID;
	pMsg->wDevID = wDevID;
	pMsg->byType = PROTOCOL_RECVMSG;
	pMsg->wLen = wLen;
	pMsg->pShowMsg = new char[wLen];
	memcpy(pMsg->pShowMsg ,pData ,wLen);
	AddMsgToShowList(pMsg);

	if(m_lpProCallBack == NULL) return false;
	SInternalData* pInternalData = new SInternalData;
	pInternalData->wFacAddr = wFacID;
	pInternalData->wDevAddr = wDevID;
	pInternalData->pData = pData;
	pInternalData->wLen = wLen;
	m_lpProCallBack(PROMSG_INTERNALENTERDB, (char* )pInternalData, wLen);
	delete pInternalData;
	return true;
}

bool AppCommu::IAnalyseVirtual103(ushort wFacID, ushort wDevID, ushort wVDevID, char* pData, ushort wLen)
{
	tagShowMsg* pMsg = new tagShowMsg;
	pMsg->wFacID = wFacID;
	pMsg->wDevID = wDevID;
	pMsg->byType = PROTOCOL_VIRTUALRCV;
	pMsg->wLen = wLen;
	pMsg->pShowMsg = new char[wLen];
	memcpy(pMsg->pShowMsg ,pData ,wLen);
	AddMsgToShowList(pMsg);
	if(!m_pProtocol) return false;
	m_pProtocol->IVirtualDataUnpack(wFacID, wDevID, wVDevID, (uchar* )pData, wLen);
	return true;
}

bool AppCommu::INotifyNetDState(ushort wFacID, ushort wDevID, uchar byNetType, uchar byNetState)
{
	if(!m_pProtocol) return false;
	m_pProtocol->INotifyNetDState(wFacID, wDevID, byNetType, byNetState);
	return true;
}

bool AppCommu::IDeviceAddOrDel(ushort wFacID, ushort wDevID, uchar byType, uchar byAddOrDel)
{
	if(!m_pProtocol) return false;
	m_pProtocol->IDeviceAddOrDel(wFacID, wDevID, byType, byAddOrDel);
	return true;
}

void AppCommu::IAskGeneralQuary()
{
	if(m_pProtocol) m_pProtocol->IAskGeneralQuary();
}


bool AppCommu::IVirtualSendYX(ushort wVDevNo, ushort wNum, SVirtualYXSend* pYXSend)
{
	if(!m_pProtocol) return false;
	m_pProtocol->IVirtualAutoSendYX(wVDevNo, wNum, pYXSend);
	return true;
}

bool AppCommu::IVirtualSendYC(ushort wVDevNo, ushort wNum, SVirtualYCSend* pYCSend)
{
	if(!m_pProtocol) return false;
	m_pProtocol->IVirtualAutoSendYC(wVDevNo, wNum, pYCSend);
	return true;
}

bool AppCommu::ISendInternalData(ushort wFacID, ushort wDevID, char * pData, ushort wLen)
{
	tagShowMsg* pMsg = new tagShowMsg;
	pMsg->wFacID = wFacID;
	pMsg->wDevID = wDevID;
	pMsg->byType = PROTOCOL_SENDMSG;
	pMsg->wLen = wLen;
	pMsg->pShowMsg = new char[wLen];
	memcpy(pMsg->pShowMsg ,pData ,wLen);
	AddMsgToShowList(pMsg);
    //return LinkCommuBroadcastData(wFacID, wDevID, pData, wLen ,MSGTYPE_INTERNALDATA);
    return false;
}

bool AppCommu::IReadGenDataAll(ushort wFacID, ushort wDevID, EPreDefinedGroupType byGroupType,
										EDevReadType byReadType, uchar byGroupNo)
{
	if(!m_pProtocol) return false;
	m_pProtocol->IGenReadItem(wFacID, wDevID, byGroupType, byReadType , byGroupNo);
	return true;
}

bool AppCommu::IReadGenData(ushort wFacID, ushort wDevID, EPreDefinedGroupType byGroupType,
										EDevReadType byReadType, SGINType* pGIN, ushort wReadNum)
{
	if(!m_pProtocol) return false;
	m_pProtocol->IGenReadItem(wFacID, wDevID, byGroupType, byReadType, pGIN, wReadNum);
	return true;
}

//写包装好条目的值
bool AppCommu::IWriteGenDataAll(ushort wFacAddr, ushort wDevAdd, uchar byType, 
															ushort wNum, ushort wLen, char* pDataSnd)
{
	if(!m_pProtocol) return false;
	m_pProtocol->IGenWriteAllItem(wFacAddr, wDevAdd, (EPreDefinedGroupType) byType, wNum, wLen, pDataSnd);
	return true;
}


bool AppCommu::IWriteGenData(ushort wFacID, ushort wDevID, EPreDefinedGroupType byGroupType, 
															ushort wNum, SGENDataItem* paSetting)
{
	if(!m_pProtocol) return false;
	m_pProtocol->IGenWriteItem(wFacID, wDevID, byGroupType, wNum, paSetting);
	return true;
}

bool AppCommu::IYKYTSend(ushort wFacID, ushort wDevID, SYKYTData* pData, bool bSCYK)
{
	if(!m_pProtocol) return false;
	m_pProtocol->IYKYTSend(wFacID, wDevID, pData, bSCYK);
	return true;
}

bool AppCommu::ISendTimeSync(ushort wFacID, QDateTime tTime)
{
	if(!m_pProtocol) return false;
	m_pProtocol->ISendTimeSync(wFacID, tTime);
	return true;
}

bool AppCommu::IRelayReset(ushort wFacNo, ushort wDevID)
{
	if(!m_pProtocol) return false;
	m_pProtocol->IRelayReset(wFacNo, wDevID);
	return true;
}

bool AppCommu::IJudgDeviceIsOnLine(ushort wFacNo, ushort wDevID)
{
	if(!m_pProtocol) return false;
	return m_pProtocol->IJudgDeviceIsOnLine(wFacNo, wDevID);
}

bool AppCommu::IReadHistoryData(ushort wFacID, ushort wDevID, EPreDefinedGroupType byReadType, qint32 tStart, qint32 tEnd)
{
	if(!m_pProtocol) return false;
	m_pProtocol->IReadHistoryData(wFacID, wDevID, byReadType, tStart, tEnd);
	return true;
}

void AppCommu::IYKCheck(ushort wFacID, ushort wDevID, SYKCheckDataID* pYKCheckID)
{
	if(m_pProtocol) m_pProtocol->IYKCheck(wFacID, wDevID, pYKCheckID);
}

void AppCommu::IVYKCheckBack(ushort wFacID, ushort wDevID, SYKCheckDataBack* pYKCheckBack)
{
	if(m_pProtocol) m_pProtocol->IVYKCheckBack(wFacID, wDevID, pYKCheckBack);
}

bool AppCommu::IAskFile(tagFileAskInf* pFileInf)
{
	if(m_pProtocol && m_pProtocol->m_pFileTrans) 
		return m_pProtocol->m_pFileTrans->AskFile(pFileInf);
	else return false;
}

void AppCommu::ISCYKRunCMD(ushort wStaAddr, ushort wDevAddr, ushort wGIN, bool bRet)
{
	if(m_pProtocol) m_pProtocol->ISCYKRunCMD(wStaAddr, wDevAddr, wGIN, bRet);
}

void AppCommu::IDoSftSwitch(ushort wStaAddr, ushort wDevAddr, ushort wGIN, bool bVal, bool bIsOtherYK)
{
	if(m_pProtocol) m_pProtocol->IDoSftSwitch(wStaAddr, wDevAddr, wGIN, bVal, bIsOtherYK);
}

//----------------------返回函数----------------
void AppCommu::RGenDataEnterDB(SGrcDataRec* pGrcRec, EPreDefinedGroupType wGroupType)
{

}

void AppCommu::RGroupEnterDB(SGroupEnterDB* pGroupEnter)
{
	if(m_lpProCallBack == NULL) return;
	m_lpProCallBack(PROMSG_GROUPENTERDB, (char* )pGroupEnter, sizeof(SGroupEnterDB));
}

void AppCommu::RDeviceAddOrDel(SDeviceAddOrDel* pDeviceAdd)
{
	if(m_lpProCallBack == NULL) return;
	m_lpProCallBack(PROMSG_DEVICEADDORDEL, (char* )pDeviceAdd, sizeof(SDeviceAddOrDel));
}

void AppCommu::RSyncTimeEnterDB(QDateTime* pFileTime)
{
	if(m_lpProCallBack == NULL) return;
    m_lpProCallBack(PROMSG_SYNCTIME, (char* )pFileTime, sizeof(QDateTime));
	
}

void AppCommu::RNotifyGenCMD(SGenCMDNotify* pGenNotify)
{
	if(m_lpProCallBack == NULL) return;
	m_lpProCallBack(PROMSG_GENCMDNOTIFY, (char* )pGenNotify, sizeof(SGenCMDNotify));
}

void AppCommu::RNotifyHisCMD(SHisCMDNotify* pHisNotify)
{
	if(m_lpProCallBack == NULL) return;
	m_lpProCallBack(PROMSG_HISCMDNOTIFY, (char* )pHisNotify, sizeof(SHisCMDNotify));
}

void AppCommu::RNotifyYKYTCMD(SYKYTCMDNotify* pYKYT)
{
	if(m_lpProCallBack == NULL) return;
	m_lpProCallBack(PROMSG_YKYTNOTIFY, (char* )pYKYT, sizeof(SYKYTCMDNotify));
}

void AppCommu::RSCRunInf(SSCCRUN_INF* pSCRunInf)
{
	if(m_lpProCallBack == NULL) return;
	m_lpProCallBack(PROMSG_SCRUNOPINF, (char* )pSCRunInf, sizeof(SCCRUN_INF));
}

bool AppCommu::RNotifyVirtualYK(ushort wVDevID, ushort wGIN, uchar byCMD)
{
	SVYKYTData* pYK = new SVYKYTData;
	pYK->wVDevID = wVDevID;
	pYK->byCommandType = byCMD;
	pYK->wGIN = wGIN;
	pYK->byExecType = (uchar) enumExecTypeExecute;
	if(m_lpProCallBack == NULL) return false;
    qint32 lRet = m_lpProCallBack(PROMSG_VIRTUALYK, (char* )pYK, sizeof(SYKYTData));
	delete pYK;
	if(lRet == -1)return false;
	else return true;
}

bool AppCommu::DtbTableHandle(SOpDtbTable* pOpDtbTable)
{
	if(m_lpProCallBack == NULL) return false;
    qint32 lRet = m_lpProCallBack(PROMSG_DTBTABLEHANDLE, (char* )pOpDtbTable, sizeof(SOpDtbTable));
	if(lRet == -1) return false;
	else return true;
}

bool AppCommu::DtbFileEnterDB(SOpDtbFile* pDtbFile)
{
	if(m_lpProCallBack == NULL) return false;
    qint32 lRet = m_lpProCallBack(PROMSG_DTBFILEENTERDB, (char* )pDtbFile, sizeof(SOpDtbFile));
	if(lRet == -1) return false;
	else return true;
}

void AppCommu::NOFFANTableEnterDB(SNOFFANTable* pNOFFanTable)
{
	if(m_lpProCallBack == NULL) return ;
	m_lpProCallBack(PROMSG_NOFFANTABLE, (char* )pNOFFanTable, sizeof(SNOFFANTable));
}

void AppCommu::GetFactoryName(QString& strFactoryName, ushort wFacNo)
{
	if(m_lpProCallBack == NULL) return;
	SGetFacName* pFacName = new SGetFacName;
	pFacName->wFacID = wFacNo;
	m_lpProCallBack(PROMSG_GETFACTORYNAME, (char* )pFacName, sizeof(SGetFacName));
	strFactoryName = pFacName->strFacName;
	delete pFacName;
}

void AppCommu::GetVirtualGroup(SGetVirtualGroup* pGroup)
{
	if(m_lpProCallBack == NULL) return ;
	m_lpProCallBack(PROMSG_GETVIRTUALGROUP, (char* )pGroup, sizeof(SGetVirtualGroup));	
}

ushort AppCommu::GetVirtualYXNum(ushort wVDevID)
{
	if(m_lpProCallBack == NULL) return 0;
	ushort wYXNum = wVDevID;
	m_lpProCallBack(PROMSG_GETVIRTUALALLYXNUM, (char* )&wYXNum, sizeof(ushort));
	return wYXNum;
}

uchar AppCommu::GetVirtualGroupYXNum(ushort wVDevAddr, uchar byGroup)
{
	if(m_lpProCallBack == NULL) return 0;
	ushort pBuf[2];
	pBuf[0] = wVDevAddr;
	pBuf[1] = byGroup;
	m_lpProCallBack(PROMSG_GETVIRTUALGRPYXNUM, (char* )pBuf, 4);
	return *((uchar* )pBuf);
}

void AppCommu::GetVirtualYXAll(SGetVirtualYXAll* pYXAll)
{
	if(m_lpProCallBack == NULL) return;
	m_lpProCallBack(PROMSG_GETVIRTUALYXALL, (char* )pYXAll, sizeof(SGetVirtualYXAll));
	
}

void AppCommu::GetVirtualYX(SVirtualYXSend* pYX)
{
	if(m_lpProCallBack == NULL) return;
	m_lpProCallBack(PROMSG_GETVIRTUALYX, (char* )pYX, sizeof(SVirtualYXSend));
}

ushort AppCommu::GetVirtualYCNum(ushort wVDevID)
{
	if(m_lpProCallBack == NULL) return 0;
	ushort wYCNum = wVDevID;
	m_lpProCallBack(PROMSG_GETVIRTUALALLYCNUM, (char* )&wYCNum, sizeof(ushort));
	return wYCNum;
}

void AppCommu::GetVirtualYCAll(SGetVirtualYCAll* pYCAll)
{
	if(m_lpProCallBack == NULL) return;
	m_lpProCallBack(PROMSG_GETVIRTUALYCALL, (char* )pYCAll, sizeof(SGetVirtualYCAll));
}

void AppCommu::RVYKCheck(ushort wFacID, ushort wDevID, ushort wVDevID, SYKCheckDataID* pYKCheckID)
{
	SYKCheckCMD* pChkCMD = new SYKCheckCMD;
	pChkCMD->wSFacAddr = wFacID;
	pChkCMD->wSDevAddr = wDevID;
	pChkCMD->wVDevAddr = wVDevID;
	pChkCMD->wYKPointFacAddr = pYKCheckID->wFacAddr;
	pChkCMD->wYKPointDevAddr = pYKCheckID->wDevAddr;
	pChkCMD->wYKPointGIN = pYKCheckID->wGIN;
	pChkCMD->byYKPointOPType = pYKCheckID->byOPType;

	m_lpProCallBack(PROMSG_YKCHECK, (char* )pChkCMD, sizeof(SYKCheckCMD));
	delete pChkCMD;
}

void AppCommu::RFileAsk(tagFileAskInf* pFileInf)
{
	m_lpProCallBack(PROMSG_FILEASKRTN, (char* )pFileInf, sizeof(tagFileAskInf));
}

void AppCommu::YKCheckBack(SYKCheckDataBack* pYKCheckBck)
{
	if(m_lpProCallBack) m_lpProCallBack(PROMSG_YKCHECKBACK, (char* )pYKCheckBck, sizeof(SYKCheckDataBack));
}

//----------------------工具函数
void AppCommu::ACETimeToDNTime(QDateTime* pACETime, DNSYSTEMTIME* pSysTime)
{
    QDateTime dtTmp = *pACETime;
	QDate dTmp = dtTmp.date();
	QTime tTmp = dtTmp.time();
	pSysTime->wYear = dTmp.year();
	pSysTime->wMonth = dTmp.month();
	pSysTime->wDay = dTmp.day();
	pSysTime->wHour = tTmp.hour();
	pSysTime->wMinute = tTmp.minute();
	pSysTime->wSecond = tTmp.second();
    pSysTime->wMilliseconds = tTmp.msec();
}

void AppCommu::IShortTimeToSysTime(SShortTime* pShortTime, DNSYSTEMTIME* pSysTime)
{
	DNSYSTEMTIME tmpSysTime;
	tmpSysTime.wYear = pSysTime->wYear;
	tmpSysTime.wDay = pSysTime->wDay;
	tmpSysTime.wMonth = pSysTime->wMonth;
	tmpSysTime.wHour = (ushort )pShortTime->byHour;
	tmpSysTime.wMinute = (ushort )pShortTime->byMinute;
	tmpSysTime.wSecond = pShortTime->wMilliseconds/1000;
	tmpSysTime.wMilliseconds = pShortTime->wMilliseconds%1000;

	if(tmpSysTime.wHour==0 && pSysTime->wHour==23)
	{
		//	后延一天
		tmpSysTime.wDay++;
        if((tmpSysTime.wMonth==1 && tmpSysTime.wDay>31)
        || (tmpSysTime.wMonth==2 && (tmpSysTime.wYear%4)==0 && tmpSysTime.wDay>29)
        || (tmpSysTime.wMonth==2 && (tmpSysTime.wYear%4)!=0 && tmpSysTime.wDay>28)
        || (tmpSysTime.wMonth==3 && tmpSysTime.wDay>31)
        || (tmpSysTime.wMonth==4 && tmpSysTime.wDay>30)
        || (tmpSysTime.wMonth==5 && tmpSysTime.wDay>31)
        || (tmpSysTime.wMonth==6 && tmpSysTime.wDay>30)
        || (tmpSysTime.wMonth==7 && tmpSysTime.wDay>31)
        || (tmpSysTime.wMonth==8 && tmpSysTime.wDay>31)
        || (tmpSysTime.wMonth==9 && tmpSysTime.wDay>30)
        || (tmpSysTime.wMonth==10 && tmpSysTime.wDay>31)
        || (tmpSysTime.wMonth==11 && tmpSysTime.wDay>30)
        || (tmpSysTime.wMonth==12 && tmpSysTime.wDay>31) )
		{
			tmpSysTime.wDay = 1;
			tmpSysTime.wMonth++;
			if(tmpSysTime.wMonth>12)
			{
				tmpSysTime.wMonth = 1;
				if(tmpSysTime.wYear==99)	tmpSysTime.wYear = 0;
				else					tmpSysTime.wYear++;
			}
		}
	}
	if(tmpSysTime.wHour==0 && pSysTime->wHour==23)
	{
			//	提前一天
		tmpSysTime.wDay--;
		if(tmpSysTime.wDay==0)
		{
			tmpSysTime.wMonth--;
			if(tmpSysTime.wMonth==0)
			{
				if(tmpSysTime.wYear==0)	tmpSysTime.wYear = 99;
				else	tmpSysTime.wYear--;
				tmpSysTime.wMonth = 12;
			}
			if(tmpSysTime.wMonth==1)  tmpSysTime.wDay=31;
			if(tmpSysTime.wMonth==2 && (tmpSysTime.wYear%4)==0)  tmpSysTime.wDay=29;
			if(tmpSysTime.wMonth==2 && (tmpSysTime.wYear%4)!=0)  tmpSysTime.wDay=28;
			if(tmpSysTime.wMonth==3)  tmpSysTime.wDay=31;
			if(tmpSysTime.wMonth==4)  tmpSysTime.wDay=30;
			if(tmpSysTime.wMonth==5)  tmpSysTime.wDay=31;
			if(tmpSysTime.wMonth==6)  tmpSysTime.wDay=30;
			if(tmpSysTime.wMonth==7)  tmpSysTime.wDay=31;
			if(tmpSysTime.wMonth==8)  tmpSysTime.wDay=31;
			if(tmpSysTime.wMonth==9)  tmpSysTime.wDay=30;
			if(tmpSysTime.wMonth==10) tmpSysTime.wDay=31;
			if(tmpSysTime.wMonth==11) tmpSysTime.wDay=30;
			if(tmpSysTime.wMonth==12) tmpSysTime.wDay=31;
		}
	}
	pSysTime->wYear = tmpSysTime.wYear;
	pSysTime->wDay = tmpSysTime.wDay;
	pSysTime->wMonth = tmpSysTime.wMonth;
	pSysTime->wHour = tmpSysTime.wHour;
	pSysTime->wMinute = tmpSysTime.wMinute;
	pSysTime->wSecond = tmpSysTime.wSecond;
	pSysTime->wMilliseconds = tmpSysTime.wMilliseconds;
}

bool AppCommu::IShortTimeToFileTime(SShortTime* pShortTime, QDateTime* pFileTime)
{
    QDateTime tdTmp = QDateTime::currentDateTime();
	QDate dTmp = tdTmp.date();
	QTime tTmp = tdTmp.time();
	DNSYSTEMTIME sysTime;
	sysTime.wYear = dTmp.year();
	sysTime.wMonth = dTmp.month();
	sysTime.wDay = dTmp.day();
	sysTime.wHour = tTmp.hour();
	sysTime.wMinute = tTmp.minute();
	sysTime.wSecond = tTmp.second();
	sysTime.wMilliseconds = tTmp.msec();
	IShortTimeToSysTime(pShortTime, &sysTime);
	if(sysTime.wYear >= 2038 || sysTime.wYear <= 1970 || sysTime.wMonth > 12
		|| sysTime.wDay > 31 || sysTime.wHour > 24 || sysTime.wMinute > 60 
		|| sysTime.wMilliseconds > 1000 || sysTime.wMonth == 0 || sysTime.wDay == 0) return false;
	dTmp = QDate(sysTime.wYear, sysTime.wMonth, sysTime.wDay);
	tTmp = QTime(sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
	tdTmp = QDateTime(dTmp, tTmp);
    *pFileTime = tdTmp;
	return true;
}

bool AppCommu::IShortTimeToFileTime(SShortTime* pShortTime, QDateTime* pFileTime, ushort* pMillisecond)
{
    QDateTime tdTmp = QDateTime::currentDateTime();
	QDate dTmp = tdTmp.date();
	QTime tTmp = tdTmp.time();
	DNSYSTEMTIME sysTime;
	sysTime.wYear = dTmp.year();
	sysTime.wMonth = dTmp.month();
	sysTime.wDay = dTmp.day();
	sysTime.wHour = tTmp.hour();
	sysTime.wMinute = tTmp.minute();
	sysTime.wSecond = tTmp.second();
	sysTime.wMilliseconds = tTmp.msec();
	IShortTimeToSysTime(pShortTime, &sysTime);
	*pMillisecond = sysTime.wMilliseconds;
	sysTime.wMilliseconds = 0;
	if(sysTime.wYear >= 2038 || sysTime.wYear <= 1970 || sysTime.wMonth > 12
		|| sysTime.wDay > 31 || sysTime.wHour > 24 || sysTime.wMinute > 60 
		|| sysTime.wMilliseconds > 1000 || sysTime.wMonth == 0 || sysTime.wDay == 0) return false;
	
	dTmp = QDate(sysTime.wYear, sysTime.wMonth, sysTime.wDay);
	tTmp = QTime(sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
	tdTmp = QDateTime(dTmp, tTmp);
    *pFileTime = tdTmp;
	return true;
}

bool AppCommu::ILongTimeToFileTime(SLongTime* pLongTime, QDateTime* pFileTime)
{
	DNSYSTEMTIME sysTime;
	sysTime.wDay = (ushort) pLongTime->byDay;
	sysTime.wDayOfWeek = (ushort) pLongTime->byDayOfWeek;
	sysTime.wMonth = (ushort) pLongTime->byMonth;
	sysTime.wYear = ((ushort) pLongTime->byYear)+2000;
	sysTime.wHour = (ushort) pLongTime->byHour;
	sysTime.wMinute = (ushort) pLongTime->byMinute;
	sysTime.wSecond = (ushort) (pLongTime->wMilliseconds/1000);
	sysTime.wMilliseconds = (ushort) (pLongTime->wMilliseconds%1000);
	if(sysTime.wYear >= 2038 || sysTime.wYear <= 1970 || sysTime.wMonth > 12
		|| sysTime.wDay > 31 || sysTime.wHour > 24 || sysTime.wMinute > 60 
		|| sysTime.wMilliseconds > 1000 || sysTime.wMonth == 0 || sysTime.wDay == 0) return false;
	QDate dTmp = QDate(sysTime.wYear, sysTime.wMonth, sysTime.wDay);
	QTime tTmp = QTime(sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
	QDateTime tdTmp = QDateTime(dTmp, tTmp);
    *pFileTime = tdTmp;
	return true;
}

bool AppCommu::ILongTimeToFileTime(SLongTime* pLongTime, QDateTime* pFileTime, ushort* pMillisecond)
{
	DNSYSTEMTIME sysTime;
	sysTime.wDay = (ushort) pLongTime->byDay;
	sysTime.wDayOfWeek = (ushort) pLongTime->byDayOfWeek;
	sysTime.wMonth = (ushort) pLongTime->byMonth;
	sysTime.wYear = ((ushort) pLongTime->byYear)+2000;
	sysTime.wHour = (ushort) pLongTime->byHour;
	sysTime.wMinute = (ushort) pLongTime->byMinute;
	sysTime.wSecond = (ushort) (pLongTime->wMilliseconds/1000);
	sysTime.wMilliseconds = 0;
	*pMillisecond = (ushort) (pLongTime->wMilliseconds%1000);
	if(sysTime.wYear >= 2038 || sysTime.wYear <= 1970 || sysTime.wMonth > 12
		|| sysTime.wDay > 31 || sysTime.wHour > 24 || sysTime.wMinute > 60 
		|| sysTime.wMilliseconds > 1000 || sysTime.wMonth == 0 || sysTime.wDay == 0) return false;
	QDate dTmp = QDate(sysTime.wYear, sysTime.wMonth, sysTime.wDay);
	QTime tTmp = QTime(sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
	QDateTime tdTmp = QDateTime(dTmp, tTmp);
    *pFileTime = tdTmp;
	return true;
}

ushort AppCommu::CharToShort(uchar* pSrc)
{
    ushort wRet = (ushort) pSrc[0] + (ushort) pSrc[1] * 256;
	return wRet;
}

quint32 AppCommu::CharToLong(uchar* pSrc)
{
    quint32 wRet = (quint32) pSrc[0] + ((quint32) pSrc[1]) * 256
                    + ((quint32) pSrc[2]) * 256 * 256 + ((quint32)pSrc[3]) * 256 * 256 * 256;
	return wRet;
}

float AppCommu::CharToFloat(uchar* pSrc)
{
	float fTmp;
    if(QSysInfo::ByteOrder == QSysInfo::BigEndian){
        uchar tmp[4];
        tmp[0] = pSrc[3];
        tmp[1] = pSrc[2];
        tmp[2] = pSrc[1];
        tmp[3] = pSrc[0];
        memcpy(&fTmp, tmp, 4);
    }
    else{
        memcpy(&fTmp, pSrc, 4);
    }
	return fTmp;
}

AppCommu::~AppCommu()
{
	if(m_pProtocol) delete m_pProtocol;
}

void AppCommu::timerEvent( QTimerEvent * e)
{
	if(e->timerId() == m_dwProtocolTimerID) {
		if(m_pProtocol) m_pProtocol->OnTimer();
	}
}

uchar AppCommu::GetGroupByGroupType(ushort wFacAddr, ushort wDevAddr, EPreDefinedGroupType wType, uchar* pGroupValue, uchar byBufLen)
{
	if(m_pProtocol == NULL) return 0;
	SCommandID keyCMD;
	keyCMD.wFacNo = wFacAddr;
	keyCMD.wLogicalNodeId = wDevAddr;
	keyCMD.wGroupType = wType;
	return m_pProtocol->GetGroupByGroupType(keyCMD, pGroupValue, byBufLen);
}

void AppCommu::IGetDeviceNetState(ushort wFacID, ushort wDevID, uchar& byNetState1, uchar& byNetState2)
{
	if(m_pProtocol) m_pProtocol->GetDeviceNetState(wFacID, wDevID, byNetState1, byNetState2);
	else {
		byNetState1 = 0;
		byNetState2 = 0;
	}
}


void AppCommu::GetProtectDealFlag(ushort &wDealFlag, ushort wFacID, ushort wProtectID)
{
	if(m_lpProCallBack == NULL) return;
	SGetProtectDealFlag* pProtectDealFlag = new SGetProtectDealFlag;
	pProtectDealFlag->wFacID=wFacID;
	pProtectDealFlag->wDevID=wProtectID;
	m_lpProCallBack(PROMSG_GETPROTECTDEALFLAG, (char* )pProtectDealFlag, sizeof(SGetProtectDealFlag));
	wDealFlag=pProtectDealFlag->wDealFlag;
	delete pProtectDealFlag; 
}

bool AppCommu::AutoSendWave(ushort wFacID, ushort wProtectID)
{
    Q_UNUSED(wFacID);
    Q_UNUSED(wProtectID);
	//如果没有设置回调，默认是所有波形都主动上传的
	if(m_lpProCallBack == NULL) 
	{
		return true;
	}
    return true;
}

void AppCommu::AddMsgToShowList(tagShowMsg* pShowMsg)
{
	g_mtxMonitor.lock();
	if(g_plShowItem.count() > MAXSHOWBUF) {
        tagShowMsg* pShowMsg = g_plShowItem.takeAt(0);
		delete pShowMsg->pShowMsg;
		delete pShowMsg;
	}
	g_plShowItem.append(pShowMsg);
	g_mtxMonitor.unlock();
}

void Protocol_Monitor()
{
}

