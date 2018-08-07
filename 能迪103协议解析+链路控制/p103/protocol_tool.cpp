#include "appcommu.h"
#include "protocol.h"
#include <qmutex.h>
#include <qdatetime.h>
#include <qfile.h>
#include <QtDebug>
#include <QTextCodec>

#define	MAXSCOVERTIME		20

QMutex	g_mutexProtocolCMD;
QMutex	g_mutexProtocolDtbItem;
QMutex	g_mutexProtocolYKYT;

/*扰动数据所用类结构定义*/
void CDistNOFFile::ClearDistFile()
{
    while(m_plDisturbFileList.count()>0){
        delete m_plDisturbFileList.takeFirst();
    }
}

void CDisturbFile::Initialize()
{
	QDateTime dtTmp = QDateTime::currentDateTime();
	m_dwFirstTimeHigh = dtTmp.toTime_t();
	m_dwFirstTimeLow = 0;

    while(m_plTagsGroupList.count()>0){
        delete m_plTagsGroupList.takeFirst();
    }

    while(m_plChannelInfoList.count()>0){
        delete m_plChannelInfoList.takeFirst();
    }
}
	
CDisturbFile::~CDisturbFile()
{
    while(m_plTagsGroupList.count()>0){
        delete m_plTagsGroupList.takeFirst();
    }

    while(m_plChannelInfoList.count()>0){
        delete m_plChannelInfoList.takeFirst();
    }
}

CChannelInfo* CDisturbFile::FindChannelInfo(uchar byACC)
{
    foreach(CChannelInfo* pChannelInfo,m_plChannelInfoList) {
		if(pChannelInfo->m_byACC==byACC) return pChannelInfo;
	}
	return NULL;
}

CTagsGroup::~CTagsGroup()
{
    while(m_aTagsList.count()>0){
        delete m_aTagsList.takeFirst();
    }
}

CChannelInfo::~CChannelInfo()
{
	m_aChannelData.resize(0);
}

//通用分类函数
CGenBuf::CGenBuf()
{
	m_pGrcSendDataBuf = NULL;

}
CGenBuf::~CGenBuf()
{
	Clear();
}

void CGenBuf::Clear()
{
	if(m_pGrcSendDataBuf) {
		delete[] m_pGrcSendDataBuf;
		m_pGrcSendDataBuf = NULL;
	}
}

CGenReadBuf::CGenReadBuf() : CGenBuf()
{
	m_pGroupValue = NULL;
}
CGenReadBuf::~CGenReadBuf()
{
	Clear();
}

void CGenReadBuf::Clear()
{
	ClearPackList();
	if(m_pGroupValue) {
		delete[] m_pGroupValue;
		m_pGroupValue = NULL;
	}
	CGenBuf::Clear();
}

void CGenReadBuf::ClearPackList()
{
    while(m_plGrcReadPackList.count()>0){
        delete m_plGrcReadPackList.takeFirst();
    }
}

CGenWriteBuf::CGenWriteBuf() : CGenBuf()
{
	m_pGrcWriteBuf = NULL;
}

void CGenWriteBuf::Clear()
{
	if(m_pGrcWriteBuf){
		delete[] m_pGrcWriteBuf;
		m_pGrcWriteBuf = NULL;
	}
	CGenBuf::Clear();
}

CGenWriteBuf::~CGenWriteBuf()
{
	Clear();
}

void CWaveSegData::SetData(uchar* pBuf, ushort wLen)
{
	if(m_pData) delete[] m_pData;
	m_pData = NULL;
	m_wLen = 0;
	if(pBuf && wLen ){
		m_pData = new uchar[wLen];
		memcpy(m_pData, pBuf, wLen);
		m_wLen = wLen;
	}
}

CWaveSection::CWaveSection(uchar bySectionName, quint32 dwSectionLen)
{
	m_bySectionName = bySectionName;
	m_dwSectionLen = dwSectionLen;
	m_wAskTimes = 0;
}

CWaveSection::~CWaveSection()
{
	Clear();
}

void CWaveSection::Clear()
{
    while(m_plWaveSegData.count()>0){
        delete m_plWaveSegData.takeFirst();
    }
}

quint32 CWaveSection::GetLength()
{
    quint32 dwLen = 0;
    foreach(CWaveSegData* pSegData,m_plWaveSegData) {
		dwLen += pSegData->GetLength();
	}
	return dwLen;
}

bool CWaveSection::JudgWaveSectionIsOK()
{
    quint32 dwLen = GetLength();
	if(dwLen != m_dwSectionLen) return false;
	return true ;
}

void CWaveSection::AddWaveSeg(uchar* pBuf, ushort wLen)
{
	CWaveSegData* pSegData = new CWaveSegData;
	pSegData->SetData(pBuf, wLen);
	m_plWaveSegData.append(pSegData);
}

CWaveFile::CWaveFile(SWaveTable* pTable)
{
	m_pTable = pTable;
	m_pCurWaveSection = NULL;
	m_wWaveAskTimer = 0;
}

CWaveFile::~CWaveFile()
{
	if(m_pTable) delete m_pTable;
	if(m_pCurWaveSection) delete m_pCurWaveSection;
    while(m_plWaveSection.count()>0){
        delete m_plWaveSection.takeFirst();
    }
}

bool CWaveFile::JudgFileIsOk()
{
	if(!m_pTable) return false;
    quint32 dwLen=0;
    foreach(CWaveSection* pSection,m_plWaveSection) {
		dwLen += pSection->GetLength();
	}
	if(dwLen == m_pTable->dwFileTotalLen) return true;
	return false;
}

void CWaveFile::ResetTimer()
{
	m_wWaveAskTimer = 0;
}

bool CWaveFile::OnTimer()
{
	m_wWaveAskTimer++;
	if(m_wWaveAskTimer > OVERTIMER_WAVEFILETRANS) return false;
	return true;
}

