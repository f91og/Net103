#include "appcommu.h"
#include "protocol.h"
#include "protocol_define.h"
#include <qmutex.h>
#include <qfile.h>
#include <qdatetime.h>

QMutex	g_mutexProtocolHisCMD;

bool CFileTrans::AskFile(tagFileAskInf* pFileInf)
{
	CDevice* pDevice = m_pProtCentre->FindDeviceDstb(pFileInf->wStationAddr, pFileInf->wDevAddr);
	if(!pDevice) return false;
	tagFileAskState* pAsk = AddFileAskList(pFileInf);
	if(!pAsk) return false;
	pAsk->byAskStep = WAVEFILE_STATE_FILESEL;
	pAsk->wTimer = 0;
	SendWaveFileAsk(pFileInf->wStationAddr, pFileInf->wDevAddr, pFileInf->wFileName, 
									0xff, WAVEFILE_STATE_FILESEL, pFileInf->byFileType);
	return true;
}

tagFileAskState* CFileTrans::AddFileAskList(tagFileAskInf* pFileInf)
{
    foreach(tagFileAskState* pFileAsk,m_plFileAsk) {
		if(pFileAsk->pFileInf->wStationAddr == pFileInf->wStationAddr 
			&& pFileAsk->pFileInf->wDevAddr == pFileInf->wDevAddr
				&& pFileAsk->pFileInf->byFileType == pFileInf->byFileType) return NULL;
    }

	tagFileAskState* pNew = new tagFileAskState;
	pNew->pFileInf = new tagFileAskInf;
    memcpy(pNew->pFileInf, pFileInf, sizeof(tagFileAskInf));
	m_plFileAsk.append(pNew);
	return pNew;
}

tagFileAskState* CFileTrans::FindFileAskInf(ushort wStationID, ushort wDeviceID, uchar byFileType)
{

    foreach(tagFileAskState* pFileAsk , m_plFileAsk) {
		if(pFileAsk->pFileInf->wStationAddr == wStationID && 
				pFileAsk->pFileInf->wDevAddr == wDeviceID	&& 
				pFileAsk->pFileInf->byFileType == byFileType) return pFileAsk;
	}
	return NULL;
}

void CFileTrans::SendWaveFileAsk(ushort wStaID, ushort wDevID, ushort wFileName, 
										uchar bySectionName, uchar byCMDType, uchar byFileType)
{
	char byBuf[200];
	byBuf[0] = (char) 0xDD;
	byBuf[1] = (char) 0x81;
	byBuf[2] = (char) 0x82;
	byBuf[3] = (char) 0xff;
	byBuf[4] = (char) 0xff;
	byBuf[5] = byFileType;
	byBuf[6] = wFileName % 256;
	byBuf[7] = wFileName / 256;
	byBuf[8] = bySectionName;
	byBuf[9] = byCMDType;

	AppCommu::Instance()->ProtocolNetSend(wStaID, wDevID, byBuf, 10);
}

void CFileTrans::SendWaveFileOK(ushort wStaID, ushort wDevID, ushort wFileName, 
										uchar bySectionName, uchar byAFQ, uchar byFileType)
{
	char byBuf[200];
	byBuf[0] = (char) 0xE3;
	byBuf[1] = (char) 0x81;
	byBuf[2] = (char) 0x82;
	byBuf[3] = (char) 0xff;
	byBuf[4] = (char) 0xff;
	byBuf[5] = byFileType;
	byBuf[6] = wFileName % 256;
	byBuf[7] = wFileName / 256;
	byBuf[8] = bySectionName;
	byBuf[9] = byAFQ;
	AppCommu::Instance()->ProtocolNetSend(wStaID, wDevID, byBuf, 10);
}

void CFileTrans::EndFileTrans(tagFileAskState* pFileAsk, bool bOK)
{
	pFileAsk->pFileInf->bOK = bOK;
	AppCommu::Instance()->RFileAsk(pFileAsk->pFileInf);
    int pos=0;
    while(pos<m_plFileAsk.count()) {
        tagFileAskState* pFileInf = m_plFileAsk.at(pos);
		if(pFileInf == pFileAsk) {
            while(!pFileInf->plSegList.isEmpty()) delete pFileInf->plSegList.takeFirst();
			delete pFileInf->pFileInf;
			delete pFileInf;
            m_plFileAsk.removeAt(pos);
			break;
		}
        pos++;
	}
}

