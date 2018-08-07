#include "appcommu.h"
#include "protocol.h"
#include "qmutex.h"
#include "qdatetime.h"
#include <QtDebug>
#include <QTextCodec>
#include "edevice.h"

uchar CProtocol::m_byRII = 0;
QMutex	g_mutexProtocolRawDataList;
QMutex	g_mutexProtocolDataUnpack;
QMutex	g_mutexProtocolDbCMD;

/***********************************************************************/
/*							规约解释开始							   */
/***********************************************************************/
CProtocol::CProtocol()
{
	m_pFileTrans = new CFileTrans(this);
	m_wSyncTimer = OVERTIMER_SYNCTIMER;
	m_pCurYKInf = NULL;

    m_gbkCodec = QTextCodec::codecForName("GBK");
}

CProtocol::~CProtocol()
{
	if(m_pFileTrans) delete m_pFileTrans;
    while(m_plDbCommandWait.count()>0){
        delete m_plDbCommandWait.takeFirst();
    }
    while(m_plGenCommand.count()>0){
        delete m_plGenCommand.takeFirst();
    }
    while(m_plHisCommand.count()>0){
        delete m_plHisCommand.takeFirst();
    }
    while(m_plDisturbItem.count()>0){
        delete m_plDisturbItem.takeFirst();
    }
    while(m_plYKYTItem.count()>0){
        delete m_plYKYTItem.takeFirst();
    }
    while(m_plGenWaitForHandle.count()>0){
        delete m_plGenWaitForHandle.takeFirst();
    }
    while(m_plRawWaitForHandle.count()>0){
        delete m_plRawWaitForHandle.takeFirst();
    }
    while(m_plDeviceDstb.count()>0){
        delete m_plDeviceDstb.takeFirst();
    }

	if(m_pCurYKInf) delete m_pCurYKInf;
}

/********************************************************************/
/*						发送部分									*/
/********************************************************************/
void CProtocol::ISendTimeSync(ushort wFacID, QDateTime tTime)
{
	if(m_wSyncTimer) return;
	uchar asdu[13];
	asdu[ASDU_TYPE] = TYPC_TIMESYNC;
	asdu[ASDU_VSQ] = VSQ_COMMON1;
	asdu[ASDU_COT] = COTC_TIMESYNC;
	asdu[ASDU_ADDR] = 255;
	asdu[ASDU_FUN] = 255;
	asdu[ASDU_INF] = 0;
    QDateTime dtTmp = tTime;
	QDate dTmp = dtTmp.date();
	QTime tTmp = dtTmp.time();
	uchar* lTime = &asdu[6];
    ushort wMsec = tTmp.msec() + tTmp.second() * 1000;
	lTime[LONGTIME_MSEC] = wMsec % 256;
	lTime[LONGTIME_MSEC+1] = wMsec / 256;
	lTime[LONGTIME_MIN] = tTmp.minute();
	lTime[LONGTIME_HOUR] = tTmp.hour();
	lTime[LONGTIME_DAY] = dTmp.day();
	lTime[LONGTIME_DAY] |= (dTmp.dayOfWeek() << 5);
	lTime[LONGTIME_MON] = dTmp.month();
	lTime[LONGTIME_YEAR] = dTmp.year();
	AppCommu::Instance()->ProtocolNetBroadcast(wFacID, 0xffff, (char* )asdu, 13);
}