void CWaveFile::SaveFile(QString strDir, ushort wFacAddr, ushort wDevAddr)
{
	if(!m_pTable) return;
	QString strFile;
    strFile.sprintf("%s%02u_%03u_%05u",strDir.toLocal8Bit().data(), wFacAddr, wDevAddr, m_pTable->wNOF);
	QString strExt, strFileName;
    foreach(CWaveSection* pSection,m_plWaveSection) {
        if(pSection->m_bySectionName == 0) strExt = ".hdr";
        else if(pSection->m_bySectionName == 1) strExt = ".cfg";
        else if(pSection->m_bySectionName == 2) strExt = ".dat";
		else continue;
		strFileName = strFile + strExt;
		QFile flTmp(strFileName);
        if(!flTmp.open(QIODevice::WriteOnly)) return;
        foreach(CWaveSegData* pSeg,pSection->m_plWaveSegData) {
            flTmp.write((const char* )pSeg->GetData(), pSeg->GetLength());
		}
		flTmp.close();
	}

	SOpDtbFile* pDtbFile = new SOpDtbFile;
    sprintf(pDtbFile->strPath, "%s", strDir.toLocal8Bit().data());
	sprintf(pDtbFile->strFileName, "%02u_%03u_%05u", wFacAddr, wDevAddr, m_pTable->wNOF);
    pDtbFile->timeStart =m_pTable->timeFault;
	AppCommu::Instance()->DtbFileEnterDB(pDtbFile);
	delete pDtbFile;
}


bool CProtocol::JudgDevIsSendGenCMD(ushort wFacNo, ushort wDevAddr)
{
	g_mutexProtocolCMD.lock();
    foreach(CGenCommand* pCMD,m_plGenCommand) {
		if(pCMD->m_keyCMD.wFacNo == wFacNo && pCMD->m_keyCMD.wLogicalNodeId == wDevAddr) {
			g_mutexProtocolCMD.unlock();
			return true;
		}
	}
	g_mutexProtocolCMD.unlock();
	return false;
}

CGenCommand* CProtocol::FindGenCMD(SCommandID& keyCMD, bool bIsRead)
{
	g_mutexProtocolCMD.lock();
    foreach(CGenCommand* pCMD,m_plGenCommand) {
		if(bIsRead){
			if(pCMD->m_bIsReadType && pCMD->m_keyCMD == keyCMD){
				g_mutexProtocolCMD.unlock();
				return pCMD;
			}
		}
		else {
			if(!pCMD->m_bIsReadType && pCMD->m_keyCMD.wFacNo == keyCMD.wFacNo 
				&& pCMD->m_keyCMD.wLogicalNodeId == keyCMD.wLogicalNodeId ) {
				g_mutexProtocolCMD.unlock();
				return pCMD;
			}
		}
	}
	g_mutexProtocolCMD.unlock();
	return NULL;
}

CGenCommand* CProtocol::FindGenCMD(ushort wFacAddr, ushort wDevAddr, uchar byRII, bool bIsRead)
{
	g_mutexProtocolCMD.lock();
    foreach(CGenCommand* pCMD,m_plGenCommand) {
		if(bIsRead){
			if(pCMD->m_bIsReadType && pCMD->m_keyCMD.wFacNo == wFacAddr
				&& pCMD->m_keyCMD.wLogicalNodeId == wDevAddr && pCMD->m_pGenBuf && pCMD->m_pGenBuf->m_byOldRII == byRII){
				g_mutexProtocolCMD.unlock();
				return pCMD;
			}
		}
		else {
			if(!pCMD->m_bIsReadType && pCMD->m_keyCMD.wFacNo == wFacAddr && pCMD->m_keyCMD.wLogicalNodeId == wDevAddr) {
				g_mutexProtocolCMD.unlock();
				return pCMD;
			}
		}
	}
	g_mutexProtocolCMD.unlock();
	return NULL;
}
//若存在则返回空指针，若不存在则创建并返回。
CGenCommand* CProtocol::GetGenCMD(SCommandID& keyCMD, bool bIsRead)
{
	g_mutexProtocolCMD.lock();
    foreach(CGenCommand* pCMD,m_plGenCommand) {
		if(pCMD->m_keyCMD.wFacNo == keyCMD.wFacNo &&
			pCMD->m_keyCMD.wLogicalNodeId == keyCMD.wLogicalNodeId){
			g_mutexProtocolCMD.unlock();
			return NULL;
		}
	}
	CGenCommand* pNewCMD = new CGenCommand(keyCMD);
	pNewCMD->m_bIsReadType = bIsRead;
	m_plGenCommand.append(pNewCMD);
	if(m_plGenCommand.count() > MAXLISTITEM) 
        delete m_plGenCommand.takeFirst();
	g_mutexProtocolCMD.unlock();
	return pNewCMD;
}

bool CProtocol::AddGenCMD(CGenCommand* pGenCMD)
{
	g_mutexProtocolCMD.lock();
    foreach(CGenCommand* pCMD,m_plGenCommand) {
		if(pCMD->m_keyCMD.wFacNo == pGenCMD->m_keyCMD.wFacNo && 
			pCMD->m_keyCMD.wLogicalNodeId == pGenCMD->m_keyCMD.wLogicalNodeId){
			g_mutexProtocolCMD.unlock();
			return false;
		}
	}
	m_plGenCommand.append(pGenCMD);
	if(m_plGenCommand.count() > MAXLISTITEM) 
        delete m_plGenCommand.takeFirst();
	g_mutexProtocolCMD.unlock();
	return true;
}

void CProtocol::DeleteGenCMD(SCommandID& keyCMD)
{
	g_mutexProtocolCMD.lock();
    int pos=0;
    while(pos<m_plGenCommand.count()) {
        CGenCommand* pCMD = m_plGenCommand.at(pos);
		if(pCMD->m_keyCMD == keyCMD){
            m_plGenCommand.removeAt(pos);
			delete pCMD;
            continue;
		}
        pos++;
	}
	g_mutexProtocolCMD.unlock();
}

void CProtocol::OverJudgGenCMD()
{
	g_mutexProtocolCMD.lock();
    int pos=0;
    while(pos<m_plGenCommand.count()) {
        CGenCommand* pCMD = m_plGenCommand.at(pos);
        if(pCMD->m_dwCount++ > OVERTIMER_GENCMD) {
            if(pCMD->m_keyCMD.wGroupType != enumGroupTypeCatalog &&
                    pCMD->m_keyCMD.wGroupType != enumGroupTypeYM)
                NotifyGenCMD(pCMD, pCMD->m_bIsReadType, false);
            m_plGenCommand.removeAt(pos);
            delete pCMD;
            continue;
        }
        pos++;
    }
	g_mutexProtocolCMD.unlock();
}



