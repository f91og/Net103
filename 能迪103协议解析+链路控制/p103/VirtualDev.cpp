#include "appcommu.h"
#include "protocol.h"
#include <qdatetime.h>


bool CProtocol::IVirtualAutoSendYX(ushort wVDevNo, ushort  wNum, SVirtualYXSend* pYXSend)
{
	ushort wSendTime = wNum / 10;
	ushort  wIndex = 0;
	char* pData = new char[128];
	pData[ASDU_TYPE] = (char) TYPW_GRC_DATA;
	pData[ASDU_VSQ] = (char) VSQ_COMMON1;
	pData[ASDU_COT] = (char) COT_SPONTANEOUS;
	pData[ASDU_ADDR] = (char) 0;
	pData[ASDU_FUN] = (char) 254;
	pData[ASDU_INF] = (char) 244;
	pData[ASDU21_RII] = (char) 0;
	pData[ASDU21_NOG] = (char) 10;
	char* pHead = pData + 8;

	for(ushort i=0; i<wSendTime; i++){
		for(uchar j=0; j<10; j++){
			SVirtualYXSend* pCurYX = pYXSend + wIndex;
			wIndex++;
			MakeYXSend(pHead, pCurYX);
			pHead += 12;
		}
		AppCommu::Instance()->ProtocolVirtualSend(0xffff, 0xffff, wVDevNo, pData, 128);
	}
	uchar byNum = (uchar )(wNum - wIndex);
	pData[ASDU21_NOG] = byNum;
	if(byNum){
		for(; wIndex<wNum; wIndex++){
			SVirtualYXSend* pCurYX = pYXSend + wIndex;
			MakeYXSend(pHead, pCurYX);
			pHead += 12;
		}
		AppCommu::Instance()->ProtocolVirtualSend(0xffff, 0xffff, wVDevNo, pData, 8+12*byNum);
	}
	delete[] pData;
    return true;
}

bool CProtocol::IVirtualAutoSendYC(ushort wVDevNo, ushort  wNum, SVirtualYCSend* pYCSend)
{
	ushort wSendTime = wNum / 10;
	ushort  wIndex = 0;
	char* pData = new char[128];

	pData[ASDU_TYPE] = (char) TYPW_GRC_DATA;
	pData[ASDU_VSQ] = (char) VSQ_COMMON1;
	pData[ASDU_COT] = (char) COT_CYCLIC;
	pData[ASDU_ADDR] = (char) 0;
	pData[ASDU_FUN] = (char) 254;
	pData[ASDU_INF] = (char) 244;
	pData[ASDU21_RII] = (char) 0;
	pData[ASDU21_NOG] = (char) 10;
	char* pHead = pData + 8;

	for(ushort i=0; i<wSendTime; i++){
		for(uchar j=0; j<10; j++){
			SVirtualYCSend* pCurYC = pYCSend + wIndex;
			MakeYCSend(pHead, pCurYC);
			pHead += 8;
			wIndex++;
		}
		AppCommu::Instance()->ProtocolVirtualSend(0xffff, 0xffff, wVDevNo, pData, 88);
	}
	uchar byNum = (uchar )(wNum - wIndex);
	if(byNum){
		int i=0;
		for(; wIndex<wNum; wIndex++){
			SVirtualYCSend* pCurYC = pYCSend + wIndex;
			MakeYCSend(pHead, pCurYC);
			pHead += 8;
			i++;
		}
		AppCommu::Instance()->ProtocolVirtualSend(0xffff, 0xffff, wVDevNo, pData, 8+8*byNum);
	}
	delete[] pData;
	return true;
}

void CProtocol::MakeYXSend(char* pData, SVirtualYXSend* pCurYX)
{
	if(pCurYX == NULL) return;
	pData[0] = pCurYX->wGIN % 256;
	pData[1] = pCurYX->wGIN / 256;
	pData[2] = KOD_ACTUALVALUE;
	pData[3] = 18;
	pData[4] = 6;
	pData[5] = 1;
	if(pCurYX->byValue) pData[6] = 2;
	else pData[6] = 1;
	QDateTime dtTmp;
	dtTmp.setTime_t(pCurYX->timeOccur);
	QTime tTmp = dtTmp.time();
	ushort wMsec = tTmp.second() * 1000;
	pData[7] = wMsec % 256;
	pData[8] = wMsec / 256;
	pData[9] = tTmp.minute();
	pData[10] = tTmp.hour();
	pData[11] = 0;
}

