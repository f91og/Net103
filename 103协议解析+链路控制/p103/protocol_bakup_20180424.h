#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__
#include "../pnet103/Device.h"
#include "FileTrans.h"

class EDevice;

class CProtocol
{
public:
// Constructors
	CProtocol();
	~CProtocol();
public:
	CFileTrans*	m_pFileTrans;
private:
    QList<CDevice*>			m_plDeviceDstb;
	QString	m_strWaveDirName;
	ushort m_wSyncTimer;
private:
	static uchar				m_byRII;
    QList<CDbCommand*>		m_plDbCommandWait;
    QList<CGrcDataRec*>		m_plGenWaitForHandle;
    QList<CRawData*>			m_plRawWaitForHandle;


    QList<CGenCommand*>		m_plGenCommand;
    QList<CDisturbItem*>		m_plDisturbItem;
    QList<CYKYTItem*>			m_plYKYTItem;
    QList<CHisCommand*>		m_plHisCommand;

//Interface Operation	
public:
	void IAskGeneralQuary();
	void IDeviceAddOrDel(ushort wFacID, ushort wDevID, uchar byType, uchar byAddOrDel);
	void INotifyNetDState(ushort wFacID, ushort wDevID, uchar byNetType, uchar byNetState);
	void IDataUnpack(ushort wFacNo, ushort wDevID, uchar* pData,ushort wLength);

	void IGenWriteAllItem(ushort wFacNo, ushort wLogicalNodeID,
				  EPreDefinedGroupType wGroupType, ushort wNum, ushort wLen, char* pDataSnd);
	void IGenWriteItem(ushort wFacNo, ushort wLogicalNodeID,
				  EPreDefinedGroupType wGroupType,  ushort wNum, SGENDataItem* paSetting, bool bAuto = false);
	void IGenReadItem(ushort wFacNo, ushort wLogicalNodeID,
				EPreDefinedGroupType wGroupType, EDevReadType byReadType, uchar byGroupNo);
	void IGenReadItem(ushort wFacNo, ushort wLogicalNodeID, EPreDefinedGroupType wGroupType, 
							EDevReadType byReadType, SGINType* pGIN, ushort wReadNum);
	void IYKYTSend(ushort wFacNo, ushort wLogicalNodeID, SYKYTData* pData, bool bSCYK, bool bAuto = false);
	bool IDistDataTrans(ushort wFacNo, ushort wLogicalNodeId, CDistTable* pDstTable);
	void IRelayReset(ushort wFacNo, ushort wLogicalNodeID);
	bool IJudgDeviceIsOnLine(ushort wFacNo, ushort wDevID);
    void ISendTimeSync(ushort wFacID, QDateTime tTime);
    void IReadHistoryData(ushort wFacID, ushort wDevID, EPreDefinedGroupType byReadType, qint32 tStart, qint32 tEnd);
	void IYKCheck(ushort wFacID, ushort wDevID, SYKCheckDataID* pYKCheckID);
	void ISCYKRunCMD(ushort wStaAddr, ushort wDevAddr, ushort wGIN, bool bRet);
	void IDoSftSwitch(ushort wStationAddr, ushort wDevAddr, ushort wGIN, bool bVal, bool bIsOtherYK);

    void OnTimer();

//ASDU HANDLE
private:
	void DataAnalysis(ushort wFacNo, ushort wDevID, uchar* pData,ushort wLength);