CDisturbItem* CProtocol::FindDtbItem(SDevID& keyDTB)
{
	g_mutexProtocolDtbItem.lock();
    foreach(CDisturbItem* pDtbItem,m_plDisturbItem) {
		if(pDtbItem->m_keyDTB == keyDTB) {
			g_mutexProtocolDtbItem.unlock();
			return pDtbItem;
		}
	}
	g_mutexProtocolDtbItem.unlock();
	return NULL;
}

CDisturbItem* CProtocol::GetDtbItem(SDevID& keyDTB)
{
	g_mutexProtocolDtbItem.lock();
    foreach(CDisturbItem* pDtbItem,m_plDisturbItem) {
		if(pDtbItem->m_keyDTB == keyDTB) {
			g_mutexProtocolDtbItem.unlock();
			return NULL;
		}
	}
	CDisturbItem* pNewDtbItem = new CDisturbItem(keyDTB);
	m_plDisturbItem.append(pNewDtbItem);
	if(m_plDisturbItem.count() > MAXLISTITEM) 
        delete m_plDisturbItem.takeFirst();
	g_mutexProtocolDtbItem.unlock();
	return pNewDtbItem;
}

bool CProtocol::AddDtbItem(CDisturbItem* pDTBItem)
{
	g_mutexProtocolDtbItem.lock();
    foreach(CDisturbItem* pDtbItem,m_plDisturbItem) {
		if(pDtbItem->m_keyDTB == pDTBItem->m_keyDTB){
			g_mutexProtocolDtbItem.unlock();
			return false;
		}
	}
	m_plDisturbItem.append(pDTBItem);
	if(m_plDisturbItem.count() > MAXLISTITEM) 
        delete m_plDisturbItem.takeFirst();
	g_mutexProtocolDtbItem.unlock();
	return true;
}

void CProtocol::DeleteDtbItem(SDevID& keyDTB)
{
	g_mutexProtocolDtbItem.lock();
    int pos=0;
    while(pos<m_plDisturbItem.count()) {
        CDisturbItem* pDtbItem = m_plDisturbItem.at(pos);
		if(pDtbItem->m_keyDTB == keyDTB){
            m_plDisturbItem.removeAt(pos);
			delete pDtbItem;
            continue;
		}
        pos++;
	}
	g_mutexProtocolDtbItem.unlock();
}


void CProtocol::OverJudgDtbItem()
{
	g_mutexProtocolDtbItem.lock();
    int pos=0;
    while(pos<m_plDisturbItem.count()) {
        CDisturbItem* pDtbItem = m_plDisturbItem.at(pos);
		if(pDtbItem->m_dwCount++ > OVERTIMER_DTBCMD){
			NotifyDisturbCMD(pDtbItem, false);
            m_plDisturbItem.removeAt(pos);
			delete pDtbItem;
            continue;
		}
        pos++;
	}
	g_mutexProtocolDtbItem.unlock();
}

CYKYTItem* CProtocol::FindYKYTItem(SDevID& keyYKYT)
{
	g_mutexProtocolYKYT.lock();
    foreach(CYKYTItem* pYKYTItem,m_plYKYTItem) {
		if(pYKYTItem->m_keyYKYT == keyYKYT) {
			g_mutexProtocolYKYT.unlock();
			return pYKYTItem;
		}
	}
	g_mutexProtocolYKYT.unlock();
	return NULL;
}

CYKYTItem* CProtocol::GetYKYTItem(SDevID& keyYKYT)
{
	g_mutexProtocolYKYT.lock();
    foreach(CYKYTItem* pYKYTItem,m_plYKYTItem) {
		if(pYKYTItem->m_keyYKYT== keyYKYT) {
			g_mutexProtocolYKYT.unlock();
			return NULL;
		}
	}
	CYKYTItem* pNewYKYTItem = new CYKYTItem(keyYKYT);
	m_plYKYTItem.append(pNewYKYTItem);
	if(m_plYKYTItem.count() > MAXLISTITEM) 
        delete m_plYKYTItem.takeFirst();
	g_mutexProtocolYKYT.unlock();
	return pNewYKYTItem;
}

bool CProtocol::AddYKYTItem(CYKYTItem* pNewItem)
{
	g_mutexProtocolYKYT.lock();
    foreach(CYKYTItem* pYKYTItem,m_plYKYTItem) {
		if(pYKYTItem->m_keyYKYT== pNewItem->m_keyYKYT) {
			g_mutexProtocolYKYT.unlock();
			return false;
		}
	}
	m_plYKYTItem.append(pNewItem);
	if(m_plYKYTItem.count() > MAXLISTITEM) 
        delete m_plYKYTItem.takeFirst();
	g_mutexProtocolYKYT.unlock();
	return true;
}

void CProtocol::DeleteYKYTItem(SDevID& keyYKYT)
{
	g_mutexProtocolYKYT.lock();
    int pos=0;
    while(pos<m_plYKYTItem.count()) {
        CYKYTItem* pYKYTItem = m_plYKYTItem.at(pos);
		if(pYKYTItem->m_keyYKYT == keyYKYT){
            m_plYKYTItem.removeAt(pos);
			delete pYKYTItem;
			break;
		}
        pos++;
	}
	g_mutexProtocolYKYT.unlock();
}


void CProtocol::OverJudgYKYTItem()
{
	ushort wOverTimer;
	g_mutexProtocolYKYT.lock();
    int pos=0;
    while(pos<m_plYKYTItem.count()) {
        CYKYTItem* pYKYTItem = m_plYKYTItem.at(pos);
		pYKYTItem->m_dwCount++;
		if(pYKYTItem->m_YKData.byExecType == enumExecTypeExecute) {
			if(pYKYTItem->m_bSCYK) wOverTimer = AppCommu::Instance()->m_wYKRunTimes * MAXSCOVERTIME;
			else wOverTimer = AppCommu::Instance()->m_wYKRunTimes;
		}
		else wOverTimer = AppCommu::Instance()->m_wYKSelTimes;
		if(pYKYTItem->m_dwCount > wOverTimer){
			if(pYKYTItem->m_YKData.byExecType == enumExecTypeExecute)
				NotifyYKYTCMD(pYKYTItem, false, "遥控执行超时");
			else if(pYKYTItem->m_YKData.byExecType == enumExecTypeSelect)
				NotifyYKYTCMD(pYKYTItem, false, "遥控选择超时");
            m_plYKYTItem.removeAt(pos);
			delete pYKYTItem;
            continue;
		}
        pos++;
	}
	g_mutexProtocolYKYT.unlock();
}