void CProtocol::MakeYCSend(char* pData, SVirtualYCSend* pCurYC)
{
	if(pCurYC == NULL) return;
	pData[0] = pCurYC->wGIN % 256;
	pData[1] = pCurYC->wGIN / 256;
	pData[2] = KOD_ACTUALVALUE;
	pData[3] = 12;
	pData[4] = 2;
	pData[5] = 1;

	ushort wVal;
	if(pCurYC->fValue > 32767 || pCurYC->fValue < -32767) wVal = 0xfff9;
	else {
		wVal = (ushort) (pCurYC->fValue/* * 4096 / 1.2*/);
		wVal = wVal << 3;
	}
	pData[6] = wVal % 256;
	pData[7] = wVal / 256;
}

void CProtocol::IVirtualDataUnpack(ushort wFacNo, ushort wDevID, ushort wVDevID, uchar* pData,ushort wLength)
{
	if(!AppCommu::Instance()->m_bIsHostNode) return;
	switch(pData[0]){
	case 10:
		VirtualHandleASDU10(pData, wLength, wFacNo, wDevID, wVDevID);
		break;
	case 21:
		VirtualHandleASDU21(pData, wLength, wFacNo, wDevID, wVDevID);
		break;
	case 228:
		VirtualHandleASDU228(pData, wLength, wFacNo, wDevID, wVDevID);
		break;
	case 6:
		if(wLength < 13) return;
		HandleASDU6(pData, wLength, wFacNo, wDevID);
		break;
	}
}

void CProtocol::VirtualHandleASDU21(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID, ushort wVDevID)
{
	if(wLength < 8 ) return;
	if(pData[ASDU_VSQ] != 0x81 || pData[ASDU_FUN] != 254) return;
	if(pData[ASDU_COT] == COTC_GENQUERY && pData[ASDU_INF] == 245){
		VirtualGenQuery(wFacNo, wLogicalNodeID, wVDevID, pData[ASDU21_RII]);
		return;
	}
	if(pData[ASDU_COT] != COTC_GRC_READ ) return;
	if(pData[ASDU_INF] == 240 && pData[ASDU21_NOG] == 0)
		VirtualSendCatalog(wFacNo, wLogicalNodeID, wVDevID, pData[ASDU21_RII]);
	else if(pData[ASDU_COT] == 241 &&  pData[ASDU21_NOG] == 1){
/*		SGenIDRec* pIDRec = (SGenIDRec* )(pData + 8);
		if(pIDRec->byKOD != KOD_ACTUALVALUE) return;
		if(!JudgIsVirtualYXGroup(pIDRec->GIN.byGROUP)) return;
		VirtualSendYXAll(wFacNo, wLogicalNodeID, pIDRec->GIN.byGROUP, pAsdu->byRII);
*/	}
	else if(pData[ASDU_COT] == 243 && pData[ASDU21_NOG] == 1){
/*		SGenIDRec* pIDRec = (SGenIDRec* )(pData + 8);
		if(pIDRec->byKOD != 0) return;
		if(!JudgIsVirtualYXGroup(pIDRec->GIN.byGROUP)) return;
		VirtualSendYXSingle(wFacNo, wLogicalNodeID, pIDRec->wGIN, pAsdu->byRII);
*/	}
	else if(pData[ASDU_COT] == 244){
/*		if((pAsdu->byNOG * 3 + 8) > wLength) return;
		SGenIDRec* pIDRec = (SGenIDRec* )(pData + 8);
		if(pIDRec->byKOD != KOD_ACTUALVALUE) return;
		if(!JudgIsVirtualYXGroup(pIDRec->GIN.byGROUP)) return;
		VirtualSendYXSpecify(wFacNo, wLogicalNodeID, pIDRec, pAsdu->byNOG, pAsdu->byRII);
*/	}
}

