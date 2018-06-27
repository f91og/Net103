#ifndef _APPCOMMU_H
#define _APPCOMMU_H

#include <QObject>
#include "comm/ProDefine.h"
#include "protocol.h"

class AppCommu	: public QObject
{
    Q_OBJECT
public:
    AppCommu(QObject* parent=0);
	~AppCommu();
public:
	static AppCommu* m_pInstance;
	static AppCommu* Instance();
	static void Exitance() ;
private slots:
    void SlotDeviceChange(ushort sta,ushort dev,bool add);
    void SlotRecvASDU(ushort sta,ushort dev, const QByteArray& data);
    void SlotNetStateChange(ushort sta,ushort dev,uchar net, uchar state);

    void SlotDutyChange(bool v);
public:
	ushort			m_wPulseTimes;
	ushort			m_wQuaryTimes;
	ushort			m_wYKSelTimes;
	ushort			m_wYKRunTimes;
public:
	int				m_dwProtocolTimerID;
    uchar			m_byNodeType;
    bool			m_bIsHostNode;
	QString			m_strWaveDir;
private:
	int				m_nThreadGrp;
public:
	CProtocol*		m_pProtocol;
	LPPROCALLBACK	m_lpProCallBack;
protected:
	void timerEvent( QTimerEvent * e);
public:
	void SetNodeState(bool bIsHostNode);
//OutPut
public:
	bool IStart();
	bool IEnd();
	bool ISetNetInf(char* strIP1, char* strIP2, ushort wFactoryID, ushort wLocalID, 
                        QList<ushort>* pVirtualAddrList, uchar byNodeType, ushort wPort);
    bool ISetCallBack(LPPROCALLBACK lpProCallBack);

	bool IAnalyse103Data(ushort wFacID, ushort wDevID, char* pData, ushort wLen);
	bool IAnalyseInternalData(ushort wFacID, ushort wDevID, char* pData, ushort wLen);
	bool IAnalyseVirtual103(ushort wFacID, ushort wDevID, ushort wVDevID, char* pData, ushort wLen);
	bool INotifyNetDState(ushort wFacID, ushort wDevID, uchar byNetType, uchar byNetState);
	bool IDeviceAddOrDel(ushort wFacID, ushort wDevID, uchar byType, uchar byAddOrDel);

	bool IVirtualSendYX(ushort wVDevNo, ushort wNum, SVirtualYXSend* pYXSend);
	bool IVirtualSendYC(ushort wVDevNo, ushort wNum, SVirtualYCSend* pYCSend);
	bool ISendInternalData(ushort wFacID, ushort wDevID, char* pData, ushort wLen);
	bool IReadGenDataAll(ushort wFacID, ushort wDevID, EPreDefinedGroupType byGroupType,
										EDevReadType byReadType ,uchar byGroupNo);
	bool IReadGenData(ushort wFacID, ushort wDevID, EPreDefinedGroupType byGroupType,
										EDevReadType byReadType, SGINType* pGIN, ushort wReadNum);
	bool IWriteGenDataAll(ushort wFacAddr, ushort wDevAdd, uchar byType, ushort wNum, ushort wLen, char* pDataSnd);
	bool IWriteGenData(ushort wFacID, ushort wDevID, EPreDefinedGroupType byGroupType, 
															ushort wNum, SGENDataItem* paSetting);
	bool IYKYTSend(ushort wFacID, ushort wDevID, SYKYTData* pData, bool bSCYK);
    bool ISendTimeSync(ushort wFacID, QDateTime tFault);
	bool IRelayReset(ushort wFacNo, ushort wDevID);
	bool IJudgDeviceIsOnLine(ushort wFacNo, ushort wDevID);
	void IAskGeneralQuary();

	void IGetDeviceNetState(ushort wFacID, ushort wDevID, uchar& byNetState1, uchar& byNetState2);
    bool IReadHistoryData(ushort wFacID, ushort wDevID, EPreDefinedGroupType byReadType, qint32 tStart, qint32 tEnd);
	void IYKCheck(ushort wFacID, ushort wDevID, SYKCheckDataID* pYKCheckID);
	void IVYKCheckBack(ushort wFacID, ushort wDevID, SYKCheckDataBack* pYKCheckBack);