CDevice* CProtocol::GetDeviceDstb(ushort wFacNo, ushort wDevID, uchar byType)
{
    foreach(CDevice* pDstDevice,m_plDeviceDstb) {
		if(pDstDevice->m_wFacNo == wFacNo && pDstDevice->m_wDevID == wDevID) {
			pDstDevice->m_byDevType = byType;
			return pDstDevice;
		}
    }
    CDevice* pDstDevice = new CDevice(wFacNo, wDevID, byType, m_strWaveDirName, this);
	m_plDeviceDstb.append(pDstDevice);
	return pDstDevice;
}

CDevice* CProtocol::FindDeviceDstb(ushort wFacNo, ushort wDevID)
{
    foreach(CDevice* pDstDevice,m_plDeviceDstb) {
		if(pDstDevice->m_wFacNo == wFacNo && pDstDevice->m_wDevID == wDevID) return pDstDevice;
    }
	return NULL;
}

void CProtocol::DeleteDeviceDstb(ushort wFacNo, ushort wDevID)
{
    int pos=0;
    while(pos<m_plDeviceDstb.count()) {
        CDevice* pDstDevice = m_plDeviceDstb.at(pos);
		if(pDstDevice->m_wFacNo == wFacNo && pDstDevice->m_wDevID == wDevID){
            m_plDeviceDstb.removeAt(pos);
			delete pDstDevice;
			return;
		}
        pos++;
	}
}

void CProtocol::SetReadInf(ushort wGroupType, uchar byReadType, quint32& dwReadInf)
{
	dwReadInf = 0;
	if(byReadType == enumDevReadDescription) dwReadInf |= enumReadKODDescription;
	else if(byReadType == enumDevReadPrecision) dwReadInf |= enumReadKODPrecision;
	else if(byReadType == enumDevReadDimension) dwReadInf |= enumReadKODDimension;
	else if(byReadType == enumDevReadFactor) dwReadInf |= enumReadKODFactor;
	else if(byReadType == enumDevReadReference) dwReadInf |= enumReadKODReference;
	else if(byReadType == enumDevReadDefault) dwReadInf |= enumReadKODDefault;
	else if(byReadType == enumDevReadRange) dwReadInf |= enumReadKODRange;
	else if(byReadType == enumDevReadValue) dwReadInf |= enumReadKODValue;
	else if(byReadType == enumDevReadEnum) dwReadInf |= enumReadKODEnum;
	else if(byReadType == enumDevReadPass) dwReadInf |= enumReadKODPass;
	else if(byReadType == enumDevReadFunInf) dwReadInf |= enumReadKODFunInf;
	else if(byReadType == enumDevReadEvent) dwReadInf |= enumReadKODEvent;
	else if(byReadType == enumDevReadTextArray) dwReadInf |= enumReadKODTextArray;
	else if(byReadType == enumDevReadValueArray) dwReadInf |= enumReadKODValueArray;
	else if(byReadType == enumDevReadEntries) dwReadInf |= enumReadKODEntries;
	else if(byReadType == enumDevReadAll) {
		switch(wGroupType){
		case enumGroupTypeDevDescription:
		case enumGroupTypeSoftSwitch:
		case enumGroupTypeHardSwitch:
		case enumGroupTypeActElement:
		case enumGroupTypeDevCheckSelf:
		case enumGroupTypeRunWarn:
		case enumGroupTypeYX:
		case enumGroupTypeYC:
		case enumGroupTypeYM:
		case enumGroupTypeYK:
		case enumGroupTypeYT:
		case enumGroupTypeTapPos:
		case enumGroupTypeOpInf:
		case enumGroupTypeRelayJD:
		case enumGroupTypeSCOPInf:
			dwReadInf = dwReadInf | enumReadKODDescription | enumReadKODValue;
			break;
		case enumGroupTypeDevParam:
		case enumGroupTypeSetting:
			dwReadInf = dwReadInf | enumReadKODDescription | enumReadKODPrecision
				| enumReadKODRange| enumReadKODDimension | enumReadKODValue;
			break;
		case enumGroupTypeRelayMeasure:
			dwReadInf = dwReadInf | enumReadKODDescription | enumReadKODPrecision
										| enumReadKODDimension | enumReadKODValue;
			break;
		case enumGroupTypeFaultInf:
			dwReadInf = dwReadInf | enumReadKODDescription | enumReadKODPrecision
										| enumReadKODDimension;
			break;
		case enumGroupTypeSettingArea:
		case enumGroupTypeSetInf:
			dwReadInf = dwReadInf | enumReadKODDescription | enumReadKODRange | enumReadKODValue;
			break;
		case enumGroupTypeDtbInf:
			dwReadInf = dwReadInf | enumReadKODDescription | enumReadKODDimension | enumReadKODValue;
			break;
		default:
			break;
		}
	}
}

uchar CProtocol::GetReadKOD(quint32& dwReadInf, bool bDel)
{
	if(dwReadInf & enumReadKODDescription){
		if(bDel) dwReadInf &= ~enumReadKODDescription;
		return KOD_DESCRIPTION;
	}
	else if(dwReadInf & enumReadKODPrecision){
		if(bDel) dwReadInf &= ~enumReadKODPrecision;
		return KOD_PRECISION;
	}
	else if(dwReadInf & enumReadKODDimension){
		if(bDel) dwReadInf &= ~enumReadKODDimension;
		return KOD_DIMENSION;
	}
	else if(dwReadInf & enumReadKODFactor){
		if(bDel) dwReadInf &= ~enumReadKODFactor;
		return KOD_FACTOR;
	}
	else if(dwReadInf & enumReadKODReference){
		if(bDel) dwReadInf &= ~enumReadKODReference;
		return KOD_REFERENCE;
	}
	else if(dwReadInf & enumReadKODDefault){
		if(bDel) dwReadInf &= ~enumReadKODDefault;
		return KOD_DEFAUTVALUE;
	}
	else if(dwReadInf & enumReadKODRange){
		if(bDel) dwReadInf &= ~enumReadKODRange;
		return KOD_RANGE;
	}
	else if(dwReadInf & enumReadKODValue){
		if(bDel) dwReadInf &= ~enumReadKODValue;
		return KOD_ACTUALVALUE;
	}
	else if(dwReadInf & enumReadKODEnum){
		if(bDel) dwReadInf &= ~enumReadKODEnum;
		return KOD_ENUMERATION;
	}
	else if(dwReadInf & enumReadKODPass){
		if(bDel) dwReadInf &= ~enumReadKODPass;
		return KOD_PASSWORD;
	}
	else if(dwReadInf & enumReadKODFunInf){
		if(bDel) dwReadInf &= ~enumReadKODFunInf;
		return KOD_FUNANDINF;
	}
	else if(dwReadInf & enumReadKODEvent){
		if(bDel) dwReadInf &= ~enumReadKODEvent;
		return KOD_EVENTS;
	}
	else if(dwReadInf & enumReadKODTextArray){
		if(bDel) dwReadInf &= ~enumReadKODTextArray;
		return KOD_TEXTARRAY;
	}
	else if(dwReadInf & enumReadKODValueArray){
		if(bDel) dwReadInf &= ~enumReadKODValueArray;
		return KOD_VALUEARRAY;
	}
	else if(dwReadInf & enumReadKODEntries){
		if(bDel) dwReadInf &= ~enumReadKODEntries;
		return KOD_ENTRIES;
	}
	return RETERROR;
}