void CProtocol::VirtualSendCatalog(ushort wFacNo, ushort wLogicalNodeID, ushort wVDevID, uchar byRII)
{
	SGetVirtualGroup* pVirYKGroup = new SGetVirtualGroup;
	pVirYKGroup->wVDevID = wVDevID;
	pVirYKGroup->byGroupType = enumGroupTypeYK;
	pVirYKGroup->byGroupNum = 0;
	AppCommu::Instance()->GetVirtualGroup(pVirYKGroup);
	SGetVirtualGroup* pVirYXGroup = new SGetVirtualGroup;;
	pVirYXGroup->wVDevID = wVDevID;
	pVirYXGroup->byGroupType = enumGroupTypeYX;
	pVirYXGroup->byGroupNum = 0;
	AppCommu::Instance()->GetVirtualGroup(pVirYXGroup);
	SGetVirtualGroup* pVirYCGroup = new SGetVirtualGroup;;
	pVirYCGroup->wVDevID = wVDevID;
	pVirYCGroup->byGroupType = enumGroupTypeYC;
	pVirYCGroup->byGroupNum = 0;
	AppCommu::Instance()->GetVirtualGroup(pVirYCGroup);

	uchar byTotal = 8 + 10 * (pVirYKGroup->byGroupNum + pVirYXGroup->byGroupNum + pVirYCGroup->byGroupNum);
	if(byTotal < 8){
		delete pVirYKGroup;
		delete pVirYXGroup;
		delete pVirYCGroup;
		return;
	}
	char* pData = new char[byTotal];
	pData[ASDU_TYPE] = (char) TYPW_GRC_DATA;
	pData[ASDU_VSQ] = (char) VSQ_COMMON1;
	pData[ASDU_COT] = (char) COT_GRCR_VALID;
	pData[ASDU_ADDR] = (char) 0;
	pData[ASDU_FUN] = (char) 254;
	pData[ASDU_INF] = (char) 240;
	pData[ASDU21_RII] = byRII;
	pData[ASDU21_NOG] = pVirYKGroup->byGroupNum + pVirYXGroup->byGroupNum + pVirYCGroup->byGroupNum;

	char* pHead = (char* )(pData + 8);
	uchar i;
	for(i=0; i<pVirYKGroup->byGroupNum; i++){
		pHead[0] = pVirYKGroup->byGroupVal[i];
		pHead[1] = 0;
		pHead[2] = KOD_DESCRIPTION;
		pHead[3] = 1;
		pHead[4] = 4;
		pHead[5] = 1;

		char byName[5];
        sprintf(byName, "遥控");
        memcpy(&pHead, byName, 4);
		pHead += 10;
	}
	for(i=0 ; i<pVirYXGroup->byGroupNum; i++){
		pHead[0] = pVirYXGroup->byGroupVal[i];
		pHead[1] = 0;
		pHead[2] = KOD_DESCRIPTION;
		pHead[3] = 1;
		pHead[4] = 4;
		pHead[5] = 1;

		char byName[5];
        sprintf(byName, "遥信");
        memcpy(&pHead, byName, 4);
		pHead += 10;
	}
	for(i=0 ; i<pVirYCGroup->byGroupNum; i++){
		pHead[0] = pVirYCGroup->byGroupVal[i];
		pHead[1] = 0;
		pHead[2] = KOD_DESCRIPTION;
		pHead[3] = 1;
		pHead[4] = 4;
		pHead[5] = 1;

		char byName[5];
        sprintf(byName, "遥测");
        memcpy(&pHead, byName, 4);
		pHead += 10;

	}
	AppCommu::Instance()->ProtocolVirtualSend(wFacNo, wLogicalNodeID, wVDevID, pData, byTotal);
	delete[] pData;
	delete pVirYXGroup;
	delete pVirYKGroup;
	delete pVirYCGroup;
}