	bool IAskFile(tagFileAskInf* pFileInf);
	void ISCYKRunCMD(ushort wStaAddr, ushort wDevAddr, ushort wGIN, bool bRet);
	void IDoSftSwitch(ushort wStaAddr, ushort wDevAddr, ushort wGIN, bool bVal, bool bIsOtherYK);

//返回函数
public:
	void RGenDataEnterDB(SGrcDataRec* pGrcRec, EPreDefinedGroupType wGroupType);
    void RSyncTimeEnterDB(QDateTime* pFileTime);
	void RNotifyGenCMD(SGenCMDNotify* pGenNotify);
	void RNotifyHisCMD(SHisCMDNotify* pHisNotify);
	void RNotifyYKYTCMD(SYKYTCMDNotify* pYKYT);
	void RGroupEnterDB(SGroupEnterDB* pGroupEnter);
	void RDeviceAddOrDel(SDeviceAddOrDel* pDeviceAdd);
	void RSCRunInf(SSCCRUN_INF* pSCRunInf);
public:
	bool RNotifyVirtualYK(ushort wVDevID, ushort wGIN, uchar byCMD);
	void RVYKCheck(ushort wFacID, ushort wDevID, ushort wVDevID, SYKCheckDataID* pYKCheckID);
public:
	void RFileAsk(tagFileAskInf* pFileInf);
public:
	void GetFactoryName(QString& strFactoryName, ushort wFacNo);						
	bool DtbTableHandle(SOpDtbTable* pOpDtbTable);
	bool DtbFileEnterDB(SOpDtbFile* pDtbFile);
	void NOFFANTableEnterDB(SNOFFANTable* pNOFFanTable);

	void GetVirtualGroup(SGetVirtualGroup* pGroup);
	ushort GetVirtualYXNum(ushort wVDevID);
	uchar GetVirtualGroupYXNum(ushort wVDevAddr, uchar byGroup);
	void GetVirtualYXAll(SGetVirtualYXAll* pYXAll);
	void GetVirtualYX(SVirtualYXSend* pYX);

	ushort GetVirtualYCNum(ushort wVDevID);
	void GetVirtualYCAll(SGetVirtualYCAll* pYCAll);

	void YKCheckBack(SYKCheckDataBack* pYKCheckBck);

public:
	void ProtocolNetSend(ushort wFacNo, ushort wLogicalNodeId, char* byBuf, ushort wLen);
	void ProtocolNetBroadcast(ushort wFacNo, ushort wLogicalNodeId, char* byBuf, ushort wLen);
	void ProtocolVirtualSend(ushort wFacNo, ushort wLogicalNodeId, ushort wVDevID, char* byBuf, ushort wLen);
	void AddMsgToShowList(tagShowMsg* pShowMsg);
//工具函数
public:
	static ushort CharToShort(uchar* pSrc);
    static quint32 CharToLong(uchar* pSrc);
	static float CharToFloat(uchar* pSrc);
    static void ACETimeToDNTime(QDateTime* pACETime, DNSYSTEMTIME* pSysTime);
	static void IShortTimeToSysTime(SShortTime* pShortTime, DNSYSTEMTIME* pSysTime);
    static bool IShortTimeToFileTime(SShortTime* pShortTime, QDateTime* pFileTime);
    static bool IShortTimeToFileTime(SShortTime* pShortTime, QDateTime* pFileTime, ushort* pMillisecond);
    static bool ILongTimeToFileTime(SLongTime* pLongTime, QDateTime* pFileTime);
    static bool ILongTimeToFileTime(SLongTime* pLongTime, QDateTime* pFileTime, ushort* pMillisecond);
public:
	bool AutoSendWave(ushort wFacID,ushort wProtectID);
	void GetProtectDealFlag(ushort& wDealFlag, ushort wFacID,ushort wProtectID);
	uchar GetGroupByGroupType(ushort wFacAddr, ushort wDevAddr, EPreDefinedGroupType wType, uchar* pGroupValue, uchar byBufLen);
};


/////////////////////////////////////////////////////////////////////////////

#endif // !defined(_APPCOMMU_H)