uchar CProtocol::ReadTypeToKOD(uchar byReadType)
{
	switch(byReadType){
    case enumDevReadDescription:
		return KOD_DESCRIPTION;
	case enumDevReadPrecision:
		return KOD_PRECISION;
	case enumDevReadDimension:
		return KOD_DIMENSION;
	case enumDevReadFactor:
		return KOD_FACTOR;
	case enumDevReadReference:
		return KOD_REFERENCE;
	case enumDevReadDefault:
		return KOD_DEFAUTVALUE;
	case enumDevReadRange:
		return KOD_RANGE;
	case enumDevReadValue:
		return KOD_ACTUALVALUE;
	case enumDevReadEnum:
		return KOD_ENUMERATION;
    case enumDevReadPass:
		return KOD_PASSWORD;
	case enumDevReadFunInf:
		return KOD_FUNANDINF;
	case enumDevReadEvent:
		return KOD_EVENTS;
    case enumDevReadTextArray:
		return KOD_TEXTARRAY;
    case enumDevReadValueArray:
		return KOD_VALUEARRAY;
    case enumDevReadEntries:
		return KOD_ENTRIES;
	}
	return RETERROR;
}

ushort CProtocol::CalcuGrcDataLen(uchar* pgd, ushort wCount)
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

ushort CProtocol::CalcuGrcIDLen(uchar* pgi, ushort wCount)
{
	if(wCount<6) return 0;

	uchar* pHead = (uchar* )pgi;
	pHead += 6;
	ushort wLen = 6;
	uchar byNum = pgi[GENDESID_NGD] & 0x3f;
	if(byNum>0){
        for(quint32 i=0; i<byNum; i++)
		{
			if(ushort(wLen + GENDESELE_SIZE) > wCount){
				pgi[GENDESID_NGD] = 0;
				wLen = 6;
				break;
			}
            //SGenDescripElement *pgde = (SGenDescripElement *)pHead;
			if(pHead[GENDESELE_GDD] ==2){	// 成组位串
				wLen += 4+((pHead[GENDESELE_GDD + GDDTYPE_SIZ]+7)/8)*pHead[GENDESELE_GDD + GDDTYPE_NUM];
				pHead += 4+((pHead[GENDESELE_GDD + GDDTYPE_SIZ]+7)/8)*pHead[GENDESELE_GDD + GDDTYPE_NUM];
			} 
			else {
				wLen += 4+pHead[GENDESELE_GDD + GDDTYPE_SIZ]*pHead[GENDESELE_GDD + GDDTYPE_NUM];
				pHead += 4+pHead[GENDESELE_GDD + GDDTYPE_SIZ]*pHead[GENDESELE_GDD + GDDTYPE_NUM];
			}
			if(wLen>wCount) {
				pHead[GENDESELE_GDD + GDDTYPE_NUM] = 0;
				wLen = 6;
				break;
			}
		}
	}
	return wLen;
}

bool CProtocol::CompareData(uchar* d1, uchar* d2, quint32 len)
{
	if(len<=0) return false;
    for(quint32 i=0; i<len; i++)
		if(d1[i] != d2[i]) return false;
	return true;
}

void CProtocol::NotifyDbCommand(CDbCommand* pDbCMD, bool bIsSuccess)
{
	if(pDbCMD->m_byDBCMDType == DBCMD_GEN){
		CGenCommand* pGenCmd = (CGenCommand* )pDbCMD;
		NotifyGenCMD(pGenCmd, pGenCmd->m_bIsReadType, bIsSuccess);
	}
	else if(pDbCMD->m_byDBCMDType == DBCMD_HIS){
		CHisCommand* pHisCmd = (CHisCommand* )pDbCMD;
		NotifyHisCMD(pHisCmd, bIsSuccess);
	}
	else if(pDbCMD->m_byDBCMDType == DBCMD_DTB){
		CDisturbItem* pDtbItem = (CDisturbItem* )pDbCMD;
		NotifyDisturbCMD(pDtbItem, bIsSuccess);
	}
	else if(pDbCMD->m_byDBCMDType == DBCMD_YKYT){
		CYKYTItem* pYKYTItem = (CYKYTItem* )pDbCMD;
		NotifyYKYTCMD(pYKYTItem, bIsSuccess, "命令等待超时");
	}
}

void CProtocol::NotifyGenCMD(CGenCommand* pCMD, bool bIsReadType, bool bIsSuccess)
{
	if(pCMD->m_bAuto) return;
	if(pCMD->m_keyCMD.wGroupType == enumGroupTypeDtbInf){
		CDevice* pDevice = FindDeviceDstb(pCMD->m_keyCMD.wFacNo, pCMD->m_keyCMD.wLogicalNodeId);
		if(pDevice && bIsSuccess) pDevice->m_byGetChannelNameTimes = 0;
		return;
	}
	else if(pCMD->m_keyCMD.wGroupType == enumGroupTypeFaultInf){
		CDevice* pDevice = FindDeviceDstb(pCMD->m_keyCMD.wFacNo, pCMD->m_keyCMD.wLogicalNodeId);
		if(pDevice && bIsSuccess) pDevice->m_byGetFaultInfTimes = 0;
	}
	if(pCMD->m_keyCMD.wGroupType == enumGroupTypeCatalog) return;

	SGenCMDNotify* pGenNotify = new SGenCMDNotify;
	pGenNotify->wFacID = pCMD->m_keyCMD.wFacNo;
	pGenNotify->wDevID = pCMD->m_keyCMD.wLogicalNodeId;
	pGenNotify->byGroupType = (uchar )pCMD->m_keyCMD.wGroupType;
	pGenNotify->bIsRead = bIsReadType;
	pGenNotify->bIsSuccess = bIsSuccess;
	AppCommu::Instance()->RNotifyGenCMD(pGenNotify);
	delete pGenNotify;
}

