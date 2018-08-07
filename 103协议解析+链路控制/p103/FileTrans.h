#ifndef _FILETRANS_H
#define _FILETRANS_H

typedef struct {
	tagFileAskInf* pFileInf;
	uchar byAskStep;
	uchar byCurSecName;
	ushort wTimer;
    qint32 lFileLen;
    qint32 lCurSecLen;
    QList<CWaveSegData*> plSegList;
}tagFileAskState;

class CFileTrans
{
public:
	CFileTrans(CProtocol* pProtCenre){m_pProtCentre = pProtCenre;}
	~CFileTrans(){}
public:
	CProtocol*	m_pProtCentre;
private:
    QList<tagFileAskState*>	m_plFileAsk;
private:
	tagFileAskState* AddFileAskList(tagFileAskInf* pFileInf);
	tagFileAskState* FindFileAskInf(ushort wStationID, ushort wDeviceID, uchar byFileType);
	void EndFileTrans(tagFileAskState* pFileAsk, bool bOK);
	void SaveFile(tagFileAskState* pFileInf);

public:
	bool AskFile(tagFileAskInf* pFileInf);
public:
	void HandleASDU222(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU223(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU224(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU225(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU226(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
	void HandleASDU229(uchar* pData , ushort wLength ,ushort wFacNo, ushort wLogicalNodeID);
public:
	void SendWaveFileAsk(ushort wStaID, ushort wDevID, ushort wFileName, uchar bySectionName, 
									uchar byCMDType, uchar byFileType = FILETYPE_WAVE);
	void SendWaveFileOK(ushort wStaID, ushort wDevID, ushort wFileName, uchar bySectionName, 
									uchar byAFQ, uchar byFileType = FILETYPE_WAVE);
	void OnTimer();

};

#endif