void CFileTrans::OnTimer()
{
    int pos=0;
    while(pos<m_plFileAsk.count()) {
        tagFileAskState* pFileInf = m_plFileAsk.at(pos);
		pFileInf->wTimer++;
		if(pFileInf->wTimer >= OVERTIMER_WAVEFILETRANS) {
			pFileInf->pFileInf->bOK = false;
			AppCommu::Instance()->RFileAsk(pFileInf->pFileInf);
            while(!pFileInf->plSegList.isEmpty()) delete pFileInf->plSegList.takeFirst();
			delete pFileInf->pFileInf;
			delete pFileInf;
            m_plFileAsk.removeAt(pos);
            continue;
		}
        pos++;
	}
}

void CFileTrans::SaveFile(tagFileAskState* pFileInf)
{
	QFile fTmp(pFileInf->pFileInf->strFullName);
    if ( !fTmp.open( QIODevice::WriteOnly)) return;

    foreach(CWaveSegData* pSegData ,pFileInf->plSegList) {
        fTmp.write((const char* )pSegData->GetData(), pSegData->GetLength());
	}
	fTmp.close();
}


void CFileTrans::HandleASDU222(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
	uchar byFileType = pData[5];
	if(byFileType == FILETYPE_WAVE) {
		if(!AppCommu::Instance()->AutoSendWave(wFacNo,wLogicalNodeID)) return;
		int nNum = pData[1] & 0x7f;
		if(wLength < nNum * 14 + 6) return;
		CDevice* pDevice = m_pProtCentre->FindDeviceDstb(wFacNo, wLogicalNodeID);
		if(!pDevice) return;
		uchar* pHead = pData + 6;
		for(int i=0; i<nNum; i++){
			SWaveTable* pWaveTable = new SWaveTable;
			pWaveTable->wNOF = AppCommu::Instance()->CharToShort(pHead);
			pHead += 2;
			pWaveTable->dwFileTotalLen = AppCommu::Instance()->CharToLong(pHead);
			pHead += 4;
			pWaveTable->bySOF = *pHead;
			pHead += 1;
			SLongTime LongTime;
			LongTime.wMilliseconds = AppCommu::Instance()->CharToShort(pHead);
			LongTime.byMinute = pHead[2] & 0x3f;
			LongTime.byRes1 = (pHead[2] & 0x40) >> 6;
			LongTime.byIV = (pHead[2] & 0x80) >> 7;
			LongTime.byHour = (pHead[3] & 0x1f);
			LongTime.byRes2 = (pHead[3] & 0x60) >> 5;
			LongTime.bySummer = (pHead[3] & 0x80) >> 7;
			LongTime.byDay = (pHead[4] & 0x1f);
			LongTime.byDayOfWeek = (pHead[4] & 0xe0) >> 5;
			LongTime.byMonth = (pHead[5] & 0x0f);
			LongTime.byRes3 = (pHead[5] & 0xf0) >> 4;
			LongTime.byYear = (pHead[6] & 0x7f);
			LongTime.byRes4 = (pHead[6] & 0x80) >> 7;
			if(!AppCommu::Instance()->ILongTimeToFileTime(&LongTime, &pWaveTable->timeFault)) {
				delete pWaveTable;
				continue;
			}
			pDevice->AddWaveTable(pWaveTable);
		}
	}
}