void CProtocol::NotifyHisCMD(CHisCommand* pCMD, bool bIsSuccess)
{
	SHisCMDNotify* pHisNotify = new SHisCMDNotify;
	pHisNotify->wFacID = pCMD->m_keyCMD.wFacNo;
	pHisNotify->wDevID = pCMD->m_keyCMD.wLogicalNodeId;
	pHisNotify->byGroupType = (uchar )pCMD->m_keyCMD.wGroupType;
	pHisNotify->bIsSuccess = bIsSuccess;
	AppCommu::Instance()->RNotifyHisCMD(pHisNotify);
	delete pHisNotify;
}

void CProtocol::NotifyDisturbCMD(CDisturbItem* pDtbItem, bool bIsTableEnter)
{
	CDevice*	pDeviceDst = FindDeviceDstb(pDtbItem->m_keyDTB.wFacNo, pDtbItem->m_keyDTB.wLogicalNodeId);
	if(pDeviceDst == NULL) return;
	pDeviceDst->m_bIsTransDTB = false;
	if(pDtbItem->m_pDtbTable && bIsTableEnter) {
		pDeviceDst->AddDistTable(pDtbItem->m_pDtbTable);
		pDtbItem->m_pDtbTable = NULL;
	}
}

void CProtocol::NotifyYKYTCMD(CYKYTItem* pYKYTItem, bool bIsSuccess, const char* strCause)
{
	if(pYKYTItem->m_bAuto) return;
	SYKYTCMDNotify* pYKYT = new SYKYTCMDNotify;
	pYKYT->wFacID = pYKYTItem->m_keyYKYT.wFacNo;
	pYKYT->wDevID = pYKYTItem->m_keyYKYT.wLogicalNodeId;
	pYKYT->byExecType = pYKYTItem->m_YKData.byExecType;
	pYKYT->byCMDType = pYKYTItem->m_YKData.byCommandType;
	pYKYT->byOpType = pYKYTItem->m_YKData.byOpType;
	pYKYT->wGIN = pYKYTItem->m_YKData.wGIN;
	pYKYT->bIsSuccess = bIsSuccess;
    if(!bIsSuccess && strCause) strcpy(pYKYT->strCause, strCause);
	AppCommu::Instance()->RNotifyYKYTCMD(pYKYT);
	delete pYKYT;
}

//void CProtocol::GenCatalogEnterDB(ushort wFacNo, ushort wLogicalNodeID, ushort wSum,
//											SGenDataRec* pTotalBuf, ushort wTotalLen)
void CProtocol::GenCatalogEnterDB(ushort wFacNo, ushort wLogicalNodeID, ushort wSum,
											uchar* pTotalBuf, ushort wTotalLen)
{
	CDevice* pDevice = FindDeviceDstb(wFacNo, wLogicalNodeID);
	if(pDevice == NULL) return;
    //qDebug()<<"装置:"<<pDevice->m_wDevID;
	YByteArray	GroupArray[MAXGROUPTYPE];
	ushort wCalLen = 0;
	uchar* pHead = (uchar* )pTotalBuf;
	for(int i=0; i<wSum; i++){
		ushort wLen;
		if(pHead[GENDATAREC_GDD] == 2)
			wLen = 6 + ((pHead[GENDATAREC_GDD + GDDTYPE_SIZ]+7)/8) * pHead[GENDATAREC_GDD + GDDTYPE_NUM];
		else wLen = 6 + pHead[GENDATAREC_GDD + GDDTYPE_SIZ] * pHead[GENDATAREC_GDD + GDDTYPE_NUM];
		uchar* pData = pHead;
		pHead += wLen;
		wCalLen += wLen;
		if(wCalLen > wTotalLen) break;
		if(pData[GENDATAREC_KOD] != KOD_DESCRIPTION || pData[GENDATAREC_GDD] != 1) continue;
		ushort wDataSize = pData[GENDATAREC_GDD + GDDTYPE_SIZ] * pData[GENDATAREC_GDD + GDDTYPE_NUM];
		uchar* byVal = new uchar[wDataSize+1];
        memcpy(byVal, &pData[GENDATAREC_GID], wDataSize);
		byVal[wDataSize] = 0;
        QString strName = m_gbkCodec->toUnicode( (char* )byVal) ;
        //qDebug()<<"名字："<<strName;
        qint32 lRet = JudgGroupType(strName);
        //qDebug()<<"序号"<<lRet;
		if(lRet <= 0 || lRet > MAXGROUPTYPE) {
			delete[] byVal;
			continue;
		}
		uchar byGroup = pData[GENDATAREC_GIN];
		uint nSize = GroupArray[lRet].size();
		GroupArray[lRet].resize(nSize + 1);
        GroupArray[lRet][nSize] = byGroup;
		delete[] byVal;
	}
	SGroupEnterDB GroupEnter;
	GroupEnter.GroupItems[0].byGroups = 0;
	for(uchar byType=1; byType<MAXGROUPTYPE-1; byType++){
		int nSum = GroupArray[byType].size();
		if(nSum <= 0) {
			pDevice->m_paGroup[byType].resize(0);
			GroupEnter.GroupItems[byType].byGroups = 0;
			continue;
		}
		if(nSum > MAXGROUPITEM) nSum = MAXGROUPITEM;
		pDevice->m_paGroup[byType].resize(nSum);
		GroupEnter.GroupItems[byType].byGroups = nSum;
		for(int n=0; n < nSum; n++){
			uchar byGroupNo = GroupArray[byType][n];
			pDevice->m_paGroup[byType][n] = byGroupNo;
			GroupEnter.GroupItems[byType].byGroupValue[n] = byGroupNo;
		}
	}
	GroupEnter.GroupItems[MAXGROUPTYPE-1].byGroups = 1;
	GroupEnter.GroupItems[MAXGROUPTYPE-1].byGroupValue[0] = 0xff;
	GroupEnter.wFacAddr = pDevice->m_wFacNo;
	GroupEnter.wDevAddr = pDevice->m_wDevID;
	AppCommu::Instance()->RGroupEnterDB(&GroupEnter);
}
/*SUBQ6710：组号判别时，包含定值（除定值区号外）的均为定值，
	包含测量的均为保护测量量。
*/
/*SUBQ9160：组号判别时，动作元件等改为包含匹配。
*/
qint32 CProtocol::JudgGroupType(const QString& strName)
{
	if(strName == "装置描述") return enumGroupTypeDevDescription;
	else if(strName == "装置参数") return enumGroupTypeDevParam;
	else if(strName == "定值区号") return enumGroupTypeSettingArea;
    else if(strName.indexOf("定值") != -1) return enumGroupTypeSetting;
    else if(strName.indexOf("动作元件") != -1) return enumGroupTypeActElement;
    else if(strName.indexOf("装置自检") != -1) return enumGroupTypeDevCheckSelf;
    else if(strName.indexOf("运行告警") != -1)return enumGroupTypeRunWarn;
    else if(strName.indexOf("软压板") != -1) return enumGroupTypeSoftSwitch;
    else if(strName.indexOf("硬压板") != -1) return enumGroupTypeHardSwitch;
    else if(strName.indexOf("遥信") != -1) return enumGroupTypeYX;
    else if(strName.indexOf("测量") != -1) return enumGroupTypeRelayMeasure;
    else if(strName.indexOf("遥测") != -1)
		return enumGroupTypeYC;
	else if(strName == "遥脉") return enumGroupTypeYM;
    else if(strName.indexOf("遥控") != -1) return enumGroupTypeYK;
	else if(strName == "遥调") return enumGroupTypeYT;
	else if(strName == "档位") return enumGroupTypeTapPos;
	else if(strName == "故障信息") return enumGroupTypeFaultInf;
    else if(strName.indexOf("扰动数据说明") != -1) return enumGroupTypeDtbInf;
	else if(strName == "设置信息") return enumGroupTypeSetInf;
	else if(strName == "操作信息") return enumGroupTypeOpInf;
	else if(strName == "顺控操作信息") 
		return enumGroupTypeSCOPInf;
	else if(strName == "接地选线数据") return enumGroupTypeRelayJD;
	else if(strName == "接地试跳") return enumGroupTypeRelayJDST;
	else return RETERROR;
}