void CProtocol::VirtualYXOut(ushort wFacNo, ushort wLogicalNodeID, ushort wVDevID, 
							 SGetVirtualYXAll* pYXAll, uchar byRII, uchar byInf, uchar byCOT)
{
	if(pYXAll->wNum <= 0) return;
	char* pData = new char[128];	
	pData[ASDU_TYPE] = (char) TYPW_GRC_DATA;
	pData[ASDU_VSQ] = (char) VSQ_COMMON1;
	pData[ASDU_COT] = byCOT;
	pData[ASDU_ADDR] = (char) 0;
	pData[ASDU_FUN] = (char) 254;
	pData[ASDU_INF] = byInf;
	pData[ASDU21_RII] = byRII;
	pData[ASDU21_NOG] = (char) 10;
	pData[ASDU21_NOG] |= 0x80;
	char* pHead = pData + 8;

	char* pHeadBak = pHead;
	ushort wSendTime = pYXAll->wNum / 10;
	ushort wIndex = 0;
	for(ushort i=0; i<wSendTime; i++){
		for(uchar j=0; j<10; j++){
			SVirtualYXSend* pCurYX = pYXAll->pYX + wIndex;
			wIndex++;
			MakeYXSend(pHead, pCurYX);
			pHead += 12;
		}
		if(wIndex == pYXAll->wNum ) pData[ASDU21_NOG] &= 0x7f;
		AppCommu::Instance()->ProtocolVirtualSend(wFacNo, wLogicalNodeID, wVDevID, pData, 128);
		if(pData[ASDU21_NOG] & 0x40) pData[ASDU21_NOG] |= 0x40;
		else pData[ASDU21_NOG] &= 0xbf;
	}
	uchar byNum = (uchar )(pYXAll->wNum - wIndex);
	pData[ASDU21_NOG] = byNum;
	pData[ASDU21_NOG] &= 0x7f;
	if(byNum){
		int i=0;
		for(; wIndex<pYXAll->wNum; wIndex++){
			SVirtualYXSend* pCurYX = pYXAll->pYX + wIndex;
			MakeYXSend(pHeadBak, pCurYX);
			i++;
			pHeadBak += 12;
		}
		AppCommu::Instance()->ProtocolVirtualSend(wFacNo, wLogicalNodeID, wVDevID, pData, 8+12*byNum);
	}
	delete[] pData;
}

void CProtocol::VirtualYCOut(ushort wFacNo, ushort wLogicalNodeID, ushort wVDevID, 
							 SGetVirtualYCAll* pYCAll, uchar byRII, uchar byInf, uchar byCOT)
{
	if(pYCAll->wNum <= 0) return;
	char* pData = new char[128];	
	pData[ASDU_TYPE] = (char) TYPW_GRC_DATA;
	pData[ASDU_VSQ] = (char) VSQ_COMMON1;
	pData[ASDU_COT] = byCOT;
	pData[ASDU_ADDR] = (char) 0;
	pData[ASDU_FUN] = (char) 254;
	pData[ASDU_INF] = byInf;
	pData[ASDU21_RII] = byRII;
	pData[ASDU21_NOG] = (char) 10;
	pData[ASDU21_NOG] |= 0x80;

	char* pHead = pData + 8;
	char* pHeadBak = pHead;
	ushort wSendTime = pYCAll->wNum / 10;
	ushort wIndex = 0;
	for(ushort i=0; i<wSendTime; i++){
		for(uchar j=0; j<10; j++){
			SVirtualYCSend* pCurYC = pYCAll->pYC + wIndex;
			wIndex++;
			MakeYCSend(pHead, pCurYC);
			pHead += 8;
		}
		if(wIndex == pYCAll->wNum ) pData[ASDU21_NOG] &= 0x7f;
		AppCommu::Instance()->ProtocolVirtualSend(wFacNo, wLogicalNodeID, wVDevID, pData, 88);
		if(pData[ASDU21_NOG] & 0x40) pData[ASDU21_NOG] |= 0x40;
		else pData[ASDU21_NOG] &= 0xbf;
	}
	uchar byNum = (uchar )(pYCAll->wNum - wIndex);
	pData[ASDU21_NOG] = byNum;
	pData[ASDU21_NOG] &= 0x7f;
	if(byNum){
		int i=0;
		for(; wIndex<pYCAll->wNum; wIndex++){
			SVirtualYCSend* pCurYC = pYCAll->pYC + wIndex;
			MakeYCSend(pHeadBak, pCurYC);
			i++;
			pHeadBak += 8;
		}
		AppCommu::Instance()->ProtocolVirtualSend(wFacNo, wLogicalNodeID, wVDevID, pData, 8+8 * byNum);
	}
	delete[] pData;
}