void CFileTrans::HandleASDU223(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
	uchar byFileType = pData[5];
	if(wLength < 13) return;
	ushort wFileName = AppCommu::Instance()->CharToShort(pData + 6);
    quint32 dwFileLength = AppCommu::Instance()->CharToLong(pData + 8);

	bool bReady = true;
	if(pData[12] & 0x80) bReady = false;
	if(byFileType == FILETYPE_WAVE) {
		CDevice* pDevice = m_pProtCentre->FindDeviceDstb(wFacNo, wLogicalNodeID);
		if(pDevice) pDevice->WaveFileReady(wFileName, dwFileLength, bReady);
	}
	else {
		tagFileAskState* pFileInf = FindFileAskInf(wFacNo, wLogicalNodeID, byFileType);
		if(!pFileInf) return;
		if(pFileInf->pFileInf->wFileName != wFileName || pFileInf->byAskStep != WAVEFILE_STATE_FILESEL) return;
		pFileInf->lFileLen = dwFileLength;
		pFileInf->wTimer = 0;
		if(!bReady) EndFileTrans(pFileInf, false);
		else {
			pFileInf->byAskStep = WAVEFILE_STATE_FILEASK;
			SendWaveFileAsk(pFileInf->pFileInf->wStationAddr, pFileInf->pFileInf->wDevAddr, pFileInf->pFileInf->wFileName, 
											0xff, WAVEFILE_CMD_FILEASK, pFileInf->pFileInf->byFileType);
		}
	}
}

void CFileTrans::HandleASDU224(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
	if(wLength < 14) return;
	uchar byFileType = pData[5];
	bool bReady = true;
	if(pData[13] & 0x80) bReady = false;
	ushort wFileName = AppCommu::Instance()->CharToShort(pData + 6);
	uchar bySecName = pData[8];
    quint32 dwSectionLength = AppCommu::Instance()->CharToLong(pData + 9);

	if(byFileType == FILETYPE_WAVE) {
		CDevice* pDevice = m_pProtCentre->FindDeviceDstb(wFacNo, wLogicalNodeID);
		if(pDevice) pDevice->WaveSectionReady(wFileName, bySecName, dwSectionLength, bReady);
	}
	else {
		tagFileAskState* pFileInf = FindFileAskInf(wFacNo, wLogicalNodeID, byFileType);
		if(!pFileInf) return;
		if(pFileInf->pFileInf->wFileName != wFileName || 
				(pFileInf->byAskStep != WAVEFILE_STATE_FILEASK && 
				pFileInf->byAskStep != WAVEFILE_STATE_SECTIONEND)) return;
		if(!bReady) EndFileTrans(pFileInf, false);
		else {
			pFileInf->wTimer = 0;
			pFileInf->byCurSecName = bySecName;
			pFileInf->lCurSecLen = dwSectionLength;
			pFileInf->byAskStep = WAVEFILE_STATE_SECTIONASK;
			SendWaveFileAsk(pFileInf->pFileInf->wStationAddr, pFileInf->pFileInf->wDevAddr, pFileInf->pFileInf->wFileName, 
											bySecName, WAVEFILE_CMD_SECTIONASK, pFileInf->pFileInf->byFileType);
		}
	}
}

void CFileTrans::HandleASDU225(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
	if(wLength < 14) return;
	ushort wFileName = AppCommu::Instance()->CharToShort(pData + 6);
	uchar bySectionName = pData[8];
	ushort wSegLength = AppCommu::Instance()->CharToShort(pData + 9);
	if(wLength < 11 + wSegLength) return;

	uchar byFileType = pData[5];
	if(byFileType == FILETYPE_WAVE) {
		CDevice* pDevice = m_pProtCentre->FindDeviceDstb(wFacNo, wLogicalNodeID);
		if(pDevice) pDevice->WaveSegTrans(wFileName, bySectionName, wSegLength, (pData + 11));
	}
	else {
		tagFileAskState* pFileInf = FindFileAskInf(wFacNo, wLogicalNodeID, byFileType);
		if(!pFileInf) return;
		if(pFileInf->pFileInf->wFileName != wFileName || pFileInf->byAskStep != WAVEFILE_STATE_SECTIONASK) return;
		if(pFileInf->byCurSecName != bySectionName) return;
		pFileInf->wTimer = 0;
		pFileInf->lCurSecLen -= wSegLength;
		pFileInf->lFileLen -= wSegLength;
		if(pFileInf->lCurSecLen < 0 || pFileInf->lFileLen < 0) EndFileTrans(pFileInf, false);
		CWaveSegData* pSegData = new CWaveSegData;
		pSegData->SetData(pData + 11, wSegLength);
		pFileInf->plSegList.append(pSegData);
	}
}