uchar CProtocol::GetGroupByGroupType(SCommandID& keyCMD, uchar* byGroupValue, uchar byBufLen)
{
	CDevice* pDevice = FindDeviceDstb(keyCMD.wFacNo, keyCMD.wLogicalNodeId);
	if(pDevice == NULL) return 0;

	YByteArray* pGroup = &pDevice->m_paGroup[keyCMD.wGroupType];
	uchar byNum = pGroup->size();
	if(!byNum) return 0;

	if(byNum > byBufLen) byNum = byBufLen;
	for(uchar i=0; i<byNum; i++)
		byGroupValue[i] = pGroup->at(i);
	return byNum;
}

uchar CProtocol::GetGroupByGroupType(SCommandID& keyCMD, uchar byGroupNo)
{
	CDevice* pDevice = FindDeviceDstb(keyCMD.wFacNo, keyCMD.wLogicalNodeId);
	if(pDevice == NULL) return RETERROR;

	YByteArray* pGroup = &pDevice->m_paGroup[keyCMD.wGroupType];
	uchar byNum = pGroup->size();
	if(byGroupNo >= byNum) return RETERROR;

	return pGroup->at(byGroupNo);
}

uchar CProtocol::GetGroupTypeByGroupVal(ushort wFacNo, ushort wLogicalNodeID, uchar byGroupValue)
{
	CDevice* pDevice = FindDeviceDstb(wFacNo, wLogicalNodeID);
	if(pDevice == NULL) return RETERROR;
	if(byGroupValue == 0xff) return enumGroupTypeNetState;

	for(uchar byType=1; byType<MAXGROUPTYPE; byType++){
		for(int i=0; i<pDevice->m_paGroup[byType].size(); i++){
			uchar byGroup = pDevice->m_paGroup[byType][i];
			if(byGroup == byGroupValue) return byType;
		}
	}
	return RETERROR;
}

//void CProtocol::DistTableEnertDB(ushort wFacNo, ushort wLogicalNodeID, uchar byNum, SDistTableRec* pTableRec)
void CProtocol::DistTableEnertDB(ushort wFacNo, ushort wLogicalNodeID, uchar byNum, uchar* pTableRec)
{
	if(AppCommu::Instance()->m_byNodeType == NODETYPE_DISPATCH || !AppCommu::Instance()->m_bIsHostNode) return;
	CDevice* pDstDevice = FindDeviceDstb(wFacNo, wLogicalNodeID);
	if(byNum == 0 || pDstDevice == NULL || 
		(pDstDevice->m_byDevType != NODETYPE_RELAYDEV && pDstDevice->m_byDevType != NODETYPE_RELAYCKDEV) ) return;

    QList<CDistTable*> paDstTable;
	CDistTable* pDstTable = NULL;
	for(int i=0; i<byNum; i++){ 
		pDstTable = new CDistTable;
		pDstTable->m_byFUN = pTableRec[0];
		pDstTable->m_wFAN = AppCommu::Instance()->CharToShort(&pTableRec[2]);
		pDstTable->m_bySOF = pTableRec[4];
		uchar* pTime = &pTableRec[5];
		SLongTime lTime;
		lTime.byYear = pTime[LONGTIME_YEAR];
		lTime.byMonth = pTime[LONGTIME_MON];
		lTime.byDay = pTime[LONGTIME_DAY];
		lTime.byHour = pTime[LONGTIME_HOUR];
		lTime.byMinute = pTime[LONGTIME_MIN];
		lTime.wMilliseconds = AppCommu::Instance()->CharToShort(&pTime[LONGTIME_MSEC]);
		if(!AppCommu::Instance()->ILongTimeToFileTime(&lTime, &pDstTable->m_timeFault)){
			delete pDstTable;
			continue;
		}
		bool bInsert = false;
        //int nCount = paDstTable.count();
		int nIndex = 0;

        while(nIndex<paDstTable.count()) {
            CDistTable* pDstOld = paDstTable.at(nIndex);
			if(((ushort)(pDstOld->m_wFAN - pDstTable->m_wFAN)) < (ushort)100){
				paDstTable.insert(nIndex, pDstTable);
				bInsert = true;
				break;
			}
			nIndex++;
		}
		if(!bInsert) paDstTable.append(pDstTable);
	}

	CDistTable* pDtbReq = pDstDevice->DistTableEntry(&paDstTable);
    while(paDstTable.count()>0){
        delete paDstTable.takeFirst();
    }
	if(pDtbReq) {
		bool bRet = IDistDataTrans(wFacNo, wLogicalNodeID, pDtbReq);
		if(!bRet) delete pDtbReq;
	}
}