	void HandleASDU5(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU6(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU10(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU11(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU23(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU26(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU27(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU28(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU29(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU30(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU31(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU229(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);

//Gen Receive Operation
private:
	void RGenReadCatalog(ushort wFacNo, ushort wLogicalNodeID, uchar* pDataCom, ushort wDataLen);
	void RGenRead10Pack(CGenCommand* pCMD, uchar* pDataCom, uchar byCot, ushort wDataLen);
    void RGenRead10Handle(ushort wFacNo, ushort wLogicalNodeID, QList<CGrcReadPack*>* pReadPackList);
	void RGenRead11Pack(CGenCommand* pCMD, uchar* pDataCom, uchar byCot, ushort wDataLen);
    void RGenRead11Handle(ushort wFacNo, ushort wLogicalNodeID, QList<CGrcReadPack*>* pReadPackList);
	void RGenWriteAffirm(CGenCommand* pCMD, uchar* pDataCom);
	void RYKYTHandle(ushort wFacNo, ushort wLogicalNodeID, 
								uchar* pDataCom,ushort wCalLen, uchar byCot);
	void GenDataEntry(ushort wFacNo, ushort wLogicalNodeID, ushort  wNum,
										uchar* pGenData, ushort wLen, uchar byCot = 42);
	void GenDataEntry(ushort wFacNo, ushort wLogicalNodeID,  uchar* pGenData, ushort wCalLen);
	void GenDataHandle(CGrcDataRec* pDataRec, bool bIsFromWait);

//out opertion
private:
	void GenCatalogEnterDB(ushort wFacNo, ushort wLogicalNodeID, ushort wSum,
											uchar* pTotalBuf, ushort wTotalLen);
	void GenDataEnterDB(CGrcDataRec* pDataRec, EPreDefinedGroupType wGroupType);
    void SyncTimeEnterDB(QDateTime* pFileTime);
	void DistTableEnertDB(ushort wFacNo, ushort wLogicalNodeID, uchar byNum, uchar* pTableRec);
    qint32 DistDataEnterDB(ushort wFacNo, ushort wLogicalNodeID, CDisturbItem* pDisturbItem);

//Gen Send Operation
private:
	void SGenReadCatalog(ushort wFacNo, ushort wLogicalNodeID);		   //读取目录

	void SendReadCommand(CGenCommand* pCMD);
	void SGenReadAllItemData(CGenCommand* pCMD);				   //读取所有条目的某个或所有属性	
	void SGenReadItemAllData(CGenCommand* pCMD);				   //读取某个条目的所有属性	
	void SGenReadItemData(CGenCommand* pCMD);					   //读取某些条目的某个属性 

	void SGenWriteCMD(CGenCommand* pCMD);						

	void SendDisturbOver(CDisturbItem* pDisturbItem, uchar byTOO, uchar byACC, uchar byTOV);
	void SendDisturbCmd(CDisturbItem* pDisturbItem, uchar byTOO, uchar byACC, uchar byTOV);
	void SendYKYTCommand(CYKYTItem* pYKItem);
	void SendAsdu10(ushort wFacNo, ushort wLogicalNodeID, uchar byLen, uchar* pData);
	void SendAsdu21(ushort wFacNo, ushort wLogicalNodeID, uchar byLen, uchar* pData);

	void SendGeneralQuary(ushort wFacNo, ushort wLogicalNodeID);
	void SReadHistoryData(CHisCommand* pHisCMD);

//Wave Ask

//定时函数
private:
	void TimerWaitCommand();
	void TimerWaitData();
	void RawDataHandle();

//Tools Operation
public:
	CDevice*	FindDeviceDstb(ushort wFacNo, ushort wDevID);
private:
	void AddCatalogCMDToWaitList(CGenCommand* pCMD);
	void AddCMDToWaitList(CDbCommand* pDbCMD, ushort wWaitTime);
	void AddDataToWaitList(CGrcDataRec* pDataRec);
	void AddRawDataToWaitList(CRawData* pRawData);
	CRawData* RemoveRawDataFromWaitList();

	CGenCommand* FindGenCMD(SCommandID& keyCMD, bool bIsRead);
	CGenCommand* FindGenCMD(ushort wFacAddr, ushort wDevAddr, uchar byRII, bool bIsRead);	//ADD FOR DD
	CGenCommand* GetGenCMD(SCommandID& keyCMD, bool bIsRead);
	void DeleteGenCMD(SCommandID& keyCMD);
	bool AddGenCMD(CGenCommand* pGenCMD);
	void OverJudgGenCMD();
	bool JudgDevIsSendGenCMD(ushort wFacNo, ushort wDevAddr);

	CDisturbItem* FindDtbItem(SDevID& keyDTB);
	CDisturbItem* GetDtbItem(SDevID& keyDTB);
	void DeleteDtbItem(SDevID& keyDTB);
    bool AddDtbItem(CDisturbItem* pDTBItem);
	void OverJudgDtbItem();

	CYKYTItem* FindYKYTItem(SDevID& keyYKYT);
	CYKYTItem* GetYKYTItem(SDevID& keyYKYT);
	void DeleteYKYTItem(SDevID& keyYKYT);
	bool AddYKYTItem(CYKYTItem* pNewItem);
	void OverJudgYKYTItem();

	CHisCommand* FindHisCMD(ushort wFacAddr, ushort wDevAddr, uchar byRII);
	CHisCommand* GetHisCMD(SCommandID& keyCMD);
	bool AddHisCMD(CHisCommand* pHisCMD);
	void DeleteHisCMD(SCommandID& keyCMD);
	void OverJudgHisCMD();

	CDevice*	GetDeviceDstb(ushort wFacNo, ushort wDevID, uchar byType);
	void		DeleteDeviceDstb(ushort wFacNo, ushort wDevID);

    qint32 PutToWriteBuf(CGenWriteBuf* pWriteBuf, ushort wNum, ushort wLen, char* pSetting);
    qint32 PutToWriteBuf(CGenWriteBuf* pWriteBuf, ushort wNum, SGENDataItem* paSetting);
    qint32 PutToReadBuf(CGenReadBuf* pReadBuf, SGINType* pGIN, ushort wReadNum);
    qint32 PutToReadBuf(CGenCommand* pCMD, uchar byReadType, uchar byGroupNo);
    void SetReadInf(ushort wCMDType, uchar byReadType, quint32 & dwwReadInf);
	uchar ReadTypeToKOD(uchar byReadType);
    uchar GetReadKOD(quint32& dwReadInf, bool bDel);
	ushort CalcuGrcDataLen(uchar* pgd, ushort wCount);
	ushort CalcuGrcIDLen(uchar* pgi, ushort wCount);
    bool CompareData(uchar* d1, uchar* d2, quint32 len);
    qint32 JudgGroupType(const QString& strName);

	void NotifyGenCMD(CGenCommand* pCMD, bool bIsReadType, bool bIsSuccess);
	void NotifyHisCMD(CHisCommand* pCMD, bool bIsSuccess);
	void NotifyDisturbCMD(CDisturbItem* pDtbItem, bool bIsTableEnter);
    void NotifyYKYTCMD(CYKYTItem* pYKYTItem, bool bIsSuccess, const char* strCause = NULL);
	void NotifyDbCommand(CDbCommand* pDbCMD, bool bIsSuccess);
	void SendNetState(ushort wFacID, ushort wDevID, bool bIsNet2, uchar byNetState);

public:
	void LoadAllDeviceDtbTable();
//
public:
	uchar GetGroupByGroupType(SCommandID& keyCMD, uchar* byGroupValue, uchar byBufLen);
	uchar GetGroupByGroupType(SCommandID& keyCMD, uchar byGroupNo);
	uchar GetGroupTypeByGroupVal(ushort wFacNo, ushort wLogicalNodeID, uchar byGroupValue);

	void GetDeviceNetState(ushort wFacID, ushort wDevID, uchar& byNetState1, uchar& byNetState2);

//Virtual Tool
public:
	SVirtualYKYTData*	m_pCurYKInf;
public:
	bool IVirtualAutoSendYX(ushort wFacNo, ushort wNum, SVirtualYXSend* pYXSend);
	void IVirtualDataUnpack(ushort wFacNo, ushort wDevID, ushort wVDevID, uchar* pData,ushort wLength);
	void IVYKCheckBack(ushort wFacID, ushort wDevID, SYKCheckDataBack* pYKCheckBack);
	bool IVirtualAutoSendYC(ushort wVDevNo, ushort  wNum, SVirtualYCSend* pYCSend);

private:
	void VirtualHandleASDU10(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID, ushort wVDevID);
	void VirtualHandleASDU21(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID, ushort wVDevID);
	void VirtualHandleASDU228(uchar* pData, ushort wLength, ushort wFacNo, ushort wDevID, ushort wVDevID);

private:
	void VirtualSendCatalog(ushort wFacNo, ushort wLogicalNodeID, ushort wVDevID, uchar byRII);
	void VirtualSendYXAll(ushort wFacNo, ushort wLogicalNodeID, uchar byGroup, uchar byRII);
	void VirtualSendYXSingle(ushort wFacNo, ushort wLogicalNodeID, ushort wGIN, uchar byRII);
	void VirtualSendYXSpecify(ushort wFacNo, ushort wLogicalNodeID, SGenIDRec* pIDRec, uchar byNum, uchar byRII);
	void VirtualGenQuery(ushort wFacNo, ushort wLogicalNodeID, ushort wVDevID, uchar byRII);

	void MakeYCSend(char* pCurRec, SVirtualYCSend* pCurYC);
	void VirtualYCOut(ushort wFacNo, ushort wLogicalNodeID, ushort wVDevID, 
								 SGetVirtualYCAll* pYCAll, uchar byRII, uchar byInf, uchar byCOT);

private:
	bool JudgIsVirtualYKGroup(uchar byGroup);
	bool JudgIsVirtualYXGroup(uchar byGroup);

public:
	void MakeYXSend(char* pCurRec, SVirtualYXSend* pCurYX);
	void VirtualYXOut(ushort wFacNo, ushort wLogicalNodeID, ushort wVDevID, SGetVirtualYXAll* pYXAll, uchar byRII, uchar byInf, uchar byCOT);

public:
    QTextCodec* m_gbkCodec;
};

#endif // __Protocol_H__