void CFileTrans::HandleASDU226(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID)
{
	if(wLength < 9) return;
	ushort wFileName = AppCommu::Instance()->CharToShort(pData + 6);
	bool bIsSeg = false;
	if(pData[8] == 3 || pData[8] == 4) bIsSeg = true;
	uchar byFileType = pData[5];
	if(byFileType == FILETYPE_WAVE) {
		CDevice* pDevice = m_pProtCentre->FindDeviceDstb(wFacNo, wLogicalNodeID);
		if(pDevice) pDevice->WaveLastSegOrSection(wFileName, bIsSeg);
	}
	else {
		tagFileAskState* pFileInf = FindFileAskInf(wFacNo, wLogicalNodeID, byFileType);
		if(!pFileInf) return;
		if(pFileInf->pFileInf->wFileName != wFileName) return;
		pFileInf->wTimer = 0;
		if(bIsSeg){
			if(pFileInf->byAskStep != WAVEFILE_STATE_SECTIONASK) return;
			if(pFileInf->lCurSecLen != 0){
				SendWaveFileOK(wFacNo, wLogicalNodeID, pFileInf->pFileInf->wFileName, 
										pFileInf->byCurSecName, WAVEFILE_CONFIRM_SEC_NO, pFileInf->pFileInf->byFileType);
				EndFileTrans(pFileInf, false);
				return;
			}
			SendWaveFileOK(wFacNo, wLogicalNodeID, pFileInf->pFileInf->wFileName, 
										pFileInf->byCurSecName, WAVEFILE_CONFIRM_SEC_OK, pFileInf->pFileInf->byFileType);
			pFileInf->byAskStep = WAVEFILE_STATE_SECTIONEND;
		}
		else {
			if(pFileInf->byAskStep != WAVEFILE_STATE_SECTIONEND) return;
			if(pFileInf->lFileLen < 0) {
				SendWaveFileOK(wFacNo, wLogicalNodeID, pFileInf->pFileInf->wFileName, 
									0xff, WAVEFILE_CONFIRM_FILE_NO, pFileInf->pFileInf->byFileType);
				EndFileTrans(pFileInf, false);
				return;
			}
			else {
				SendWaveFileOK(wFacNo, wLogicalNodeID, pFileInf->pFileInf->wFileName, 
									0xff, WAVEFILE_CONFIRM_FILE_OK, pFileInf->pFileInf->byFileType);
				SaveFile(pFileInf);
				EndFileTrans(pFileInf, true);
			}
		}
	}
}

//装置历史报告查询
CHisCommand* CProtocol::FindHisCMD(ushort wFacAddr, ushort wDevAddr, uchar byRII)
{
	g_mutexProtocolHisCMD.lock();

    foreach(CHisCommand* pCMD , m_plHisCommand) {
		if(pCMD->m_keyCMD.wFacNo == wFacAddr && pCMD->m_keyCMD.wLogicalNodeId == wDevAddr && pCMD->m_byOldRII == byRII){
			g_mutexProtocolHisCMD.unlock();
			return pCMD;
		}
    }
	g_mutexProtocolHisCMD.unlock();
	return NULL;
}
//若存在则返回空指针，若不存在则创建并返回。
CHisCommand* CProtocol::GetHisCMD(SCommandID& keyCMD)
{
	g_mutexProtocolHisCMD.lock();

    foreach(CHisCommand* pCMD ,m_plHisCommand) {
		if(pCMD->m_keyCMD.wFacNo == keyCMD.wFacNo && pCMD->m_keyCMD.wLogicalNodeId == keyCMD.wLogicalNodeId){
			g_mutexProtocolHisCMD.unlock();
			return NULL;
		}
    }
	CHisCommand* pNewCMD = new CHisCommand(keyCMD);
	m_plHisCommand.append(pNewCMD);
    if(m_plHisCommand.count() > MAXLISTITEM) delete m_plHisCommand.takeFirst();
	g_mutexProtocolHisCMD.unlock();
	return pNewCMD;
}

bool CProtocol::AddHisCMD(CHisCommand* pHisCMD)
{
	g_mutexProtocolHisCMD.lock();

    foreach(CHisCommand* pCMD , m_plHisCommand) {
		if(pCMD->m_keyCMD.wFacNo == pHisCMD->m_keyCMD.wFacNo && pCMD->m_keyCMD.wLogicalNodeId == pHisCMD->m_keyCMD.wLogicalNodeId){
			g_mutexProtocolHisCMD.unlock();
			return false;
		}
	}
	m_plHisCommand.append(pHisCMD);
	if(m_plHisCommand.count() > MAXLISTITEM) 
        delete m_plHisCommand.takeFirst();
	g_mutexProtocolHisCMD.unlock();
	return true;
}