void CProtocol::VirtualHandleASDU10(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID, ushort wVDevID)
{
	if(wLength < 15) return;
	if(pData[ASDU_VSQ] != VSQ_COMMON1 || pData[ASDU_FUN] != 254 || pData[ASDU21_NOG] != 1
		|| pData[8 + GENDATAREC_KOD] != KOD_ACTUALVALUE || (pData[8 + GENDATAREC_GDD + GDDTYPE_NUM] & 0x7f) != 1 ||
		pData[8 + GENDATAREC_GDD + GDDTYPE_SIZ] != 1 || pData[8 + GENDATAREC_GDD + GDDTYPE_TYP] != 9 ) return;
	ushort wGIN = pData[8] * 256 + pData[9];
	if(pData[ASDU_COT] == COTC_GRC_WRITE && pData[ASDU_INF] == 249){
		if(m_pCurYKInf == NULL) m_pCurYKInf = new SVirtualYKYTData;
		m_pCurYKInf->wFacNo = wFacNo;
		m_pCurYKInf->wDevID = wLogicalNodeID;
		m_pCurYKInf->wVDevID = wVDevID;
		m_pCurYKInf->wGIN = wGIN;
		if(pData[8 + GENDATAREC_GID] == DCO_ON) m_pCurYKInf->byCommandType = (uchar) enumYKTypeOnUp;
		else if(pData[8 + GENDATAREC_GID] == DCO_OFF) m_pCurYKInf->byCommandType = (uchar) enumYKTypeOffDown;
		else {
			delete m_pCurYKInf;
			m_pCurYKInf = NULL;
			return;
		}
		m_pCurYKInf->byExecType = enumExecTypeSelect;
		pData[ASDU_COT] = COT_GRCW_CFM;
		AppCommu::Instance()->ProtocolVirtualSend(wFacNo, wLogicalNodeID, wVDevID, (char* )pData, wLength);
	}
	else if(pData[ASDU_COT] == COTC_GRC_WRITE && pData[ASDU_INF] == 250){
		if(m_pCurYKInf == NULL) return;
		uchar byCMD;
		if(pData[8 + GENDATAREC_GID] == DCO_ON) byCMD = (uchar) enumYKTypeOnUp;
		else if(pData[8 + GENDATAREC_GID] == DCO_OFF) byCMD = (uchar) enumYKTypeOffDown;
		else {
			delete m_pCurYKInf;
			m_pCurYKInf = NULL;
			return;
		}
		if((m_pCurYKInf->wVDevID != wVDevID) || (m_pCurYKInf->byExecType != (uchar) enumExecTypeSelect) ||
			(m_pCurYKInf->wFacNo != wFacNo ) || (m_pCurYKInf->wDevID != wLogicalNodeID) || 
			(m_pCurYKInf->wGIN != wGIN) || (m_pCurYKInf->byCommandType != byCMD)) {
			delete m_pCurYKInf;
			m_pCurYKInf = NULL;
			return;
		}
		if(AppCommu::Instance()->RNotifyVirtualYK(wVDevID, wGIN, byCMD))
			AppCommu::Instance()->ProtocolVirtualSend(wFacNo, wLogicalNodeID, wVDevID, (char* )pData, wLength);
	}
	else {
		if(m_pCurYKInf) {
			delete m_pCurYKInf;
			m_pCurYKInf = NULL;
		}
	}
}