//写包装好条目的值
void CProtocol::IGenWriteAllItem(ushort wFacNo, ushort wLogicalNodeID,
						 EPreDefinedGroupType wGroupType, ushort wNum, ushort wLen, char* pDataSnd)
{
	if(!((wGroupType == enumGroupTypeDevParam) || (wGroupType == enumGroupTypeSettingArea) 
		|| (wGroupType == enumGroupTypeSetting) || (wGroupType == enumGroupTypeSoftSwitch)
		|| (wGroupType == enumGroupTypeSetInf))) return;
	SCommandID keyCMD;
	keyCMD.wFacNo = wFacNo;
	keyCMD.wLogicalNodeId = wLogicalNodeID;
	keyCMD.wGroupType = wGroupType;
	CGenCommand* pCMD = GetGenCMD(keyCMD, false);
	if(pCMD == NULL) {		//有相同命令正在运行，放到队列缓冲中
		pCMD = new CGenCommand(keyCMD);
		pCMD->m_bIsReadType = false;
		CGenWriteBuf* pWriteBuf = new CGenWriteBuf();
		pCMD->m_pGenBuf = pWriteBuf;
		if(PutToWriteBuf(pWriteBuf, wNum, wLen, pDataSnd) == RETERROR){
			NotifyGenCMD(pCMD, false, false);
			delete pCMD;
		}
		else AddCMDToWaitList(pCMD, OVERTIMER_DBGENWAITCMD);
		return;	
	}
	CGenWriteBuf* pWriteBuf = new CGenWriteBuf();
	pCMD->m_pGenBuf = pWriteBuf;
	if(PutToWriteBuf(pWriteBuf, wNum, wLen, pDataSnd) == RETERROR){
		NotifyGenCMD(pCMD, false, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	pCMD->m_dwCount = 0;
	SGenWriteCMD(pCMD);
}

//写某个目录指定条目的值
void CProtocol::IGenWriteItem(ushort wFacNo, ushort wLogicalNodeID,
					  EPreDefinedGroupType wGroupType,  ushort wNum, SGENDataItem* paSetting, bool bAuto)
{
	if(!((wGroupType == enumGroupTypeDevParam) || (wGroupType == enumGroupTypeSettingArea) 
		|| (wGroupType == enumGroupTypeSetting) || (wGroupType == enumGroupTypeSoftSwitch)
		|| (wGroupType == enumGroupTypeSetInf))) return;
	SCommandID keyCMD;
	keyCMD.wFacNo = wFacNo;
	keyCMD.wLogicalNodeId = wLogicalNodeID;
	keyCMD.wGroupType = wGroupType;
	CGenCommand* pCMD = GetGenCMD(keyCMD, false);
	if(pCMD == NULL) {		//有相同命令正在运行，放到队列缓冲中
		pCMD = new CGenCommand(keyCMD);
		pCMD->m_bIsReadType = false;
		pCMD->m_bAuto = bAuto;
		CGenWriteBuf* pWriteBuf = new CGenWriteBuf();
		pCMD->m_pGenBuf = pWriteBuf;
		if(PutToWriteBuf(pWriteBuf, wNum, paSetting) == RETERROR){
			NotifyGenCMD(pCMD, false, false);
			delete pCMD;
		}
		else AddCMDToWaitList(pCMD, OVERTIMER_DBGENWAITCMD);
		return;	
	}
	pCMD->m_bAuto = bAuto;
	CGenWriteBuf* pWriteBuf = new CGenWriteBuf();
	pCMD->m_pGenBuf = pWriteBuf;
	if(PutToWriteBuf(pWriteBuf, wNum, paSetting) == RETERROR){
		NotifyGenCMD(pCMD, false, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	pCMD->m_dwCount = 0;
	SGenWriteCMD(pCMD);
}

qint32 CProtocol::PutToWriteBuf(CGenWriteBuf* pWriteBuf, ushort wNum, ushort wLen, char* pSetting)
{
	ushort wTotalLen = wLen + 4;
	pWriteBuf->m_pGrcSendDataBuf = new uchar[wTotalLen];

	uchar* pDataSnd = (uchar* )pWriteBuf->m_pGrcSendDataBuf;
	pDataSnd[GENDATACOM_FUN] = 254;
	pDataSnd[GENDATACOM_INF] = 249;
	pDataSnd[GENDATACOM_RII] = m_byRII++;
	memcpy(&pWriteBuf->m_pGrcSendDataBuf[4], pSetting, wLen);
	pWriteBuf->m_dwGrcSendDataLen = wTotalLen;
	pWriteBuf->m_dwGrcSendDataIp = 4;
	pWriteBuf->m_wGrcSendDataNOG = wNum;
	pWriteBuf->m_byGrcSendDataLastNGDbCount = 0;
	return RETOK;

}

qint32 CProtocol::PutToWriteBuf(CGenWriteBuf* pWriteBuf, ushort wNum, SGENDataItem* paSetting)
{
	ushort wLen = 4;
	SGENDataItem* paCurSetting;
	ushort i;
	for( i=0; i<wNum; i++){
		paCurSetting = paSetting+i;
		if(paCurSetting->m_byDataType == 2) 
			wLen += (((paCurSetting->m_byDataSize+7)/8)*(paCurSetting->m_byDataNumber));
		else wLen += (paCurSetting->m_byDataSize * paCurSetting->m_byDataNumber);
		wLen += 6;
	}
	if(wLen <= 10)	return RETERROR;

	pWriteBuf->m_pGrcSendDataBuf = new uchar[wLen];

	uchar* pDataSnd = (uchar* )pWriteBuf->m_pGrcSendDataBuf;
	pDataSnd[GENDATACOM_FUN] = 254;
	pDataSnd[GENDATACOM_INF] = 249;
	pDataSnd[GENDATACOM_RII] = m_byRII++;
	uchar* pHead = (uchar* )pDataSnd;
	pHead += 4;
	for(i=0; i<wNum; i++)
	{
		paCurSetting = paSetting+i;
		uchar* pDataRec = (uchar* )pHead;
		pDataRec[GENDATAREC_GIN] = paCurSetting->wGINOrder % 256;
		pDataRec[GENDATAREC_GIN+1] = paCurSetting->wGINOrder / 256;
		pDataRec[GENDATAREC_KOD] = KOD_ACTUALVALUE;
		pDataRec[GENDATAREC_GDD] = paCurSetting->m_byDataType;
		pDataRec[GENDATAREC_GDD+1] = paCurSetting->m_byDataSize;
		pDataRec[GENDATAREC_GDD+2] = paCurSetting->m_byDataNumber;
		uchar dataSize;
		if(paCurSetting->m_byDataType == 2) 
			dataSize = ((paCurSetting->m_byDataSize+7)/8) * (paCurSetting->m_byDataNumber);
		else dataSize = (paCurSetting->m_byDataSize) * (paCurSetting->m_byDataNumber);
		if((paCurSetting->m_Value.size()) < dataSize) return RETERROR;
		for(int j=0; j<dataSize; j++) 
			pDataRec[GENDATAREC_GID+j] = paCurSetting->m_Value[j];
		pHead = pHead + 6 + dataSize;
	}
	pWriteBuf->m_dwGrcSendDataLen = wLen;
	pWriteBuf->m_dwGrcSendDataIp = 4;
	pWriteBuf->m_wGrcSendDataNOG = wNum;
	pWriteBuf->m_byGrcSendDataLastNGDbCount = 0;
	return RETOK;
}

//读某个目录指定条目的所有属性或某个属性
void CProtocol::IGenReadItem(ushort wFacNo, ushort wLogicalNodeID, EPreDefinedGroupType wGroupType,
										EDevReadType byReadType, SGINType* pGIN, ushort wReadNum)
{
	SCommandID	keyCMD;
	keyCMD.wFacNo = wFacNo;
	keyCMD.wLogicalNodeId = wLogicalNodeID;
	keyCMD.wGroupType = wGroupType;
	if( wGroupType == enumGroupTypeCatalog ){
		CGenCommand* pCMDTmp = GetGenCMD(keyCMD, true);
		if(pCMDTmp){
			pCMDTmp->m_bIsReadType = true;
			pCMDTmp->m_dwCount = 0;
			SGenReadCatalog(keyCMD.wFacNo, keyCMD.wLogicalNodeId);
		}
		return ;
	}

	CGenCommand* pCMD = GetGenCMD(keyCMD, true);
	if(pCMD == NULL) {		//命令正在执行
		pCMD = new CGenCommand(keyCMD);
		pCMD->m_bIsReadType = true;
		CGenReadBuf* pReadBuf = new CGenReadBuf();
		pCMD->m_pGenBuf = pReadBuf;
		pReadBuf->m_byRWType = byReadType;
		pReadBuf->m_dwRWInf = byReadType;
		if(PutToReadBuf(pReadBuf, pGIN, wReadNum) == RETERROR){
			NotifyGenCMD(pCMD, true, false);
			delete pCMD;
		}
		else AddCMDToWaitList(pCMD, OVERTIMER_DBGENWAITCMD);
		return;
	}
	if(pCMD->m_pGenBuf) delete pCMD->m_pGenBuf;
	CGenReadBuf* pReadBuf = new CGenReadBuf();
	pCMD->m_pGenBuf = pReadBuf;
	pReadBuf->m_byRWType = byReadType;
	pReadBuf->m_dwRWInf = byReadType;
	if(PutToReadBuf(pReadBuf, pGIN, wReadNum) == RETERROR) {
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
	}
	else SendReadCommand(pCMD);
}

qint32 CProtocol::PutToReadBuf(CGenReadBuf* pReadBuf, SGINType* pGIN, ushort wReadNum)
{
	if(pReadBuf->m_byRWType == enumDevReadAll){//读条目的所有属性
		pReadBuf->m_wGroupSum = wReadNum;
		if(pReadBuf->m_pGroupValue) delete[] pReadBuf->m_pGroupValue;
		pReadBuf->m_pGroupValue = new SGINType[wReadNum];
		for(int i=0; i<wReadNum; i++){
			SGINType* pGINVal = pGIN + i;
			pReadBuf->m_pGroupValue[i].byENTRY = pGINVal->byENTRY;
			pReadBuf->m_pGroupValue[i].byGROUP = pGINVal->byGROUP;
		}
		pReadBuf->m_wGroupCurNo = 0;
		pReadBuf->m_byReadWay = READ_ITEMALLDATA;
		return RETOK;
	}

	uchar byKOD = ReadTypeToKOD(pReadBuf->m_byRWType);		//读条目的某个属性
	if(byKOD == (uchar) RETERROR) return RETERROR;
	pReadBuf->m_pGroupValue = NULL;
	pReadBuf->m_wGroupSum = 0;
	pReadBuf->m_wGroupCurNo = -1;
	pReadBuf->m_byOldRII = m_byRII++;
	pReadBuf->m_wGrcSendDataNOG = wReadNum;
	pReadBuf->m_dwGrcSendDataLen = 3 * wReadNum + 4;
	pReadBuf->m_pGrcSendDataBuf = new uchar[pReadBuf->m_dwGrcSendDataLen];
	pReadBuf->m_byGrcSendDataLastNGDbCount = 0;
	pReadBuf->m_dwGrcSendDataIp = 4;
	uchar* pReq = (uchar* )pReadBuf->m_pGrcSendDataBuf;
	pReq[GENREQCOM_FUN] = 254;
	pReq[GENREQCOM_INF] = 244;
	pReq[GENREQCOM_RII] = pReadBuf->m_byOldRII;
	uchar* pGIR = &pReq[GENREQCOM_GIR];
	for(int i=0; i<wReadNum; i++){
		pGIR[GENID_KOD] = byKOD;
		pGIR[GENID_GIN+1] = (pGIN+i)->byENTRY;
		pGIR[GENID_GIN] = (pGIN+i)->byGROUP;
		pGIR += 3;
	}
	pReadBuf->m_byReadWay = READ_ITEMDATA;
	return RETOK;
}

//读某个目录所有组或指定组的所有条目的所有属性或某个属性
//byGroupNo == -1表示读所有组
void CProtocol::IGenReadItem(ushort wFacNo, ushort wLogicalNodeID, 
					EPreDefinedGroupType wGroupType, EDevReadType byReadType, uchar byGroupNo)
{
	SCommandID	keyCMD;
	keyCMD.wFacNo = wFacNo;
	keyCMD.wLogicalNodeId = wLogicalNodeID;
	keyCMD.wGroupType = wGroupType;
	if( wGroupType == enumGroupTypeCatalog ){
		CGenCommand* pCMDTmp = GetGenCMD(keyCMD, true);
		if(pCMDTmp){
			pCMDTmp->m_bIsReadType = true;
			pCMDTmp->m_dwCount = 0;
			SGenReadCatalog(keyCMD.wFacNo, keyCMD.wLogicalNodeId);
		}
		return ;
	}

	CGenCommand* pCMD = new CGenCommand(keyCMD);
	pCMD->m_bIsReadType = true;
	CGenReadBuf* pReadBuf = new CGenReadBuf();
	pCMD->m_pGenBuf = pReadBuf;
	if(JudgDevIsSendGenCMD(keyCMD.wFacNo, keyCMD.wLogicalNodeId)){
		if(PutToReadBuf(pCMD, byReadType, byGroupNo) == -1){
			NotifyGenCMD(pCMD, true, false);
			delete pCMD;
		}
		else AddCMDToWaitList(pCMD, OVERTIMER_DBGENWAITCMD);
		return;
	}
    qint32 byRet = PutToReadBuf(pCMD, byReadType, byGroupNo);
	if(byRet == -1){
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
		return ;
	}
	else if(byRet == 0)
		AddCMDToWaitList(pCMD, OVERTIMER_DBGENWAITCMD);
	else {
		AddGenCMD(pCMD);
		pReadBuf->m_wGroupCurNo = 0;
		SendReadCommand(pCMD);
	}
}

qint32 CProtocol::PutToReadBuf(CGenCommand* pCMD, uchar byReadType, uchar byGroupNo)
{
	SCommandID keyCMD = pCMD->m_keyCMD;
	if(!pCMD->m_bIsReadType) return -1;
	CGenReadBuf* pReadBuf = (CGenReadBuf* )pCMD->m_pGenBuf;
	pReadBuf->m_byReadWay = READ_ALLITEMDATA;
	pReadBuf->m_byRWType = byReadType;
	pReadBuf->m_dwRWInf = 0;
	SetReadInf(keyCMD.wGroupType, pReadBuf->m_byRWType, pReadBuf->m_dwRWInf);
//组号个数由10个改为40个，用宏表示
	uchar	byGroupValue[MAXGROUPITEM];
	if(byGroupNo == 0xff){
		pReadBuf->m_wGroupSum = GetGroupByGroupType(keyCMD, byGroupValue, MAXGROUPITEM);
	}
	else {
		pReadBuf->m_wGroupSum = 1;
		byGroupValue[0] = GetGroupByGroupType(keyCMD, byGroupNo);//根据第几组得到组号
		if(byGroupValue[0] == (uchar )RETERROR) return -1;
	}
	if(pReadBuf->m_wGroupSum <= 0) {
		pReadBuf->m_wGroupCurNo = -1;
		pReadBuf->m_wGroupSum = 0;
		keyCMD.wGroupType = enumGroupTypeCatalog;
		CGenCommand* pCMDTmp = GetGenCMD(keyCMD, true);
		if(pCMDTmp){
			pCMDTmp->m_bIsReadType = true;
			pCMDTmp->m_dwCount = 0;
			SGenReadCatalog(keyCMD.wFacNo, keyCMD.wLogicalNodeId);
		}
		else {
			pCMDTmp = new CGenCommand(keyCMD);
			pCMDTmp->m_bIsReadType = true;
			AddCMDToWaitList(pCMDTmp, OVERTIMER_DBGENWAITCMD);
		}
		return 0;
	}
	if(pReadBuf->m_pGroupValue) delete[] pReadBuf->m_pGroupValue;
	pReadBuf->m_pGroupValue = new SGINType[pReadBuf->m_wGroupSum];
	for(int i=0; i<pReadBuf->m_wGroupSum; i++){
		pReadBuf->m_pGroupValue[i].byGROUP = byGroupValue[i];
		pReadBuf->m_pGroupValue[i].byENTRY = 0;
	}
	pReadBuf->m_wGroupCurNo = 0;
	return 1;
}

void CProtocol::IYKYTSend(ushort wFacNo, ushort wLogicalNodeID, SYKYTData* pData, bool bSCYK, bool bAuto)
{
	SDevID keyYKYT;
	keyYKYT.wFacNo = wFacNo;
	keyYKYT.wLogicalNodeId = wLogicalNodeID;
	if(bSCYK && pData->byExecType == enumExecTypeCancel) {
		CYKYTItem* pYKItem = FindYKYTItem(keyYKYT);
		if(pYKItem && pYKItem->m_YKData.wGIN == pData->wGIN) {
			pYKItem->m_YKData.byExecType = enumExecTypeCancel;
			pYKItem->m_byOldRII = m_byRII++;
			SendYKYTCommand(pYKItem);
			DeleteYKYTItem(keyYKYT);
			return;
		}
	}
	CYKYTItem* pYKItem = GetYKYTItem(keyYKYT);
	if(pYKItem == NULL){
		pYKItem = new CYKYTItem(keyYKYT);
		pYKItem->m_bSCYK = bSCYK;
		pYKItem->m_bAuto = bAuto;
        memcpy(&pYKItem->m_YKData, pData, sizeof(SYKYTData));
		pYKItem->m_byOldRII = m_byRII++;
		AddCMDToWaitList(pYKItem, AppCommu::Instance()->m_wYKSelTimes + AppCommu::Instance()->m_wYKRunTimes);
		return;
	}
	pYKItem->m_bSCYK = bSCYK;
	pYKItem->m_bAuto = bAuto;
    memcpy(&pYKItem->m_YKData, pData, sizeof(SYKYTData));
	pYKItem->m_byOldRII = m_byRII++;
	SendYKYTCommand(pYKItem);
}

void CProtocol::SendYKYTCommand(CYKYTItem* pYKItem)
{
	SDevID keyYKYT = pYKItem->m_keyYKYT;
//	SCommandID keyCMD;
//	keyCMD.wFacNo = keyYKYT.wFacNo;
//	keyCMD.wLogicalNodeId = keyYKYT.wLogicalNodeId;
//	keyCMD.wGroupType = enumGroupTypeYK;
	if(pYKItem->m_YKData.wGIN == (ushort )RETERROR) return;
	uchar* pDataRec = (uchar* )pYKItem->m_bySndBuf;
	pDataRec[GENDATACOM_FUN] = 254;
	if(pYKItem->m_YKData.byExecType == enumExecTypeSelect)	pDataRec[GENDATACOM_INF] = 249;
	else if(pYKItem->m_YKData.byExecType == enumExecTypeExecute) pDataRec[GENDATACOM_INF] = 250;
	else {
		pYKItem->m_YKData.byExecType = enumExecTypeCancel;
		pDataRec[GENDATACOM_INF] = 251;
	}
	pDataRec[GENDATACOM_RII] = pYKItem->m_byOldRII;
	pDataRec[GENDATACOM_NGD] = 1;
	uchar* GDR = &pDataRec[GENDATACOM_GDR];
	GDR[GENDATAREC_GIN] = pYKItem->m_YKData.wGIN % 256;
	GDR[GENDATAREC_GIN+1] = pYKItem->m_YKData.wGIN / 256;
	GDR[GENDATAREC_KOD] = KOD_ACTUALVALUE;
	GDR[GENDATAREC_GDD+GDDTYPE_NUM] = 1;
	if(!pYKItem->m_bSCYK) GDR[GENDATAREC_GDD+GDDTYPE_SIZ] = 1;
	else GDR[GENDATAREC_GDD+GDDTYPE_SIZ] = 2; 
	bool bOnUp = true;
	if(!pYKItem->m_bSCYK) {
		GDR[GENDATAREC_GDD+GDDTYPE_SIZ] = 1;
		if(pYKItem->m_YKData.byCommandType == enumYKTypeOnUp) bOnUp = true;
		else if(pYKItem->m_YKData.byCommandType == enumYKTypeOffDown) bOnUp = false;
		else return;
		if(pYKItem->m_YKData.byOpType > YK_TYPE_HH) return;
		if(pYKItem->m_YKData.byOpType == YK_TYPE_NORMAL) {
			GDR[GENDATAREC_GDD] = 9;
			if(bOnUp) GDR[GENDATAREC_GID] = DCO_ON;
			else GDR[GENDATAREC_GID] = DCO_OFF;
		}
		else {
			GDR[GENDATAREC_GDD] = 206;
			if(bOnUp) GDR[GENDATAREC_GID] = DCO_ON << 3;
			else GDR[GENDATAREC_GID] = DCO_OFF << 3; 
			GDR[GENDATAREC_GID] |= pYKItem->m_YKData.byOpType;
			if(pYKItem->m_YKData.byExecType == enumExecTypeSelect)	GDR[GENDATAREC_GID] |= 0x80;
			else if(pYKItem->m_YKData.byExecType == enumExecTypeExecute) GDR[GENDATAREC_GID] &= 0x3f;
			else GDR[GENDATAREC_GID] |= 0x40;
		}
		SendAsdu10(keyYKYT.wFacNo, keyYKYT.wLogicalNodeId, 11, pYKItem->m_bySndBuf);
	}
	else {
		GDR[GENDATAREC_GDD+GDDTYPE_SIZ] = 2;
		GDR[GENDATAREC_GDD] = 3;
		GDR[GENDATAREC_GID] = pYKItem->m_YKData.byCommandType % 256;
		GDR[GENDATAREC_GID] = pYKItem->m_YKData.byCommandType / 256;
		pYKItem->m_bSCWaitForChk = true;
		SendAsdu10(keyYKYT.wFacNo, keyYKYT.wLogicalNodeId, 12, pYKItem->m_bySndBuf);
	}
}

//下发扰动数据请求命令
bool CProtocol::IDistDataTrans(ushort wFacNo, ushort wLogicalNodeId, CDistTable* pDstTable)
{
	if(pDstTable == NULL) return false;
	SDevID keyDTB;
	keyDTB.wFacNo = wFacNo;
	keyDTB.wLogicalNodeId = wLogicalNodeId;
	CDevice* pDevice = FindDeviceDstb(wFacNo, wLogicalNodeId);
	if(pDevice == NULL) return false;
	if(pDevice->m_byGetChannelNameTimes) return false;

	CDisturbItem* pDisturbItem = GetDtbItem(keyDTB);
	if(pDisturbItem == NULL) return false;
	pDisturbItem->m_pDtbTable = pDstTable;
	pDevice->m_bIsTransDTB = true;
	SendDisturbCmd(pDisturbItem, TOO_FAULT_SELECT, 0, 0);
	return true;
}

void CProtocol::IRelayReset(ushort wFacNo, ushort wLogicalNodeID)
{
	uchar asdu[8];
	asdu[ASDU_TYPE] = TYPC_COMMAND;
	asdu[ASDU_VSQ] = VSQ_COMMON1;
	asdu[ASDU_COT] = COTC_COMMAND;
	asdu[ASDU_ADDR] = wLogicalNodeID % 256;
	asdu[ASDU_FUN] = 255;
	asdu[ASDU_INF] = 19;
	asdu[ASDU20_CC] = DCO_ON;
	asdu[ASDU20_RII] = m_byRII++;
	AppCommu::Instance()->ProtocolNetBroadcast(wFacNo, 0xffff, (char* )asdu, 8);
}

bool CProtocol::IJudgDeviceIsOnLine(ushort wFacNo, ushort wDevID)
{
    foreach (CDevice* pDev, m_plDeviceDstb) {
        if(pDev->m_wFacNo == wFacNo && pDev->m_wDevID == wDevID) {
            return true;
        }
    }
	return false;
}

void CProtocol::SendReadCommand(CGenCommand* pCMD)
{
	SCommandID keyCMD = pCMD->m_keyCMD;
	pCMD->m_dwCount = 0;
	if(!pCMD->m_bIsReadType) {
		NotifyGenCMD(pCMD, false, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	if(pCMD->m_keyCMD.wGroupType == enumGroupTypeCatalog){
		SGenReadCatalog(pCMD->m_keyCMD.wFacNo, pCMD->m_keyCMD.wLogicalNodeId);
		return;
	}
	CGenReadBuf* pReadBuf = (CGenReadBuf* )pCMD->m_pGenBuf;
	switch(pReadBuf->m_byReadWay){
	case READ_ALLITEMDATA:
		SGenReadAllItemData(pCMD);
		break;
	case READ_ITEMALLDATA:
		SGenReadItemAllData(pCMD);
		break;
	case READ_ITEMDATA:
		SGenReadItemData(pCMD);
		break;
	default:
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
		break;
	}
	return;
}

void CProtocol::SGenWriteCMD(CGenCommand* pCMD)
{
	SCommandID keyCMD = pCMD->m_keyCMD;
	if(pCMD->m_bIsReadType || pCMD->m_pGenBuf == NULL) {
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	CGenWriteBuf* pWriteBuf = (CGenWriteBuf* )pCMD->m_pGenBuf;
	uchar byCmdBufLen = 4;
	uchar* pHead = (uchar* )&pWriteBuf->m_pGrcSendDataBuf[pWriteBuf->m_dwGrcSendDataIp];
	uchar* pHeadBak = pHead;
	ushort ReservedNOG = pWriteBuf->m_wGrcSendDataNOG;
	for(ushort i=0; i<(ReservedNOG+1); i++)
	{
		uchar* pDataRec = (uchar* )pHead;
		if(i < ReservedNOG){
			uchar byDataSize;
			uint nType = pDataRec[GENDATAREC_GDD];
			uint nSize = pDataRec[GENDATAREC_GDD + GDDTYPE_SIZ];
			uint nNum = pDataRec[GENDATAREC_GDD + GDDTYPE_NUM];
			if(nType==2)	// 成组位串
				byDataSize = 6 + ((nSize+7)/8) * nNum;
			else byDataSize = nSize * nNum + 6;
			byCmdBufLen += byDataSize;
			pHead += byDataSize;
			pWriteBuf->m_dwGrcSendDataIp += byDataSize;/*上次写到的位置*/
			pWriteBuf->m_wGrcSendDataNOG--;			//表示还剩多少个未写
		}
		if(byCmdBufLen>=128 || i==ReservedNOG) {
			if(pWriteBuf->m_pGrcWriteBuf) delete[] pWriteBuf->m_pGrcWriteBuf;
			pWriteBuf->m_pGrcWriteBuf = new uchar[byCmdBufLen];
			pWriteBuf->m_dwGrcWriteBufLen = byCmdBufLen;

			uchar* ppack = (uchar* )pWriteBuf->m_pGrcWriteBuf;
			memcpy(pWriteBuf->m_pGrcWriteBuf, pWriteBuf->m_pGrcSendDataBuf, 4);
			if(byCmdBufLen-4>0) memcpy(pWriteBuf->m_pGrcWriteBuf+4, pHeadBak, byCmdBufLen-4);

			if(byCmdBufLen>=128)	ppack[GENDATACOM_NGD] = i+1;
			else ppack[GENDATACOM_NGD] = (uchar )i;
			if(pWriteBuf->m_byGrcSendDataLastNGDbCount) ppack[GENDATACOM_NGD] |= 0x40;
			else ppack[GENDATACOM_NGD] &= 0xbf; 
			pWriteBuf->m_byGrcSendDataLastNGDbCount = 1-pWriteBuf->m_byGrcSendDataLastNGDbCount;
			if(pWriteBuf->m_wGrcSendDataNOG) ppack[GENDATACOM_NGD] |= 0x80;
			else ppack[GENDATACOM_NGD] &= 0x7f;
			SendAsdu10(keyCMD.wFacNo, keyCMD.wLogicalNodeId, byCmdBufLen, pWriteBuf->m_pGrcWriteBuf);
			break;
		}
	}
}

//读某个组指定条目的所有属性
void CProtocol::SGenReadItemAllData(CGenCommand* pCMD)
{
	SCommandID keyCMD = pCMD->m_keyCMD;
	if(!pCMD->m_bIsReadType) {
		NotifyGenCMD(pCMD, false, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	CGenReadBuf* pReadBuf = (CGenReadBuf* )pCMD->m_pGenBuf;
	if(pReadBuf == NULL){
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	if(pReadBuf->m_byReadWay != READ_ITEMALLDATA) return;
	if(!pReadBuf->m_dwRWInf || !pReadBuf->m_wGroupSum || pReadBuf->m_pGroupValue == NULL) {
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
		return;
	}

	pReadBuf->m_dwGrcSendDataLen = 0;
	pReadBuf->m_wGrcSendDataNOG = 0;
	pReadBuf->m_dwGrcSendDataIp = 0;
	pReadBuf->m_byOldRII = m_byRII++;
	uchar byBuf[10];
	uchar* pGenReq = (uchar* )byBuf;
	pGenReq[GENREQCOM_FUN] = 254;
	pGenReq[GENREQCOM_INF] = 243;
	pGenReq[GENREQCOM_RII] = pReadBuf->m_byOldRII;
	pGenReq[GENREQCOM_NOG] = 1;
	uchar* GIR = &pGenReq[GENREQCOM_GIR];
	GIR[GENID_KOD] = 0;
	GIR[GENID_GIN] = (pReadBuf->m_pGroupValue + pReadBuf->m_wGroupCurNo)->byGROUP;
	GIR[GENID_GIN+1] = (pReadBuf->m_pGroupValue + pReadBuf->m_wGroupCurNo)->byENTRY;
	uchar byLen = 7;
	pReadBuf->m_wGroupCurNo++;
	if(pReadBuf->m_wGroupCurNo >= pReadBuf->m_wGroupSum) pReadBuf->m_dwRWInf = 0;
	SendAsdu21(keyCMD.wFacNo, keyCMD.wLogicalNodeId, byLen, byBuf);
}

//读一个组所有条目的某个属性
void CProtocol::SGenReadAllItemData(CGenCommand* pCMD)
{
	SCommandID keyCMD = pCMD->m_keyCMD;
	if(!pCMD->m_bIsReadType){
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	CGenReadBuf* pReadBuf = (CGenReadBuf* )pCMD->m_pGenBuf;
	if(!pReadBuf->m_dwRWInf || pReadBuf->m_byReadWay != READ_ALLITEMDATA) {
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	if(!pReadBuf->m_wGroupSum || pReadBuf->m_pGroupValue == NULL){
		uchar	byGroupValue[MAXGROUPITEM];
		pReadBuf->m_wGroupSum = GetGroupByGroupType(keyCMD, byGroupValue, MAXGROUPITEM);
		if(pReadBuf->m_wGroupSum <= 0) {
			NotifyGenCMD(pCMD, true, false);
			DeleteGenCMD(keyCMD);
			return;
		}
		if(pReadBuf->m_pGroupValue) delete[] pReadBuf->m_pGroupValue;
		pReadBuf->m_pGroupValue = new SGINType[pReadBuf->m_wGroupSum];
		for(int i=0; i<pReadBuf->m_wGroupSum; i++){
			(pReadBuf->m_pGroupValue + i)->byGROUP = byGroupValue[i];
			(pReadBuf->m_pGroupValue + i)->byENTRY = 0;
		}
		pReadBuf->m_wGroupCurNo = 0;
	}
	uchar byKOD;
	if(pReadBuf->m_wGroupCurNo == pReadBuf->m_wGroupSum-1) 
		byKOD = GetReadKOD(pReadBuf->m_dwRWInf, true);
	else byKOD = GetReadKOD(pReadBuf->m_dwRWInf, false);
	if(byKOD == (uchar )RETERROR){
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	pReadBuf->m_dwGrcSendDataLen = 0;
	pReadBuf->m_wGrcSendDataNOG = 0;
	pReadBuf->m_dwGrcSendDataIp = 0;
	pReadBuf->m_byOldRII = m_byRII++;
	uchar byBuf[10];
	uchar* pGenReq = (uchar* )byBuf;
	pGenReq[GENREQCOM_FUN] = 254;
	pGenReq[GENREQCOM_INF] = 241;
	pGenReq[GENREQCOM_RII] = pReadBuf->m_byOldRII;
	pGenReq[GENREQCOM_NOG] = 1;
	uchar* GIR = &pGenReq[GENREQCOM_GIR];
	GIR[GENID_KOD] = byKOD;
	GIR[GENID_GIN] = (pReadBuf->m_pGroupValue + pReadBuf->m_wGroupCurNo)->byGROUP;
	GIR[GENID_GIN+1] = 0;
	uchar byLen = 7;
	pReadBuf->m_wGroupCurNo++;
	if(pReadBuf->m_wGroupCurNo >= pReadBuf->m_wGroupSum) pReadBuf->m_wGroupCurNo = 0;
	SendAsdu21(keyCMD.wFacNo, keyCMD.wLogicalNodeId, byLen, byBuf);
}

void CProtocol::SGenReadItemData(CGenCommand* pCMD)
{
	SCommandID keyCMD = pCMD->m_keyCMD;
	if(!pCMD->m_bIsReadType || pCMD->m_pGenBuf == NULL) {
		NotifyGenCMD(pCMD, false, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	CGenReadBuf* pReadBuf = (CGenReadBuf* )pCMD->m_pGenBuf;
	if(pReadBuf->m_byReadWay != READ_ITEMDATA) {
		NotifyGenCMD(pCMD, false, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	uchar SendTemp[300];
	uchar byCmdBufLen;
	uchar* pDataCom = (uchar* )SendTemp;
	uchar* pBufHead = (uchar* )(pReadBuf->m_pGrcSendDataBuf + pReadBuf->m_dwGrcSendDataIp);
	ushort ReservedNOG = pReadBuf->m_wGrcSendDataNOG;
	if(ReservedNOG == 0) return;
	if(ReservedNOG>75){
		pReadBuf->m_dwGrcSendDataIp += 75*3;
		pReadBuf->m_wGrcSendDataNOG -= 75;
		byCmdBufLen = 75*3;
        memcpy(pDataCom, pReadBuf->m_pGrcSendDataBuf, 4);
        memcpy(&SendTemp[4], pBufHead, byCmdBufLen);
		pDataCom[GENREQCOM_NOG] = 75;
	}
	else{
		pReadBuf->m_dwGrcSendDataIp += ReservedNOG*3;
		pReadBuf->m_wGrcSendDataNOG = 0;
		byCmdBufLen = ReservedNOG * 3;
        memcpy(pDataCom, pReadBuf->m_pGrcSendDataBuf, 4);
        memcpy(&SendTemp[4], pBufHead, byCmdBufLen);
		pDataCom[GENREQCOM_NOG] = (uchar) ReservedNOG;
	}
	uchar byLen = byCmdBufLen+8;
	SendAsdu21(keyCMD.wFacNo, keyCMD.wLogicalNodeId, byLen, SendTemp);
}

void CProtocol::SGenReadCatalog(ushort wFacNo, ushort wLogicalNodeID)
{
	uchar byBuf[10];
	uchar* pDataRec = (uchar* )byBuf;
	pDataRec[GENREQCOM_FUN]= 254;
	pDataRec[GENREQCOM_INF]= 240;
	pDataRec[GENREQCOM_RII]= m_byRII++;
	pDataRec[GENREQCOM_NOG]= 0;
	SendAsdu21(wFacNo, wLogicalNodeID, 4, byBuf);
}

void CProtocol::SendAsdu10(ushort wFacNo, ushort wLogicalNodeID, uchar byLen, uchar* pData)
{
	char pAsdu[300];
	pAsdu[ASDU_TYPE] = (char) TYPC_GRC_DATA;
	pAsdu[ASDU_VSQ] = (char) VSQ_COMMON1;
	pAsdu[ASDU_COT] = (char) COTC_GRC_WRITE;
	pAsdu[ASDU_ADDR] = (uchar) (wLogicalNodeID % 256);
	if(byLen > 250) return;
    memcpy(&pAsdu[4], pData, byLen);
	AppCommu::Instance()->ProtocolNetSend(wFacNo, wLogicalNodeID, pAsdu, byLen+4);
}

void CProtocol::SendAsdu21(ushort wFacNo, ushort wLogicalNodeID, uchar byLen, uchar* pData)
{
	char pAsdu[300];
	pAsdu[ASDU_TYPE] = (char) TYPC_GRC_COMMAND;
	pAsdu[ASDU_VSQ] = (char) VSQ_COMMON1;
	pAsdu[ASDU_COT] = (char) COTC_GRC_READ;
	pAsdu[ASDU_ADDR] = (uchar ) (wLogicalNodeID % 256);
	if(byLen > 250) return;
    memcpy(&pAsdu[4], pData, byLen);
	AppCommu::Instance()->ProtocolNetSend(wFacNo, wLogicalNodeID, pAsdu, byLen+4);
}

void CProtocol::SendDisturbCmd(CDisturbItem* pDisturbItem, uchar byTOO, uchar byACC, uchar byTOV)
{
	if(pDisturbItem->m_pDtbTable == NULL) return;
	SDevID keyDTB = pDisturbItem->m_keyDTB;
	uchar asdu[11];
	asdu[ASDU_TYPE] = TYPC_DTB_COMMAND;
	asdu[ASDU_VSQ] = VSQ_COMMON1;
	asdu[ASDU_COT] = COT_DTB_DATA;
	asdu[ASDU_ADDR] = (uchar) (keyDTB.wLogicalNodeId & 0x00ff);
	asdu[ASDU_FUN] = pDisturbItem->m_pDtbTable->m_byFUN;
	asdu[ASDU_INF] = 0;
	asdu[ASDU24_TOO] = byTOO;
	asdu[ASDU24_TOV] = byTOV;
	asdu[ASDU24_FAN] = pDisturbItem->m_pDtbTable->m_wFAN % 256;
	asdu[ASDU24_FAN+1] = pDisturbItem->m_pDtbTable->m_wFAN / 256;
	asdu[ASDU24_ACC] = byACC;
	AppCommu::Instance()->ProtocolNetSend(keyDTB.wFacNo, keyDTB.wLogicalNodeId, 
							(char* )asdu, 11);
}

void CProtocol::SendDisturbOver(CDisturbItem* pDisturbItem, uchar byTOO, uchar byACC, uchar byTOV)
{
	if(pDisturbItem->m_pDtbTable == NULL) return;
	SDevID keyDTB = pDisturbItem->m_keyDTB;
	uchar asdu[11];
	asdu[ASDU_TYPE] = TYPC_DTB_CONFIRM;
	asdu[ASDU_VSQ] = VSQ_COMMON1;
	asdu[ASDU_COT] = COT_DTB_DATA;
	asdu[ASDU_ADDR] = (uchar) (keyDTB.wLogicalNodeId & 0x00ff);
	asdu[ASDU_FUN] = pDisturbItem->m_pDtbTable->m_byFUN;
	asdu[ASDU_INF] = 0;
	asdu[ASDU24_TOO] = byTOO;
	asdu[ASDU24_FAN] = pDisturbItem->m_pDtbTable->m_wFAN % 256;
	asdu[ASDU24_FAN+1] = pDisturbItem->m_pDtbTable->m_wFAN / 256;
	asdu[ASDU24_ACC] = byACC;
	asdu[ASDU24_TOV] = byTOV;
	AppCommu::Instance()->ProtocolNetSend(keyDTB.wFacNo, keyDTB.wLogicalNodeId, (char* )asdu, 11);
}

void CProtocol::SendGeneralQuary(ushort wFacNo, ushort wLogicalNodeID)
{
	char pAsdu[8];
	pAsdu[ASDU_TYPE] = (uchar )TYPC_GRC_COMMAND;
	pAsdu[ASDU_VSQ] = (uchar )VSQ_COMMON1;
	pAsdu[ASDU_COT] = (uchar )COTC_GENQUERY;
	pAsdu[ASDU_ADDR] = (uchar ) (wLogicalNodeID & 0x00ff);

	uchar* pDataRec = (uchar* )&pAsdu[4];
	pDataRec[GENREQCOM_FUN] = 254;
	pDataRec[GENREQCOM_INF] = 245;
	pDataRec[GENREQCOM_RII] = m_byRII++;
	pDataRec[GENREQCOM_NOG] = 1;
	
	AppCommu::Instance()->ProtocolNetSend(wFacNo, wLogicalNodeID, pAsdu, 8);
}

void CProtocol::IYKCheck(ushort wFacID, ushort wDevID, SYKCheckDataID* pYKCheckID)
{
	if(!pYKCheckID) return;
	uchar* pSndBuf = new uchar[200];
	pSndBuf[0] = 228;
	pSndBuf[1] = 0x81;
	pSndBuf[2] = 0x14;
	pSndBuf[3] = 0xff;
	pSndBuf[4] = 0xfe;
	pSndBuf[5] = 0xf4;
	pSndBuf[6] = 0;
	pSndBuf[7] = 1;
	pSndBuf[8] = pYKCheckID->wFacAddr % 256;
	pSndBuf[9] = pYKCheckID->wFacAddr / 256;
	pSndBuf[10] = pYKCheckID->wDevAddr % 256;
	pSndBuf[11] = pYKCheckID->wDevAddr / 256;
	pSndBuf[12] = pYKCheckID->wGIN % 256;
	pSndBuf[13] = pYKCheckID->wGIN / 256;
	pSndBuf[14] = pYKCheckID->byOPType;
	AppCommu::Instance()->ProtocolNetSend(wFacID, wDevID, (char* )pSndBuf, 15);
	delete[] pSndBuf;
}
//yangsh need
void CProtocol::ISCYKRunCMD(ushort wStaAddr, ushort wDevAddr, ushort wGIN, bool bRet)
{
	SDevID	DevID;
	DevID.wFacNo = wStaAddr;
	DevID.wLogicalNodeId = wDevAddr;
	CYKYTItem* pYKItem = FindYKYTItem(DevID);
	if(!pYKItem || pYKItem->m_YKData.wGIN != wGIN || !pYKItem->m_bSCYK) return;
	if(pYKItem->m_YKData.byExecType != enumExecTypeExecute) return;
	if(bRet) SendYKYTCommand(pYKItem);	
	else {
		pYKItem->m_YKData.byExecType = enumExecTypeCancel;
		SendYKYTCommand(pYKItem);
	}
}

//yangsh need
void CProtocol::IDoSftSwitch(ushort wStationAddr, ushort wDevAddr, ushort wGIN, bool bVal, bool bIsOtherYK)
{
	if(bIsOtherYK) {
		SYKYTData YKData;
		YKData.wGIN = wGIN;
		YKData.byOpType = YK_TYPE_NORMAL;
		YKData.byExecType = enumExecTypeSelect;
		if(bVal) YKData.byCommandType = enumYKTypeOnUp;
		else YKData.byCommandType = enumYKTypeOffDown;
		IYKYTSend(wStationAddr, wDevAddr, &YKData, false, true);
	}
	else {
		SGENDataItem DataItem;
		DataItem.wGINOrder = wGIN;
		DataItem.m_byDataNumber = 1;
		DataItem.m_byDataSize = 1;
		DataItem.m_byDataType = 3;
		uchar byVal = 0;
		if(bIsOtherYK) byVal = 1;
		DataItem.m_Value.resize(1);
		DataItem.m_Value[0] = byVal;
		IGenWriteItem(wStationAddr, wDevAddr, enumGroupTypeSoftSwitch,  1, &DataItem);
	}
}

/****************************************************************/
/*					接收部分									*/
/****************************************************************/
void CProtocol::IAskGeneralQuary()
{
    foreach (CDevice* pDevice, m_plDeviceDstb) {
        if(pDevice->m_byDevType >= NODETYPE_RELAYDEV) {
            pDevice->m_bIsNeedGeneralQuary = true;
        }
    }
}

void CProtocol::IDeviceAddOrDel(ushort wFacID, ushort wDevID, uchar byType, uchar byAddOrDel)
{
	if(byAddOrDel) {
		CDevice* pDevice = GetDeviceDstb(wFacID, wDevID, byType);
		if(pDevice != NULL && pDevice->m_byDevType != NODETYPE_RECORDDEV && pDevice->m_byDevType >= NODETYPE_RELAYDEV) {
            IGenReadItem(wFacID, wDevID, enumGroupTypeCatalog, enumDevReadAll, -1);
		}
	}
	else {
//		INotifyNetDState(wFacID, wDevID, 0, 0);
		DeleteDeviceDstb(wFacID, wDevID);
	}
	SDeviceAddOrDel DeviceAddOrDel;
	DeviceAddOrDel.wFacAddr = wFacID;
	DeviceAddOrDel.wDevAddr = wDevID;
	DeviceAddOrDel.byDevType = byType;
	if(byAddOrDel) DeviceAddOrDel.bAdd = true;
	else DeviceAddOrDel.bAdd = false;
	AppCommu::Instance()->RDeviceAddOrDel(&DeviceAddOrDel);
}

void CProtocol::SendNetState(ushort wFacID, ushort wDevID, bool bIsNet2, uchar byNetState)
{
	ushort wDataSize = 7;
	CGrcDataRec* pDataRec = new CGrcDataRec(wFacID, wDevID, wDataSize, 2);
	pDataRec->m_pGrcRec->pGenDataRec = new uchar[7];
	uchar* pData = (uchar* )pDataRec->m_pGrcRec->pGenDataRec;
	if(bIsNet2) pData[GENDATAREC_GIN+1] = 2;
	else pData[GENDATAREC_GIN+1] = 1;
	pData[GENDATAREC_GIN] = 0xff;
	pData[GENDATAREC_KOD] = (uchar )KOD_ACTUALVALUE;
	pData[GENDATAREC_GDD + GDDTYPE_NUM] = 1;
	pData[GENDATAREC_GDD + GDDTYPE_SIZ] = 1; 
	pData[GENDATAREC_GDD] = 3;
	if(byNetState) pData[GENDATAREC_GID] = 1;
	else pData[GENDATAREC_GID] = 0;
	GenDataEnterDB(pDataRec, enumGroupTypeNetState);
	
	delete pDataRec;
}

void CProtocol::INotifyNetDState(ushort wFacID, ushort wDevID, uchar byNetType, uchar byNetState)
{
	CDevice* pDevice = FindDeviceDstb(wFacID, wDevID);
	if(!pDevice) return ;
	if(byNetType == 0 && pDevice->m_byNet1State != byNetState){
		pDevice->m_byNet1State = byNetState;
		SendNetState(wFacID, wDevID, false, byNetState);
	}
	else if(byNetType == 1 && pDevice->m_byNet2State != byNetState){
		pDevice->m_byNet2State = byNetState;
		SendNetState(wFacID, wDevID, true, byNetState);
	} 
}

void CProtocol::IDataUnpack(ushort wFacNo, ushort wDevID, uchar* pData,ushort wLength)
{
	if(wLength <= 1) return;
	g_mutexProtocolDataUnpack.lock();
	RawDataHandle();						//先处理上次未处理完的
	DataAnalysis(wFacNo, wDevID, pData, wLength);
	g_mutexProtocolDataUnpack.unlock();
//	else {
//		CRawData* pRawData = new CRawData(wFacNo, wDevID, wLength);
//		memcpy(pRawData->m_pDataBuf, pData, wLength);
//		AddRawDataToWaitList(pRawData);
//	}
}

void CProtocol::DataAnalysis(ushort wFacNo, ushort wDevID, uchar* pData, ushort wLength)
{
	switch(*pData){
	case 5:
		if(wLength < 19) return;
		HandleASDU5(pData, wLength, wFacNo, wDevID);
		break;
	case 6:
		if(wLength < 13) return;
		HandleASDU6(pData, wLength, wFacNo, wDevID);
		break;
	case 10:
		HandleASDU10(pData, wLength, wFacNo, wDevID);
		break;
	case 11:
		HandleASDU11(pData, wLength, wFacNo, wDevID);
		break;
	case 23:
		HandleASDU23(pData, wLength, wFacNo, wDevID);
		break;
	case 26:
		if(wLength<21) return;
		HandleASDU26(pData, wLength, wFacNo, wDevID);
		break;
	case 28:
		if(wLength<10) return;
		HandleASDU28(pData, wLength, wFacNo, wDevID);
		break;
	case 29:
		HandleASDU29(pData, wLength, wFacNo, wDevID);
		break;
	case 31:
		if(wLength<11) return;
		HandleASDU31(pData, wLength, wFacNo, wDevID);
		break;
	case 27:
		if(wLength<23) return;
		HandleASDU27(pData, wLength, wFacNo, wDevID);
		break;
	case 30:
		HandleASDU30(pData, wLength, wFacNo, wDevID);
		break;
	case 222:
		if(AppCommu::Instance()->m_byNodeType != NODETYPE_DISPATCH && AppCommu::Instance()->m_bIsHostNode) 
			m_pFileTrans->HandleASDU222(pData, wLength, wFacNo, wDevID);
		break;
	case 223:
		if(AppCommu::Instance()->m_byNodeType != NODETYPE_DISPATCH && AppCommu::Instance()->m_bIsHostNode) 
			m_pFileTrans->HandleASDU223(pData, wLength, wFacNo, wDevID);
		break;
	case 224:
		if(AppCommu::Instance()->m_byNodeType != NODETYPE_DISPATCH && AppCommu::Instance()->m_bIsHostNode) 
			m_pFileTrans->HandleASDU224(pData, wLength, wFacNo, wDevID);
		break;
	case 225:
		if(AppCommu::Instance()->m_byNodeType != NODETYPE_DISPATCH && AppCommu::Instance()->m_bIsHostNode) 
			m_pFileTrans->HandleASDU225(pData, wLength, wFacNo, wDevID);
		break;
	case 226:
		if(AppCommu::Instance()->m_byNodeType != NODETYPE_DISPATCH && AppCommu::Instance()->m_bIsHostNode) 
			m_pFileTrans->HandleASDU226(pData, wLength, wFacNo, wDevID);
		break;
	case 229:
		if(AppCommu::Instance()->m_byNodeType != NODETYPE_DISPATCH && AppCommu::Instance()->m_bIsHostNode) 
			HandleASDU229(pData, wLength, wFacNo, wDevID);
		break;
	default:
		break;
	}
}

//通用分类10处理
void CProtocol::HandleASDU10(uchar* pData , ushort wLength , ushort wFacNo, ushort wLogicalNodeID)
{
	if(wLength <= 4) return;
	ushort wLen = wLength-4;
	uchar* pDataCom = (uchar* )(pData+4);
	ushort wCalLen = CalcuGrcDataLen(pDataCom, wLen);
	if(wCalLen < 4) return;
	SDevID keyData;
	keyData.wFacNo = wFacNo;
	keyData.wLogicalNodeId = wLogicalNodeID;
	uchar byCot = pData[ASDU_COT];
	uchar byInf = pData[ASDU_INF];
	uchar byRII = pData[ASDU21_RII];
	uchar byNum = pDataCom[GENDATACOM_NGD] & 0x3f;
	if(byCot == COT_SPONTANEOUS || byCot == COT_CYCLIC || byCot == COT_GI || byCot == COT_TELECONTROL){
		GenDataEntry(wFacNo, wLogicalNodeID, byNum, (uchar* )(pData+8), wCalLen-4, byCot);
	}
	else if(byCot == COT_HIS || byCot == COT_HISOVER){
		CHisCommand* pHisCMD = FindHisCMD(wFacNo, wLogicalNodeID, byRII);
		if(!pHisCMD) return;
		pHisCMD->m_dwCount = 0;
		GenDataEntry(wFacNo, wLogicalNodeID, byNum, (uchar* )(pData+8), wCalLen-4, byCot);
		if(byCot == COT_HISOVER) {
			NotifyHisCMD(pHisCMD, true);
			DeleteHisCMD(pHisCMD->m_keyCMD);
		}
	}
	else if((byCot == 41 || byCot==44) && byInf==249){		//带确认的写条目
		if(byNum == 0 || wCalLen <= 4) return;
		uchar byGroup = pDataCom[GENDATACOM_GDR + GENDATAREC_GIN ];
		uchar byType = GetGroupTypeByGroupVal(wFacNo, wLogicalNodeID, byGroup);
		if(byType == (uchar )RETERROR) return;
		else if(byType == enumGroupTypeYK || byType == enumGroupTypeYT || byType == enumGroupTypeRelayJDST) {
			RYKYTHandle(wFacNo, wLogicalNodeID, pDataCom, wCalLen, byCot);
		}
		else if(byType == enumGroupTypeSoftSwitch && byNum == 1 && FindYKYTItem(keyData)) {
			RYKYTHandle(wFacNo, wLogicalNodeID, pDataCom, wCalLen, byCot);
		}
		else {
			SCommandID keyCMD;
			keyCMD.wFacNo = wFacNo;
			keyCMD.wLogicalNodeId = wLogicalNodeID;
			CGenCommand* pCMD = FindGenCMD(keyCMD, false);
			if(pCMD) {
				if(byCot == 41) {
					keyCMD = pCMD->m_keyCMD;
					NotifyGenCMD(pCMD, false, false);
					DeleteGenCMD(keyCMD);
					return;
				}
				else RGenWriteAffirm(pCMD, pDataCom);
			}
		}
	}
	else if((byCot==40 || byCot == 41 ) && byInf==250){ //带执行的写条目
		uchar	byType = (uchar )RETERROR;
		if(byNum){
			uchar byGroup = pDataCom[GENDATACOM_GDR + GENDATAREC_GIN ];
			byType = GetGroupTypeByGroupVal(wFacNo, wLogicalNodeID, byGroup);
			if(byType == (uchar) RETERROR) return;
		}
		if(byType == enumGroupTypeYK || byType == enumGroupTypeYT || byType == enumGroupTypeRelayJDST) {
			RYKYTHandle(wFacNo, wLogicalNodeID, pDataCom, wCalLen, byCot);
		}
		else if(byType == enumGroupTypeSoftSwitch && byNum == 1 && FindYKYTItem(keyData) ) {
			RYKYTHandle(wFacNo, wLogicalNodeID, pDataCom, wCalLen, byCot);
		}
		else {
			SCommandID keyCMD;
			keyCMD.wFacNo = wFacNo;
			keyCMD.wLogicalNodeId = wLogicalNodeID;
			CGenCommand* pCMD = FindGenCMD(keyCMD, false);
			if(pCMD){
				if(byCot == 41)	NotifyGenCMD(pCMD, false, false);
				else NotifyGenCMD(pCMD, false, true);		//成功
				keyCMD = pCMD->m_keyCMD;
				DeleteGenCMD(keyCMD);
				return;
			}
		}
	}
	else if((byCot==40 || byCot == 41 ) && byInf==251){ //带取消的写条目
		uchar	byType = (uchar )RETERROR;
		if(byNum){
			uchar byGroup = pDataCom[GENDATACOM_GDR + GENDATAREC_GIN];
			byType = GetGroupTypeByGroupVal(wFacNo, wLogicalNodeID, byGroup);
			if(byType == (uchar) RETERROR) return;
		}
		if(byType == enumGroupTypeYK || byType == enumGroupTypeYT || byType == enumGroupTypeRelayJDST) {
			RYKYTHandle(wFacNo, wLogicalNodeID, pDataCom, wCalLen, byCot);
		}
		else if(byType == enumGroupTypeSoftSwitch && byNum == 1 && FindYKYTItem(keyData)) {
			RYKYTHandle(wFacNo, wLogicalNodeID, pDataCom, wCalLen, byCot);
		}
		else return;
	}
	else if(byCot==42 && byInf==240){	//读目录响应
		RGenReadCatalog(wFacNo, wLogicalNodeID, pDataCom, wCalLen);
	}
	else if( (byCot==42 || byCot==43) && (byInf==241 || byInf==244) ) {//读一个组的全部条目
		if(byNum == 0){							//ADD FOR DD Start
			CGenCommand* pCMD = FindGenCMD(wFacNo, wLogicalNodeID, byRII, true);
			if(pCMD) RGenRead10Pack(pCMD, pDataCom, byCot, wCalLen);
			return;
		}														//ADD FOR DD End				
		uchar byGroup = pDataCom[GENDATACOM_GDR + GENDATAREC_GIN];
		uchar byCMDType = GetGroupTypeByGroupVal(wFacNo, wLogicalNodeID, byGroup);
		if(byCMDType == (uchar )RETERROR) return;
		SCommandID keyCMD;
		keyCMD.wFacNo = wFacNo;
		keyCMD.wLogicalNodeId = wLogicalNodeID;
		keyCMD.wGroupType = (EPreDefinedGroupType) byCMDType;
		CGenCommand* pCMD = FindGenCMD(keyCMD, true);
		if(pCMD) RGenRead10Pack(pCMD, pDataCom, byCot, wCalLen);
	}
}

//通用分类11处理
void CProtocol::HandleASDU11(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
	if(wLength <= 4) return;
	ushort wLen = wLength-4;
	uchar* pGenID = (uchar* )(pData+4);
	ushort wCalLen = CalcuGrcIDLen(pGenID, wLen);
	if(wCalLen > 240 || wCalLen == 4) return;
	uchar byCot = pData[ASDU_COT];
	if((byCot==42||byCot==43) && pGenID[GENDESID_INF] == 243){
		SCommandID keyCMD;
		keyCMD.wFacNo = wFacNo;
		keyCMD.wLogicalNodeId = wLogicalNodeID;
		uchar byGroup = pGenID[GENDESID_GIN];
		uchar byGroupType =  GetGroupTypeByGroupVal(wFacNo, wLogicalNodeID, byGroup);
		if(byGroupType == (uchar )RETERROR) return;
		keyCMD.wGroupType = (EPreDefinedGroupType) byGroupType;
		CGenCommand* pCMD = FindGenCMD(keyCMD, true);
		if(pCMD) RGenRead11Pack(pCMD, pGenID, byCot, wCalLen);
	}
}

void CProtocol::RGenWriteAffirm(CGenCommand* pCMD, uchar* pDataCom)
{
	SCommandID keyCMD = pCMD->m_keyCMD;
	if(pCMD->m_bIsReadType){
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	pCMD->ResetTimer();
	CGenWriteBuf* pWriteBuf = (CGenWriteBuf* )pCMD->m_pGenBuf;
	if(pWriteBuf == NULL){
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	if(!(CompareData(pWriteBuf->m_pGrcWriteBuf, pDataCom,
									pWriteBuf->m_dwGrcWriteBufLen))) {
		NotifyGenCMD(pCMD, false, false);
		uchar byBuf[4];
		uchar* pCancelCmd = (uchar* )byBuf;
		pCancelCmd[GENDATACOM_FUN] = pDataCom[GENDATACOM_FUN];
		pCancelCmd[GENDATACOM_INF] = 251;
		pCancelCmd[GENDATACOM_RII] = pWriteBuf->m_byOldRII;
		pCancelCmd[GENDATACOM_NGD] = 0;
		if (pWriteBuf->m_byGrcSendDataLastNGDbCount) pCancelCmd[GENDATACOM_NGD] |= 0x40;
		else pCancelCmd[GENDATACOM_NGD] &= 0xbf;
		SendAsdu10(keyCMD.wFacNo, keyCMD.wLogicalNodeId, 4, byBuf);
		DeleteGenCMD(keyCMD);
		return;
	}
	if(pWriteBuf->m_dwGrcSendDataIp < pWriteBuf->m_dwGrcSendDataLen
		&& pWriteBuf->m_wGrcSendDataNOG > 0)
		SGenWriteCMD(pCMD);
	else {
		uchar byBuf[4];
		uchar* pExecCmd = (uchar* )byBuf;
		pExecCmd[GENDATACOM_FUN] = pDataCom[GENDATACOM_FUN];
		pExecCmd[GENDATACOM_INF] = 250;
		pExecCmd[GENDATACOM_RII] = pWriteBuf->m_byOldRII;
		pExecCmd[GENDATACOM_NGD] = 0;
		if(pWriteBuf->m_byGrcSendDataLastNGDbCount) pExecCmd[GENDATACOM_NGD] |= 0x40;
		else pExecCmd[GENDATACOM_NGD] &= 0xbf;
		SendAsdu10(keyCMD.wFacNo, keyCMD.wLogicalNodeId, 4, byBuf);
	}
}

//读组的返回处理
void CProtocol::RGenReadCatalog(ushort wFacNo, ushort wLogicalNodeID, uchar* pDataCom, ushort wCalLen)
{
    qDebug()<<"读目录返回";
	SCommandID keyCMD;
	keyCMD.wFacNo = wFacNo;
	keyCMD.wLogicalNodeId = wLogicalNodeID;
	keyCMD.wGroupType = enumGroupTypeCatalog;
	CGenCommand* pCMD = FindGenCMD(keyCMD, true);
	if(pCMD == NULL) return;
	if(!pCMD->m_bIsReadType){
		DeleteGenCMD(keyCMD);
		return;
	}
	pCMD->ResetTimer();
	CGenReadBuf* pReadBuf;
	if(pCMD->m_pGenBuf == NULL){
		pReadBuf = new CGenReadBuf;
		pCMD->m_pGenBuf = pReadBuf;
	}
	else pReadBuf = (CGenReadBuf* )pCMD->m_pGenBuf;
	CGrcReadPack* pReadPack = new CGrcReadPack;
	pReadPack->m_wReadPackLen = wCalLen;
    memcpy(pReadPack->m_byReadPack, pDataCom, wCalLen);
	pReadBuf->m_plGrcReadPackList.append(pReadPack);
	uchar byNGD = pDataCom[GENDATACOM_NGD];
	if(!(byNGD & 0x80)){
		ushort wTotalLen = 0;
        foreach(CGrcReadPack* pPack,pReadBuf->m_plGrcReadPackList) {
			wTotalLen += pPack->m_wReadPackLen;
        }
		if(wTotalLen < 4){
			DeleteGenCMD(keyCMD);
			return;
		}
        //qDebug()<<"总长度:"<<wTotalLen;
		uchar* pTotalBuf = new uchar[wTotalLen];
		ushort wSum = 0;
		uchar* pTotalHead = pTotalBuf;
        foreach(CGrcReadPack* pPack,pReadBuf->m_plGrcReadPackList) {
			uchar* pPackgd = (uchar* )pPack->m_byReadPack;
			if(pPackgd != NULL){
				ushort wLen = CalcuGrcDataLen(pPackgd, 250);
				if(wLen-4 > 0) {
                    memcpy(pTotalHead, ((uchar* )pPackgd)+4, wLen-4);//若是空的，不拷贝
					pTotalHead += (wLen-4);
					wSum += (pPackgd[GENDATACOM_NGD] & 0x3f);
				}
			}
        }
		GenCatalogEnterDB(wFacNo, wLogicalNodeID, wSum, pTotalBuf, wTotalLen);	
		delete[] pTotalBuf;
		DeleteGenCMD(keyCMD);


        for(int pos = 0;pos<m_plGenWaitForHandle.count();pos++) {
            CGrcDataRec* pDataRec = m_plGenWaitForHandle.at(pos);
			if(pDataRec->m_pGrcRec->wFacNo == wFacNo && pDataRec->m_pGrcRec->wDevID == wLogicalNodeID){
				GenDataHandle(pDataRec, true);
                m_plGenWaitForHandle.removeAt(pos);
				delete pDataRec;
                pos--;
			}
		}
	}
}

void CProtocol::RGenRead10Pack(CGenCommand* pCMD, uchar* pDataCom, uchar byCot, ushort wDataLen)
{
	if(pCMD == NULL) return;
	SCommandID keyCMD = pCMD->m_keyCMD;
	if(!pCMD->m_bIsReadType || pCMD->m_pGenBuf == NULL){
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	pCMD->ResetTimer();
	CGenReadBuf* pReadBuf = (CGenReadBuf* )pCMD->m_pGenBuf;
	if(pDataCom[GENDATACOM_RII] != pReadBuf->m_byOldRII){
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	if(byCot == COT_GRCR_VALID){
		CGrcReadPack* pReadPack = new CGrcReadPack;
		pReadPack->m_byReadCOT = byCot;
		pReadPack->m_wReadPackLen = wDataLen;
        memcpy(pReadPack->m_byReadPack, pDataCom, wDataLen);
		pReadBuf->m_plGrcReadPackList.append(pReadPack);
	}
	uchar byNGD = pDataCom[GENDATACOM_NGD];
	if(!(byNGD & 0x80)){
		if(pDataCom[GENDATACOM_INF] == 244){
			if(pReadBuf->m_byReadWay == READ_ITEMDATA){
				if(pReadBuf->m_dwGrcSendDataIp < pReadBuf->m_dwGrcSendDataLen 
					&& pReadBuf->m_wGrcSendDataNOG > 0)
					 SGenReadItemData(pCMD);
				else {
					RGenRead10Handle(keyCMD.wFacNo, keyCMD.wLogicalNodeId, &pReadBuf->m_plGrcReadPackList);
					pReadBuf->ClearPackList();
					NotifyGenCMD(pCMD, true, true);//成功
					DeleteGenCMD(keyCMD);
				}
			}
			else {
				NotifyGenCMD(pCMD, true, false);
				DeleteGenCMD(keyCMD);
				return;
			}
		}
		else if(pDataCom[GENDATACOM_INF] == 241){
			if(pReadBuf->m_byReadWay == READ_ALLITEMDATA){
				RGenRead10Handle(keyCMD.wFacNo, keyCMD.wLogicalNodeId, &pReadBuf->m_plGrcReadPackList);
				pReadBuf->ClearPackList();
				if(pReadBuf->m_dwRWInf) SGenReadAllItemData(pCMD);
				else {
					NotifyGenCMD(pCMD, true, true);//成功
					DeleteGenCMD(keyCMD);
				}
			}
			else {
				NotifyGenCMD(pCMD, true, false);
				DeleteGenCMD(keyCMD);
				return;
			}
		}
	}
	return;
}

void CProtocol::RGenRead11Pack(CGenCommand* pCMD, uchar* pDataCom, uchar byCot, ushort wDataLen)
{
	SCommandID keyCMD = pCMD->m_keyCMD;
	if(!pCMD->m_bIsReadType || pCMD->m_pGenBuf == NULL) {
		NotifyGenCMD(pCMD, false, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	pCMD->ResetTimer();
	CGenReadBuf* pReadBuf = (CGenReadBuf* )pCMD->m_pGenBuf;
	if(pDataCom[GENDESID_RII] != pReadBuf->m_byOldRII || pReadBuf->m_byReadWay != READ_ITEMALLDATA){
		NotifyGenCMD(pCMD, true, false);
		DeleteGenCMD(keyCMD);
		return;
	}
	if(byCot == COT_GRCR_VALID){
		CGrcReadPack* pReadPack = new CGrcReadPack;
		pReadPack->m_byReadCOT = byCot;
		pReadPack->m_wReadPackLen = wDataLen;
        memcpy(pReadPack->m_byReadPack, pDataCom, wDataLen);
		pReadBuf->m_plGrcReadPackList.append(pReadPack);
	}
	if(!(pDataCom[GENDESID_NGD] & 0x80)){
		RGenRead11Handle(keyCMD.wFacNo, keyCMD.wLogicalNodeId, &pReadBuf->m_plGrcReadPackList);
		pReadBuf->ClearPackList();
		if(pReadBuf->m_dwRWInf) SGenReadItemAllData(pCMD);
		else {
			NotifyGenCMD(pCMD, true, true);//成功
			DeleteGenCMD(keyCMD);
		}
	}
	return;
}

void CProtocol::RGenRead10Handle(ushort wFacNo, ushort wLogicalNodeID, QList<CGrcReadPack*>* pReadPackList)
{
	ushort wTotalLen = 0;
    foreach(CGrcReadPack* pPack,*pReadPackList) {
		wTotalLen += pPack->m_wReadPackLen;
	}
	uchar* pTotalBuf = new uchar[wTotalLen];
	ushort wSum = 0;
	uchar* pTotalHead = pTotalBuf;
	ushort wCalLen = 0;
    foreach(CGrcReadPack* pPack,*pReadPackList) {
		uchar* pPackgd = (uchar* )pPack->m_byReadPack;
		if(pPackgd != NULL){
			ushort wLen = CalcuGrcDataLen(pPackgd, 250);
			if(wLen-4 > 0) {
                memcpy(pTotalHead, ((uchar* )pPackgd)+4, wLen-4);//若是空的，不拷贝
				pTotalHead += (wLen-4);
				wCalLen += (wLen-4);
				wSum += (pPackgd[GENDATACOM_NGD] & 0x3f);
			}
		}
	}
	GenDataEntry(wFacNo, wLogicalNodeID, wSum, pTotalBuf, wCalLen);
	delete[] pTotalBuf;
}

void CProtocol::RGenRead11Handle(ushort wFacNo, ushort wLogicalNodeID, QList<CGrcReadPack*>* pReadPackList)
{
	ushort wTotalLen = 0;
	if(pReadPackList->isEmpty()) return;
	CGrcReadPack* pPackFirst = (CGrcReadPack* )pReadPackList->first();
	if(pPackFirst == NULL) return;

    foreach(CGrcReadPack* pPack,*pReadPackList) {
		wTotalLen += pPack->m_wReadPackLen;
	}
	uchar* pTotalBuf = new uchar[wTotalLen];
	uchar* pTotalHead = (uchar* )pTotalBuf;
	uchar* pGenRec = (uchar* )pTotalHead;
	uchar* pPackgd = (uchar* )pPackFirst->m_byReadPack;
	pGenRec[GENDESREC_GIN] = pPackgd[GENDESID_GIN];
	pGenRec[GENDESREC_GIN + 1] = pPackgd[GENDESID_GIN+1];
	pGenRec[GENDESREC_NUM] = 0;
	pTotalHead += 4;
	ushort wCalLen = 4;
    foreach(CGrcReadPack* pPack,*pReadPackList) {
		pPackgd = (uchar* )pPack->m_byReadPack;
		if(pPackgd!=NULL){
			ushort wLen = CalcuGrcIDLen(pPackgd,250);
			if(wLen-6 > 0) {
                memcpy(pTotalHead, ((uchar* )pPackgd)+6, wLen-6);//若是空的，不拷贝
				pTotalHead += (wLen-6);
				wCalLen += (wLen-6);
				pGenRec[GENDESREC_NUM] += (pPackgd[GENDESID_NGD] & 0x3f);
			}
		}
	}
	GenDataEntry(wFacNo, wLogicalNodeID, pTotalBuf, wCalLen);
	delete[] pTotalBuf;
}

void CProtocol::RYKYTHandle(ushort wFacNo, ushort wLogicalNodeID, 
								uchar* pDataCom, ushort wCalLen, uchar byCot)
{
	SDevID keyYKYT;
	keyYKYT.wFacNo = wFacNo;
	keyYKYT.wLogicalNodeId = wLogicalNodeID;
	CYKYTItem* pYKItem = FindYKYTItem(keyYKYT);
	if(pYKItem == NULL) return;
	uchar* GDR = &pDataCom[GENDATACOM_GDR];
	uchar* GID = &GDR[GENDATAREC_GID];
	if(byCot == 41){
		if(wCalLen > 21 && GDR[GENDATAREC_GDD] == 23 && 
			GDR[GENDATAREC_GDD+GDDTYPE_NUM] == 1 && GDR[GENDATAREC_GDD+GDDTYPE_SIZ] > 7
			&& GID[4] == 1 && GID[5] >= 1  && GID[6] == 1){
			char* pTmp = new char[GID[5] + 1];
            memcpy(pTmp, &GID[7], GID[5]);
			pTmp[GID[5]] = 0;
			NotifyYKYTCMD(pYKItem, false, pTmp);
			delete[] pTmp;
		}
		else NotifyYKYTCMD(pYKItem, false, "遥控反校失败");
	}
	else {
		ushort wLenTmp;
		if(pYKItem->m_bSCYK) 
			wLenTmp = 12;
		else wLenTmp = 11;

		if(wCalLen != wLenTmp ||!CompareData(pYKItem->m_bySndBuf, pDataCom, wLenTmp)) 
			NotifyYKYTCMD(pYKItem, false, "遥控反校失败");
		else {
			if(!pYKItem->m_bSCYK || pYKItem->m_YKData.byExecType == enumExecTypeSelect) {
				NotifyYKYTCMD(pYKItem, true);
				if(pYKItem->m_bAuto) {
					SYKYTData YKYTData;
					memcpy(&YKYTData, &pYKItem->m_YKData, sizeof(YKYTData));
					YKYTData.byExecType = enumExecTypeExecute;
					DeleteYKYTItem(keyYKYT);
					IYKYTSend(keyYKYT.wFacNo, keyYKYT.wLogicalNodeId, &YKYTData, false, true);
					return;
				}
			}
			else {
				pYKItem->m_dwCount = 0;
				pYKItem->m_bSCWaitForChk = false;
				return;
			}
		}
	}
	DeleteYKYTItem(keyYKYT);
	return;
}

void CProtocol::GenDataEntry(ushort wFacNo, ushort wLogicalNodeID, 
							 ushort wNum, uchar* pGenData, ushort wCalLen, uchar byCot)
{
	ushort wLen = 0;
	uchar* pHead = (uchar* )pGenData;
	for(uchar i = 0; i<wNum; i++){
		uchar* pCurRec = (uchar* )pHead;
		ushort wDataSize;
		if(pCurRec[GENDATAREC_GDD] == 2)// 成组位串
			wDataSize = ((pCurRec[GENDATAREC_GDD+GDDTYPE_SIZ]+7)/8) * pCurRec[GENDATAREC_GDD+GDDTYPE_NUM] + 6;
		else wDataSize = pCurRec[GENDATAREC_GDD+GDDTYPE_SIZ] * pCurRec[GENDATAREC_GDD+GDDTYPE_NUM] + 6;
		pHead += wDataSize;
		wLen += wDataSize;
		if(wLen > wCalLen) return;
		CGrcDataRec* pDataRec = new CGrcDataRec(wFacNo, wLogicalNodeID, wDataSize, byCot);
        memcpy(pDataRec->m_pGrcRec->pGenDataRec, pCurRec, wDataSize);
		GenDataHandle(pDataRec, false);
	}
}

void CProtocol::GenDataEntry(ushort wFacNo, ushort wLogicalNodeID,  uchar* pGenData, ushort wCalLen)
{
    char* pHead = (char* )&pGenData[GENDESREC_GDE];
	ushort wLen = 0;
	for(uchar i=0; i < pGenData[GENDESREC_NUM]; i++){
		uchar*	pCurRec = (uchar* )pHead;
		ushort wDataSize;
		if(pCurRec[GENDESELE_GDD] == 2)// 成组位串
			wDataSize = ((pCurRec[GENDESELE_GDD+GDDTYPE_SIZ]+7)/8) * pCurRec[GENDESELE_GDD+GDDTYPE_NUM] + 4;
		else wDataSize = pCurRec[GENDESELE_GDD+GDDTYPE_SIZ] * pCurRec[GENDESELE_GDD+GDDTYPE_NUM] + 4;
		wLen += wDataSize;
		pHead += wDataSize;
		if(wLen > wCalLen) return;
		CGrcDataRec* pDataRec = new CGrcDataRec(wFacNo, wLogicalNodeID, wDataSize+2, 42);
		uchar* pGIN = (uchar* )pDataRec->m_pGrcRec->pGenDataRec;
		pGIN[0] = pGenData[GENDESREC_GIN];
		pGIN[1] = pGenData[GENDESREC_GIN+1];
        memcpy(&pDataRec->m_pGrcRec->pGenDataRec[2], pCurRec, wDataSize);
		GenDataHandle(pDataRec, false);
	}
}

void CProtocol::GenDataHandle(CGrcDataRec* pDataRec, bool bIsFromWait)
{
	SGrcDataRec* pGrcRec = pDataRec->m_pGrcRec;
	uchar* pGenData = (uchar* ) pGrcRec->pGenDataRec;
	uchar byGroup = pGenData[GENDATAREC_GIN];
	uchar byGroupType = GetGroupTypeByGroupVal(pGrcRec->wFacNo, pGrcRec->wDevID, byGroup);

	if(byGroupType == (uchar )RETERROR){
		if(bIsFromWait) return;
		AddDataToWaitList(pDataRec);
		SCommandID keyCMD;
		keyCMD.wFacNo = pGrcRec->wFacNo;
		keyCMD.wLogicalNodeId = pGrcRec->wDevID;
		keyCMD.wGroupType = enumGroupTypeCatalog;
		CGenCommand* pCMD = FindGenCMD(keyCMD, true);
		if(pCMD == NULL) {
			pCMD = GetGenCMD(keyCMD, true);
			if(pCMD){
				pCMD->m_bIsReadType = true;
				pCMD->m_dwCount = 0;
				SGenReadCatalog(keyCMD.wFacNo, keyCMD.wLogicalNodeId);
			}
			else {
				pCMD = new CGenCommand(keyCMD);
				pCMD->m_bIsReadType = true;
				AddCatalogCMDToWaitList(pCMD);
			}
		}
		return;
	}
	GenDataEnterDB(pDataRec, (EPreDefinedGroupType) byGroupType);
	if(!bIsFromWait) delete pDataRec;
}

//扰动数据接收
//被纪录的扰动表
void CProtocol::HandleASDU23(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
	//当给定装置（给的是厂站地址，装置地址）的处理标志为禁止主动波形上送，就直接返回，不再进行录波
	if(!AppCommu::Instance()->AutoSendWave(wFacNo,wLogicalNodeID))
	{
		return;
	}
	uchar byNum = (*(pData+1))&0x7f;
	if(wLength < ((byNum*10)+6)) return;
	uchar* pTableRec = (uchar* )(pData+4);
	DistTableEnertDB(wFacNo, wLogicalNodeID, byNum, pTableRec);
}

//扰动数据传输准备就绪
void CProtocol::HandleASDU26(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
    Q_UNUSED(wLength);
	CDisturbFile* pDisturbFile;
	CDisturbItem* pDisturbItem;
	uchar* pDtbReady = (uchar* )(pData+4);	
	SDevID keyDTB;
	keyDTB.wFacNo = wFacNo;
	keyDTB.wLogicalNodeId = wLogicalNodeID;
	ushort wFAN = pDtbReady[DISTR_FAN] + pDtbReady[DISTR_FAN+1] * 256;
	if((pDisturbItem = FindDtbItem(keyDTB))==NULL) return;
	if(pDisturbItem->m_pDtbTable == NULL || pDisturbItem->m_pDtbTable->m_wFAN != wFAN) {
		NotifyDisturbCMD(pDisturbItem, false);
		DeleteDtbItem(keyDTB);
		return;
	}
	pDisturbItem->ResetTimer();
	if(pDisturbItem->m_pDtbFile == NULL)
		pDisturbItem->m_pDtbFile = new CDisturbFile;
	pDisturbFile = pDisturbItem->m_pDtbFile;
	pDisturbFile->Initialize();

	pDisturbFile->m_wFAN = wFAN;
	pDisturbFile->m_byFUN = pDtbReady[DISTR_FUN];
	pDisturbFile->m_byTOV = pDtbReady[DISTR_TOV];
	pDisturbFile->m_wNOF = AppCommu::Instance()->CharToShort(&pDtbReady[DISTR_NOF]);
	pDisturbFile->m_byNOC = pDtbReady[DISTR_NOC];
	pDisturbFile->m_wNOE = AppCommu::Instance()->CharToShort(&pDtbReady[DISTR_NOE]);
	pDisturbFile->m_wINT = AppCommu::Instance()->CharToShort(&pDtbReady[DISTR_INT]);
	uchar* pTime = &pDtbReady[DISTR_TIM]; 
	SShortTime shTime;
	shTime.byHour = pTime[SHORTTIME_HOUR];
	shTime.byMinute = pTime[SHORTTIME_MIN];
	shTime.wMilliseconds = pTime[SHORTTIME_MSEC];
	DNSYSTEMTIME sysTime;
	AppCommu::Instance()->ACETimeToDNTime(&pDisturbItem->m_pDtbTable->m_timeFault, &sysTime);
	AppCommu::Instance()->IShortTimeToSysTime(&shTime, &sysTime);
	QDate dTmp(sysTime.wYear, sysTime.wMonth, sysTime.wHour);
	QTime tTmp(sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
	QDateTime dtTmp(dTmp, tTmp);

	pDisturbFile->m_dwFirstTimeLow = dtTmp.toTime_t();
	pDisturbFile->m_dwFirstTimeHigh = tTmp.msec() * 1000;
	SendDisturbCmd(pDisturbItem, TOO_DTB_REQ, 0, pDtbReady[DISTR_TOV]);
}
//通道传输准备就绪
void CProtocol::HandleASDU27(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
    Q_UNUSED(wLength);
	uchar* pChanelReady = (uchar* )(pData+4);
	SDevID keyDTB;
	keyDTB.wFacNo = wFacNo;
	keyDTB.wLogicalNodeId = wLogicalNodeID;
	ushort wFAN = AppCommu::Instance()->CharToShort(&pChanelReady[DISTC_FAN]);
	CDisturbItem* pDisturbItem = FindDtbItem(keyDTB);
	if(pDisturbItem == NULL) return;
	if(pDisturbItem->m_pDtbTable == NULL || pDisturbItem->m_pDtbTable->m_wFAN != wFAN
													|| pDisturbItem->m_pDtbFile == NULL){
		NotifyDisturbCMD(pDisturbItem, false);
		DeleteDtbItem(keyDTB);
		return;
	}
	pDisturbItem->ResetTimer();
	CDisturbFile* pDisturbFile = pDisturbItem->m_pDtbFile;
	CChannelInfo* pChannel = pDisturbFile->FindChannelInfo(pChanelReady[DISTC_ACC]);
	if(pChannel==NULL) {
		pChannel = new CChannelInfo;// 添加新通道数据头
		pDisturbFile->m_plChannelInfoList.append(pChannel);
	}
	pChannel->m_aChannelData.resize(0);	
	pChannel->m_wFAN = wFAN;
	pChannel->m_byACC = pChanelReady[DISTC_ACC];
	if((pDisturbFile->m_wNOE)>0) 
		pChannel->m_aChannelData.resize(pDisturbFile->m_wNOE);
	for(ushort i=0; i<pDisturbFile->m_wNOE; i++)
        pChannel->m_aChannelData[i] = 0;
	pChannel->m_byTOV = AppCommu::Instance()->CharToFloat(&pChanelReady[DISTC_TOV]);
	pChannel->m_fRPV = AppCommu::Instance()->CharToFloat(&pChanelReady[DISTC_RPV]);
	pChannel->m_fRSV = AppCommu::Instance()->CharToFloat(&pChanelReady[DISTC_RSV]);
	pChannel->m_fRFA = AppCommu::Instance()->CharToFloat(&pChanelReady[DISTC_RFA]);

	SendDisturbCmd(pDisturbItem, TOO_CHANNEL_REQ, pChanelReady[DISTC_ACC], pChanelReady[DISTC_TOV]);
}
//状态变位传输准备就绪
void CProtocol::HandleASDU28(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
    Q_UNUSED(wLength);
	uchar* pTagReady = (uchar* )(pData+4);
	SDevID keyDTB;
	keyDTB.wFacNo = wFacNo;
	keyDTB.wLogicalNodeId = wLogicalNodeID;
	ushort wFAN = AppCommu::Instance()->CharToShort(&pTagReady[DISTT_FAN]);
	CDisturbItem* pDisturbItem = FindDtbItem(keyDTB);
	if(pDisturbItem == NULL) return;
	pDisturbItem->ResetTimer();
	if(pDisturbItem->m_pDtbTable == NULL || pDisturbItem->m_pDtbTable->m_wFAN != wFAN
													|| pDisturbItem->m_pDtbFile == NULL){
		NotifyDisturbCMD(pDisturbItem, false);
		DeleteDtbItem(keyDTB);
		return;
	}
	SendDisturbCmd(pDisturbItem, TOO_TAGS_REQ, 0, 0);
}
//传送状态变位
void CProtocol::HandleASDU29(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
	if(wLength < 11) return;
	uchar byNOT = *(pData+8);
    if(wLength<(((quint32)byNOT)*3+11)) return;
	uchar* pTagsTran = (uchar* ) (pData+4);
	SDevID keyDTB;
	keyDTB.wFacNo = wFacNo;
	keyDTB.wLogicalNodeId = wLogicalNodeID;
	ushort  wFAN = AppCommu::Instance()->CharToShort(&pTagsTran[DISTTT_FAN]);
	CDisturbItem* pDisturbItem = FindDtbItem(keyDTB);
	if(pDisturbItem == NULL) return;
	pDisturbItem->ResetTimer();
	if(pDisturbItem->m_pDtbTable == NULL || pDisturbItem->m_pDtbTable->m_wFAN != wFAN
													|| pDisturbItem->m_pDtbFile == NULL){
		NotifyDisturbCMD(pDisturbItem, false);
		DeleteDtbItem(keyDTB);
		return;
	}
	CDisturbFile* pDisturbFile = pDisturbItem->m_pDtbFile;
	CTagsGroup* pTagsGroup = new CTagsGroup;
	pTagsGroup->m_wFAN = AppCommu::Instance()->CharToShort(&pTagsTran[DISTTT_FAN]);
	pTagsGroup->m_byNOT = pTagsTran[DISTTT_NOT];
	pTagsGroup->m_wTAP = AppCommu::Instance()->CharToShort(&pTagsTran[DISTTT_TAP]);
	uchar* pTag = &pTagsTran[DISTTT_TAG];
	for(uchar i=0; i<byNOT; i++){
		CTags* pTags = new CTags;
		pTagsGroup->m_aTagsList.append(pTags);
		pTags->m_byFUN = pTag[DISTTA_FUN];
		pTags->m_byINF = pTag[DISTTA_INF];
		pTags->m_byDPI = pTag[DISTTA_DPI];
		pTag += 3;
	}
	pDisturbFile->m_plTagsGroupList.append(pTagsGroup);
}
//传送扰动值
void CProtocol::HandleASDU30(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
	if(wLength<15) return;
	uchar byNDV = *(pData + 11);
	if(wLength < (14+byNDV*2)) return;
	uchar* pChanelTran = (uchar* )(pData+4);
	SDevID keyDTB;
	keyDTB.wFacNo = wFacNo;
	keyDTB.wLogicalNodeId = wLogicalNodeID;
	ushort  wFAN = AppCommu::Instance()->CharToShort(&pChanelTran[DISTCT_FAN]);
	CDisturbItem* pDisturbItem = FindDtbItem(keyDTB);
	if(pDisturbItem == NULL) return;
	pDisturbItem->ResetTimer();
	if(pDisturbItem->m_pDtbTable == NULL || pDisturbItem->m_pDtbTable->m_wFAN != wFAN
													|| pDisturbItem->m_pDtbFile == NULL){
		NotifyDisturbCMD(pDisturbItem, false);
		DeleteDtbItem(keyDTB);
		return;
	}
	CDisturbFile *pDisturbFile = pDisturbItem->m_pDtbFile;

	// 除非找到通道数据头,否则不作处理
	CChannelInfo* pChannel;
	if((pChannel = pDisturbFile->FindChannelInfo(pChanelTran[DISTCT_ACC]))==NULL) return;

	ushort Offset = AppCommu::Instance()->CharToShort(&pChanelTran[DISTCT_NFE]);
	ushort wMaxIndex = pChannel->m_aChannelData.size();
	// 添加合理数据到适当位置
	uchar* SDV = &pChanelTran[DISTCT_SDV];
	for(ushort i=0; i<pChanelTran[DISTCT_NDV] && (i+Offset)<wMaxIndex; i++)
	{
		pChannel->m_aChannelData[i+Offset] = AppCommu::Instance()->CharToShort(SDV);
		SDV += 2;
	}
}
//传送结束
void CProtocol::HandleASDU31(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
    Q_UNUSED(wLength);
	uchar* pTranOver = (uchar* )(pData+4);
	SDevID keyDTB;
	keyDTB.wFacNo = wFacNo;
	keyDTB.wLogicalNodeId = wLogicalNodeID;
	ushort wFAN = AppCommu::Instance()->CharToShort(&pTranOver[DISTO_FAN]);
	CDisturbItem* pDisturbItem = FindDtbItem(keyDTB);
	if(pDisturbItem == NULL) return;
	if(pDisturbItem->m_pDtbTable == NULL || pDisturbItem->m_pDtbTable->m_wFAN != wFAN){
		NotifyDisturbCMD(pDisturbItem, false);
		DeleteDtbItem(keyDTB);
		return;
	}
	pDisturbItem->ResetTimer();

	if(pTranOver[DISTO_TOO]==TOO_TAGS_OVER)
		SendDisturbOver(pDisturbItem, TOO_TAGS_TRANS_OK, 0, pTranOver[DISTO_TOV]);
	else if(pTranOver[DISTO_TOO]==TOO_CHANNEL_OVER) 
		SendDisturbOver(pDisturbItem, TOO_CHANNEL_TRANS_OK, pTranOver[DISTO_ACC], pTranOver[DISTO_TOV]);
	else if(pTranOver[DISTO_TOO]==TOO_DTB_OVER){
		SendDisturbOver(pDisturbItem, TOO_DTB_TRANS_OK, 0, pTranOver[DISTO_TOV]);
		DistDataEnterDB(wFacNo, wLogicalNodeID, pDisturbItem);
		DeleteDtbItem(keyDTB);
	}
	else if(pTranOver[DISTO_TOO]==TOO_DTB_OVEREX_SYS||pTranOver[DISTO_TOO]==TOO_DTB_OVEREX_DEV
		||pTranOver[DISTO_TOO]==TOO_TAGS_OVEREX_SYS||pTranOver[DISTO_TOO]==TOO_TAGS_OVEREX_DEV
		||pTranOver[DISTO_TOO]==TOO_CHANNEL_OVEREX_SYS||pTranOver[DISTO_TOO]==TOO_CHANNEL_OVEREX_DEV){
		SendDisturbOver(pDisturbItem, TOO_DTB_TRANS_FAIL, 0, pTranOver[DISTO_TOV]);
		NotifyDisturbCMD(pDisturbItem, true);
		DeleteDtbItem(keyDTB);
	}
}

void CProtocol::HandleASDU5(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
	if(wLength < 19) return;
	if(((*(pData+2)) != COT_START) && ((*(pData+2)) != COT_POWER_ON)) return;

	CDevice* pDev = FindDeviceDstb(wFacNo, wLogicalNodeID);
	if(pDev && pDev->m_byDevType != NODETYPE_RECORDDEV) pDev->m_bIsNeedGeneralQuary = true;

}

void CProtocol::HandleASDU6(uchar* pData, ushort wLength, ushort wFacNo, ushort wDevID)
{
    Q_UNUSED(wLength);
    Q_UNUSED(wFacNo);
    Q_UNUSED(wDevID);
	if(pData[ASDU_VSQ] != 0x81 || pData[ASDU_COT] != COT_TIMESYNC || pData[ASDU_FUN] != 255 ) return;
	m_wSyncTimer = OVERTIMER_SYNCTIMER;
	uchar* pTime = &pData[6];
	SLongTime lTime;
	lTime.wMilliseconds = pTime[LONGTIME_MSEC];
	lTime.byMinute = pTime[LONGTIME_MIN];
	lTime.byHour = pTime[LONGTIME_HOUR];
	lTime.byDay = pTime[LONGTIME_DAY];
	lTime.byMonth = pTime[LONGTIME_MON];
	lTime.byYear = pTime[LONGTIME_YEAR];
    QDateTime FileTime;
	if(!AppCommu::Instance()->ILongTimeToFileTime(&lTime, &FileTime)) return;
	SyncTimeEnterDB(&FileTime);
}

void CProtocol::HandleASDU229(uchar* pData, ushort wLength, ushort wFacNo, ushort wDevID)
{
	if(wLength != 15) return;
	if(pData[2] != 20 && pData[2] != 21) return;
	if(pData[7] != 1) return;
	uchar* pID = (uchar* )&pData[8];
	SYKCheckDataBack* pYKCheckBck = new SYKCheckDataBack;
	pYKCheckBck->wFacAddr = AppCommu::Instance()->CharToShort(&pID[0]);
	pYKCheckBck->wDevAddr = AppCommu::Instance()->CharToShort(&pID[2]);
	pYKCheckBck->wGIN = AppCommu::Instance()->CharToShort(&pID[4]);
	pYKCheckBck->byOPType = pID[6];
	if(pData[2] == 20) pYKCheckBck->bIsSuccess = true;
	else pYKCheckBck->bIsSuccess = false;
	pYKCheckBck->wSrcFacAddr = wFacNo;
	pYKCheckBck->wSrcDevAddr = wDevID;
	AppCommu::Instance()->YKCheckBack(pYKCheckBck);
	delete pYKCheckBck;
}

/*********************************************************************************/
/*								定时处理部分									 */
/*********************************************************************************/
void CProtocol::OnTimer()
{
	if(m_wSyncTimer) m_wSyncTimer--;
	g_mutexProtocolDataUnpack.lock();
	OverJudgGenCMD();
	OverJudgHisCMD();
	OverJudgDtbItem();
	OverJudgYKYTItem();

	TimerWaitData();
	RawDataHandle();

	bool bAsk = false;
    foreach(CDevice* pDev,m_plDeviceDstb) {
		pDev->OnTimer();
		if(pDev->m_byDevType <= NODETYPE_ROUTER) continue;
		if(pDev->m_bIsNeedGeneralQuary && 
			!JudgDevIsSendGenCMD(pDev->m_wFacNo, pDev->m_wDevID) && !bAsk){
			SendGeneralQuary(pDev->m_wFacNo, pDev->m_wDevID);
			pDev->GeneralQuaryOver();
			bAsk = true;
		}
		else if(pDev->m_byGetChannelNameTimes && !pDev->m_byQuaryTimer && 
					!JudgDevIsSendGenCMD(pDev->m_wFacNo, pDev->m_wDevID)){
			IGenReadItem(pDev->m_wFacNo, pDev->m_wDevID, enumGroupTypeDtbInf, enumDevReadAll, -1);
			pDev->m_byGetChannelNameTimes--;
		}
		else if(pDev->m_byGetFaultInfTimes && !pDev->m_byQuaryTimer && 
					!JudgDevIsSendGenCMD(pDev->m_wFacNo, pDev->m_wDevID)){
			IGenReadItem(pDev->m_wFacNo, pDev->m_wDevID, enumGroupTypeFaultInf, enumDevReadAll, -1);
			pDev->m_byGetFaultInfTimes--;
		}
		else if(pDev->m_bIsNeedPulse && !pDev->m_byQuaryTimer && 
					!JudgDevIsSendGenCMD(pDev->m_wFacNo, pDev->m_wDevID) ){
			pDev->m_bIsNeedPulse = false;
			SCommandID keyCMD;
			keyCMD.wFacNo = pDev->m_wFacNo;
			keyCMD.wLogicalNodeId = pDev->m_wDevID;
			keyCMD.wGroupType = enumGroupTypeYM;
			uchar	byGroupValue[MAXGROUPITEM];
			if(GetGroupByGroupType(keyCMD, byGroupValue, MAXGROUPITEM) <= 0) return;
			IGenReadItem(pDev->m_wFacNo, pDev->m_wDevID, enumGroupTypeYM, enumDevReadValue, -1);
		}
	}

	g_mutexProtocolDataUnpack.unlock();

	TimerWaitCommand();
	if(m_pFileTrans) m_pFileTrans->OnTimer();
}

void CProtocol::TimerWaitData()
{
    int pos=0;
    while(pos<m_plGenWaitForHandle.count()) {
        CGrcDataRec* pDataRec = m_plGenWaitForHandle.at(pos);
		if(!(--pDataRec->m_wDataWaitTime)){
            m_plGenWaitForHandle.removeAt(pos);
			delete pDataRec;
		}
        else{
            pos++;
        }
	}

}

void CProtocol::RawDataHandle()
{
	while(1){
		CRawData* pRawData = RemoveRawDataFromWaitList();
		if(pRawData != NULL){
			DataAnalysis(pRawData->m_wFacNo, pRawData->m_wDevID, pRawData->m_pDataBuf, pRawData->m_wLen);
			delete pRawData;
		}
		else break;
	}
}

void CProtocol::TimerWaitCommand()
{
	g_mutexProtocolDbCMD.lock();

    int pos=0;
    while(pos<m_plDbCommandWait.count()) {
        CDbCommand* pDbCMD = m_plDbCommandWait.at(pos);
		if(!(--pDbCMD->m_dwCount)){
			NotifyDbCommand(pDbCMD, false);
            m_plDbCommandWait.removeAt(pos);
			delete pDbCMD;
            continue;
		}
		else {
			if(pDbCMD->m_byDBCMDType == DBCMD_YKYT){
				CYKYTItem* pYKYTItem = (CYKYTItem* )pDbCMD;
				if(AddYKYTItem(pYKYTItem)){
                    m_plDbCommandWait.removeAt(pos);
					pYKYTItem->m_dwCount = 0;
					SendYKYTCommand(pYKYTItem);
                    continue;
				}
			}
			else if(pDbCMD->m_byDBCMDType == DBCMD_GEN){
				CGenCommand* pGenCMD = (CGenCommand* )pDbCMD;
				if(AddGenCMD(pGenCMD)){
					pGenCMD->m_dwCount = 0;
                    m_plDbCommandWait.removeAt(pos);
					if(pGenCMD->m_bIsReadType)	SendReadCommand(pGenCMD);
					else SGenWriteCMD(pGenCMD);
                    continue;
				}
			}
			else if(pDbCMD->m_byDBCMDType == DBCMD_HIS){
				CHisCommand* pHisCMD = (CHisCommand* )pDbCMD;
				if(AddHisCMD(pHisCMD)){
                    m_plDbCommandWait.removeAt(pos);
					pHisCMD->m_dwCount = 0;
					SReadHistoryData(pHisCMD);
                    continue;
				}
			}
			else if(pDbCMD->m_byDBCMDType == DBCMD_DTB){
				CDisturbItem* pDtbItem = (CDisturbItem* )pDbCMD;
				if(AddDtbItem(pDtbItem)){
                    m_plDbCommandWait.removeAt(pos);
					pDtbItem->m_dwCount = 0;
					SendDisturbCmd(pDtbItem, TOO_FAULT_SELECT, 0, 0);
                    continue;
				}
			}
			else {
                m_plDbCommandWait.removeAt(pos);
				delete pDbCMD;
                continue;
			}
		}
        pos++;
	}
	g_mutexProtocolDbCMD.unlock();
}

void CProtocol::AddRawDataToWaitList(CRawData* pRawData)
{
	g_mutexProtocolRawDataList.lock();
	if(m_plRawWaitForHandle.count() > MAX_RAWDATA) 
        delete m_plRawWaitForHandle.takeFirst();
	m_plRawWaitForHandle.append(pRawData);
	g_mutexProtocolRawDataList.unlock();
}

CRawData* CProtocol::RemoveRawDataFromWaitList()
{
	CRawData* pRawData = NULL;
	g_mutexProtocolRawDataList.lock();
	if( !m_plRawWaitForHandle.isEmpty() ) 
        pRawData = m_plRawWaitForHandle.takeFirst();
	g_mutexProtocolRawDataList.unlock();
	return pRawData;
}

void CProtocol::AddCMDToWaitList(CDbCommand* pDbCMD, ushort wWaitTime)
{
	g_mutexProtocolDbCMD.lock();
	if(pDbCMD->m_byDBCMDType == DBCMD_GEN){
		CGenCommand* pNew = (CGenCommand* )pDbCMD;
		bool bFind = false;
		if(pNew->m_keyCMD.wGroupType == enumGroupTypeCatalog && pNew->m_bIsReadType){
            foreach(CDbCommand* pDbCMDOld,m_plDbCommandWait) {
				if(pDbCMDOld->m_byDBCMDType != DBCMD_GEN) continue;
				CGenCommand* pOld = (CGenCommand* )pDbCMDOld;
				if(pOld->m_bIsReadType && pNew->m_keyCMD == pOld->m_keyCMD){
					bFind = true;
					break;
				}
			}
		}
		if(!bFind){
			pDbCMD->m_dwCount = wWaitTime;
			m_plDbCommandWait.append(pDbCMD);
		}
		else delete pDbCMD;
	}
	else {
		pDbCMD->m_dwCount = wWaitTime;
		m_plDbCommandWait.append(pDbCMD);
	}
	g_mutexProtocolDbCMD.unlock();
}

void CProtocol::AddCatalogCMDToWaitList(CGenCommand* pCMD)
{
	bool bFind = false;
	g_mutexProtocolDbCMD.lock();
    foreach(CDbCommand* pDbCMD,m_plDbCommandWait) {
		if(pDbCMD->m_byDBCMDType != DBCMD_GEN) continue;
		CGenCommand* pGenCMD = (CGenCommand* )pDbCMD;
		if(pGenCMD->m_bIsReadType && pGenCMD->m_keyCMD == pCMD->m_keyCMD) {
			bFind = true;
			break;
		}
	}
	if(!bFind){
		pCMD->m_dwCount = OVERTIMER_DBGENWAITCMD;
		m_plDbCommandWait.append(pCMD);
	}
	else delete pCMD;
	g_mutexProtocolDbCMD.unlock();
}

void CProtocol::AddDataToWaitList(CGrcDataRec* pDataRec)
{
	if(m_plGenWaitForHandle.count() > MAX_WAITDATA)
        delete m_plGenWaitForHandle.takeFirst();
	m_plGenWaitForHandle.append(pDataRec);
}