void CProtocol::DeleteHisCMD(SCommandID& keyCMD)
{
	g_mutexProtocolHisCMD.lock();
    int pos=0;
    while(pos<m_plHisCommand.count()) {
        CHisCommand* pCMD = m_plHisCommand.at(pos);
		if(pCMD->m_keyCMD == keyCMD){
            m_plHisCommand.removeAt(pos);
			delete pCMD;
			break;
		}
        pos++;
	}
	g_mutexProtocolHisCMD.unlock();
}

void CProtocol::OverJudgHisCMD()
{
	g_mutexProtocolHisCMD.lock();
    int pos=0;
    while(pos<m_plHisCommand.count()) {
        CHisCommand* pCMD = m_plHisCommand.at(pos);
		if(pCMD->m_dwCount++ > OVERTIMER_HISCMD) {
			NotifyHisCMD(pCMD, false);
            m_plHisCommand.removeAt(pos);
			delete pCMD;
            continue;
		}
        pos++;
	}
	g_mutexProtocolHisCMD.unlock();
}


void CProtocol::IReadHistoryData(ushort wFacID, ushort wDevID, EPreDefinedGroupType byReadType, qint32 tStart, qint32 tEnd)
{
	if(tStart < 0 || tEnd < 0) return;
	SCommandID keyCMD;
	keyCMD.wFacNo = wFacID;
	keyCMD.wLogicalNodeId = wDevID;
	keyCMD.wGroupType = byReadType;
	CHisCommand* pHisCMD = GetHisCMD(keyCMD);
	if(!pHisCMD){
		CHisCommand* pCMD = new CHisCommand(keyCMD);
		pCMD->m_tStart = tStart;
		pCMD->m_tEnd = tEnd;
		AddCMDToWaitList(pCMD, OVERTIMER_DBHISWAITCMD);
		return;
	}
	pHisCMD->m_tStart = tStart;
	pHisCMD->m_tEnd= tEnd;
	SReadHistoryData(pHisCMD);
}

void CProtocol::SReadHistoryData(CHisCommand* pHisCMD)
{
	char	byBuf[200];
	byBuf[0] = (char) 0xDC;
	byBuf[1] = (char) 0x81;
	byBuf[2] = (char) COTC_HIS;
	byBuf[3] = (char) 0xff;
	byBuf[4] = (char) 0xff;
	byBuf[5] = (char) 0;
	byBuf[6] = pHisCMD->m_keyCMD.wGroupType;
	QDateTime tmTmp;
	tmTmp.setTime_t(pHisCMD->m_tStart);
	QDate dTmp = tmTmp.date();
	QTime tTmp = tmTmp.time();
	ushort wMSec = tTmp.second() * 1000;
	byBuf[7] = wMSec % 256;
	byBuf[8] = wMSec / 256;
	byBuf[9] = tTmp.minute();
	byBuf[10] = tTmp.hour();
	byBuf[11] = dTmp.day() + dTmp.dayOfWeek() * 0x20;
	byBuf[12] = dTmp.month();
	byBuf[13] = dTmp.year();
	
	tmTmp.setTime_t(pHisCMD->m_tEnd);
	dTmp = tmTmp.date();
	tTmp = tmTmp.time();
	wMSec = tTmp.second() * 1000;
	byBuf[14] = wMSec % 256;
	byBuf[15] = wMSec / 256;
	byBuf[16] = tTmp.minute();
	byBuf[17] = tTmp.hour();
	byBuf[18] = dTmp.day() + dTmp.dayOfWeek() * 0x20;
	byBuf[19] = dTmp.month();
	byBuf[20] = dTmp.year();

	byBuf[21] = m_byRII++;
	pHisCMD->m_byOldRII = byBuf[21];
	AppCommu::Instance()->ProtocolNetSend(pHisCMD->m_keyCMD.wFacNo, pHisCMD->m_keyCMD.wLogicalNodeId, byBuf, 22);
}