void CProtocol::VirtualGenQuery(ushort wFacNo, ushort wLogicalNodeID, ushort wVDevID, uchar byRII)
{
	ushort wNum = AppCommu::Instance()->GetVirtualYXNum(wVDevID);
	if(wNum) {
		SGetVirtualYXAll* pGetYXAll = new SGetVirtualYXAll;
		pGetYXAll->wVDevID = wVDevID;
		pGetYXAll->wNum = wNum;
		pGetYXAll->byGroup = -1;
		pGetYXAll->pYX = new SVirtualYXSend[wNum];
		AppCommu::Instance()->GetVirtualYXAll(pGetYXAll);
		VirtualYXOut(wFacNo, wLogicalNodeID, wVDevID, pGetYXAll, byRII, 241, COT_GI);
		delete[] pGetYXAll->pYX;
		delete pGetYXAll;
	}

	wNum = AppCommu::Instance()->GetVirtualYCNum(wVDevID);
	if(wNum) {
		SGetVirtualYCAll* pGetYCAll = new SGetVirtualYCAll;
		pGetYCAll->wVDevID = wVDevID;
		pGetYCAll->wNum = wNum;
		pGetYCAll->byGroup = -1;
		pGetYCAll->pYC = new SVirtualYCSend[wNum];
		AppCommu::Instance()->GetVirtualYCAll(pGetYCAll);
		VirtualYCOut(wFacNo, wLogicalNodeID, wVDevID, pGetYCAll, byRII, 241, COT_GI);
		delete[] pGetYCAll->pYC;
		delete pGetYCAll;
	}
}

void CProtocol::VirtualHandleASDU228(uchar* pData, ushort wLength, ushort wFacNo, ushort wDevID, ushort wVDevID)
{
	if(wLength < 15) return;
	if(pData[2] != 20 || pData[7] != 1) return;
	AppCommu::Instance()->RVYKCheck(wFacNo, wDevID, wVDevID, (SYKCheckDataID* )&pData[8]);
}

void CProtocol::IVYKCheckBack(ushort wFacID, ushort wDevID, SYKCheckDataBack* pYKCheckBack)
{
	uchar* pSndBuf = new uchar[200];
	pSndBuf[0] = 229;
	pSndBuf[1] = 0x81;
	if(pYKCheckBack->bIsSuccess) pSndBuf[2] = 0x14;
	else pSndBuf[2] = 0x15; 
	pSndBuf[3] = 0xff;
	pSndBuf[4] = 0xfe;
	pSndBuf[5] = 0xf4;
	pSndBuf[6] = 0;
	pSndBuf[7] = 1;
	SYKCheckDataID* pID = (SYKCheckDataID* )&pSndBuf[8];
	pID->wFacAddr = pYKCheckBack->wFacAddr;
	pID->wDevAddr = pYKCheckBack->wDevAddr;
	pID->wGIN = pYKCheckBack->wGIN;
	pID->byOPType = pYKCheckBack->byOPType;
	AppCommu::Instance()->ProtocolVirtualSend(wFacID, wDevID, pYKCheckBack->wVDevAddr, (char* )pSndBuf, 15);
	delete[] pSndBuf;
}

bool CProtocol::JudgIsVirtualYXGroup(uchar byGroup)
{
    Q_UNUSED(byGroup);
/*	SGetVirtualGroup* pVirYXGroup = new SGetVirtualGroup;;
	pVirYXGroup->byGroupType = enumGroupTypeYX;
	pVirYXGroup->byGroupNum = 0;
	AppCommu::Instance()->GetVirtualGroup(pVirYXGroup);
	for(uchar i=0; i<pVirYXGroup->byGroupNum; i++){
		if(byGroup == pVirYXGroup->byGroupVal[i]) {
			delete pVirYXGroup;
			return TRUE;
		}
	}
	delete pVirYXGroup;
*/	return false;
}

bool CProtocol::JudgIsVirtualYKGroup(uchar byGroup)
{
    Q_UNUSED(byGroup);
/*	SGetVirtualGroup* pVirYKGroup = new SGetVirtualGroup;;
	pVirYKGroup->byGroupType = enumGroupTypeYK;
	pVirYKGroup->byGroupNum = 0;
	AppCommu::Instance()->GetVirtualGroup(pVirYKGroup);
	for(uchar i=0; i<pVirYKGroup->byGroupNum; i++){
		if(byGroup == pVirYKGroup->byGroupVal[i]) {
			delete pVirYKGroup;
			return TRUE;
		}
	}
	delete pVirYKGroup;
*/	return false;
}