qint32 CProtocol::DistDataEnterDB(ushort wFacNo, ushort wLogicalNodeID, CDisturbItem* pDisturbItem)
{
	if(AppCommu::Instance()->m_byNodeType == NODETYPE_DISPATCH) return RETERROR;
	if(pDisturbItem->m_pDtbTable == NULL) return RETERROR;
	if(pDisturbItem->m_pDtbFile == NULL) {
		NotifyDisturbCMD(pDisturbItem, true);
		return RETERROR;
	}

	CDevice* pDeviceDst = FindDeviceDstb(wFacNo, wLogicalNodeID);
	if(pDeviceDst && (pDeviceDst->m_byDevType == NODETYPE_RELAYDEV 
					|| pDeviceDst->m_byDevType == NODETYPE_RELAYCKDEV)){
		pDeviceDst->DistDataEntry(pDisturbItem->m_pDtbFile, pDisturbItem->m_pDtbTable->m_timeFault);
		NotifyDisturbCMD(pDisturbItem, true);
		pDisturbItem->m_pDtbFile = NULL;
	}
	else NotifyDisturbCMD(pDisturbItem, false);
	return RETOK;
}

void CProtocol::GenDataEnterDB(CGrcDataRec* pDataRec, EPreDefinedGroupType wGroupType)
{
	if(wGroupType == enumGroupTypeDtbInf){
		CDevice* pDevice = FindDeviceDstb(pDataRec->m_pGrcRec->wFacNo, pDataRec->m_pGrcRec->wDevID);
		if(pDevice)	pDevice->EnterDtbInf((uchar* )pDataRec->m_pGrcRec->pGenDataRec, pDataRec->m_pGrcRec->wGenDataLen);
		return;
	}
	else if(wGroupType == enumGroupTypeSCOPInf){
		uchar* pGenData = pDataRec->m_pGrcRec->pGenDataRec;
		uint nType = pGenData[GENDATAREC_GDD];
		uint nSize = pGenData[GENDATAREC_GDD + GDDTYPE_SIZ];
		uint nNum = pGenData[GENDATAREC_GDD + GDDTYPE_NUM];
		if(pGenData[GENDATAREC_KOD] != KOD_ACTUALVALUE || nNum != 1 || nSize != 72 || nType != 23) return;
		uchar* pDataGIN = &pGenData[GENDATAREC_GID];
		if(pDataGIN[GDDTYPE_NUM] != 1 || pDataGIN[GDDTYPE_SIZ] != 2 || pDataGIN[GDDTYPE_TYP] != 3) return;
		ushort wGIN = AppCommu::Instance()->CharToShort(&pDataGIN[3]);
		uchar* pDataType = pDataGIN + 5;
		if(pDataType[GDDTYPE_NUM] != 1 || pDataType[GDDTYPE_SIZ] != 1 || pDataType[GDDTYPE_TYP] != 3) return;
		uchar byType = pDataType[3];
		uchar* pDataVal = pDataType + 4;
		if(pDataVal[GDDTYPE_NUM] != 1 || pDataVal[GDDTYPE_SIZ] != 60 || pDataVal[GDDTYPE_TYP] != 210) return;
		SCCRUN_INF RunInf;
		RunInf.byCMDNo = pDataVal[3];
		RunInf.byReserve = pDataVal[4];
		RunInf.byInfType = pDataVal[5];
		RunInf.byInfValue = pDataVal[6];
// 		memcpy(RunInf.strLDRef, &pDataVal[7], MAXNAME_LD);
        memcpy(RunInf.strFailureCause, &pDataVal[7+MAXNAME_LD], MAX_SCMSGLEN);

		SDevID	keyYKYT;
		keyYKYT.wFacNo = pDataRec->m_pGrcRec->wFacNo;
		keyYKYT.wLogicalNodeId = pDataRec->m_pGrcRec->wDevID;
		CYKYTItem* pYKItem = FindYKYTItem(keyYKYT);
		if(!pYKItem || pYKItem->m_YKData.wGIN != wGIN || pYKItem->m_bSCWaitForChk) return;
		if(byType == SCCINF_FAIL){
			NotifyYKYTCMD(pYKItem, false, RunInf.strFailureCause);
			DeleteYKYTItem(keyYKYT);
		}
		else if(byType == SCCINF_SCCU){
			NotifyYKYTCMD(pYKItem, true, RunInf.strFailureCause);
			DeleteYKYTItem(keyYKYT);
		}
		else {
			SSCCRUN_INF SRunInf;
            memcpy(&SRunInf.RunInf, &RunInf, sizeof(SCCRUN_INF));
			SRunInf.wStaAddr = pDataRec->m_pGrcRec->wFacNo;
			SRunInf.wDevAddr = pDataRec->m_pGrcRec->wDevID;
			SRunInf.wGIN = wGIN;
			pYKItem->m_dwCount = 0;
			AppCommu::Instance()->RSCRunInf(&SRunInf);
		}
		return;
	}
	AppCommu::Instance()->RGenDataEnterDB(pDataRec->m_pGrcRec, wGroupType);
}

void CProtocol::SyncTimeEnterDB(QDateTime* pFileTime)
{
	AppCommu::Instance()->RSyncTimeEnterDB(pFileTime);
}

void CProtocol::LoadAllDeviceDtbTable()
{
    foreach(CDevice* pDevice ,m_plDeviceDstb) {
		pDevice->Load();
	}
}

void CProtocol::GetDeviceNetState(ushort wFacID, ushort wDevID, uchar& byNetState1, uchar& byNetState2)
{
	CDevice* pDevice = FindDeviceDstb(wFacID, wDevID);
	if(pDevice){
		byNetState1 = pDevice->m_byNet1State;
		byNetState2 = pDevice->m_byNet2State;
	}
	else {
		byNetState1 = 0;
		byNetState2 = 0;
	}
}