void CProtocol::VirtualSendYXAll(ushort wFacNo, ushort wLogicalNodeID, uchar byGroup, uchar byRII)
{
    Q_UNUSED(wFacNo);
    Q_UNUSED(wLogicalNodeID);
    Q_UNUSED(byGroup);
    Q_UNUSED(byRII);
/*	uchar byNum = AppCommu::Instance()->GetVirtualGroupYXNum(wVDevAddr, byGroup);
	if(byNum == 0) return;
	SGetVirtualYXAll* pGetYXAll = new SGetVirtualYXAll;
	pGetYXAll->wNum = byNum;
	pGetYXAll->byGroup = byGroup;
	pGetYXAll->pYX = new SVirtualYXSend[byNum];
	AppCommu::Instance()->GetVirtualYXAll(pGetYXAll);
	VirtualYXOut(wFacNo, wLogicalNodeID, pGetYXAll, byRII, 241, COT_GRCR_VALID);
	delete[] pGetYXAll->pYX;
	delete pGetYXAll;
*/
}

void CProtocol::VirtualSendYXSingle(ushort wFacNo, ushort wLogicalNodeID, ushort wGIN, uchar byRII)
{
    Q_UNUSED(wFacNo);
    Q_UNUSED(wLogicalNodeID);
    Q_UNUSED(wGIN);
    Q_UNUSED(byRII);
/*	SVirtualYXSend* pYXSend = new SVirtualYXSend;
	pYXSend->wGIN = wGIN;
	AppCommu::Instance()->GetVirtualYX(pYXSend);
	char* pData = new char[20];
	ASDU_21* pAsdu = (ASDU_21* )pData;
	pAsdu->byType = TYPW_GRC_MARK;
	pAsdu->byType = VSQ_COMMON1;
	pAsdu->byCOT = COT_GRCR_VALID;
	pAsdu->byAddress = 0;
	SGenDescripID* pDescrip = (SGenDescripID* )(pData + 7);
	pDescrip->byFUN = 254;
	pDescrip->byINF = 243;
	pDescrip->byRII = byRII;
	pDescrip->wGIN = wGIN;
	pDescrip->NGD.byCONT = 0;
	pDescrip->NGD.byCOUNT = 0;
	pDescrip->NGD.byNUM = 1;
	SGenDescripElement* pElement = (SGenDescripElement* )&pDescrip->GDE[0];	
	pElement->byKOD = KOD_ACTUALVALUE;
	pElement->GDD.byCONT = 0;
	pElement->GDD.byNUMBER = 1;
	pElement->GDD.bySIZE = 6;
	pElement->GDD.byTYPE = 18;
	if(pYXSend->byValue) pElement->byGID[0] = 2;
	else pElement->byGID[0] = 1;
	pElement->byGID[5] = 0;
	SYSTEMTIME sysTime;
	FileTimeToSystemTime(&pYXSend->timeOccur, &sysTime);
	SShortTime* pShortTime =(SShortTime* )pElement->byGID[1];
	pShortTime->byHour = sysTime.wHour;
	pShortTime->byMinute = sysTime.wMinute;
	pShortTime->wMilliseconds = sysTime.wSecond * 1000 + sysTime.wMilliseconds;
	AppCommu::Instance()->ProtocolVirtualSend(wFacNo, wLogicalNodeID, pData, 20);
	delete[] pData;
	delete pYXSend;
*/
 }

void CProtocol::VirtualSendYXSpecify(ushort wFacNo, ushort wLogicalNodeID, SGenIDRec* pIDRec, uchar byNum, uchar byRII)
{
    Q_UNUSED(wFacNo);
    Q_UNUSED(wLogicalNodeID);
    Q_UNUSED(pIDRec);
    Q_UNUSED(byNum);
    Q_UNUSED(byRII);
/*	if(byNum <= 0) return;
	SGetVirtualYXAll* pGetYXAll = new SGetVirtualYXAll;
	pGetYXAll->wNum = byNum;
	pGetYXAll->pYX = new SVirtualYXSend[byNum];
	for(int i=0; i<byNum; i++){
		SGenIDRec* pIDCur = pIDRec + i;
		SVirtualYXSend* pYXCur = pGetYXAll->pYX + i;
		pYXCur->wGIN = pIDCur->wGIN;
		AppCommu::Instance()->GetVirtualYX(pYXCur);
	}
	VirtualYXOut(wFacNo, wLogicalNodeID, pGetYXAll, byRII, 244, COT_GRCR_VALID);
	delete[] pGetYXAll->pYX;
	delete pGetYXAll;
*/
}

