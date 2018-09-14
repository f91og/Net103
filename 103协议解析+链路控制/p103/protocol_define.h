#ifndef __PROTOCOL_DEFINE_H__
#define __PROTOCOL_DEFINE_H__

#include "comm/ProDefine.h"
//报文类型
#define MSGTYPE_103DATA			0
#define MSGTYPE_HEARTBEAT		1
#define MSGTYPE_INTERNALDATA	2
#define MSGTYPE_REQUESTDATA		0xff

//ASDU 应用类型定义 
//控制方向 
#define TYPC_TIMESYNC			6
#define TYPC_COMMAND			20
#define TYPC_DTB_COMMAND		24
#define TYPC_DTB_CONFIRM		25
#define TYPC_GRC_DATA			10
#define TYPC_GRC_COMMAND		21
#define TYPC_WAVE_AFFIRM		231
#define TYPC_WAVE_TRANSCMD		233
#define TYPC_WAVE_OVER			235
//监视方向
#define TYPW_MARK				5
#define TYPW_GRC_DATA			10
#define TYPW_GRC_MARK			11
#define	TYPW_DTB_TABLE			23
#define TYPW_DTB_DATAREADY		26
#define TYPW_DTB_CHREADY		27
#define TYPW_DTB_STATEREADY		28
#define TYPW_DTB_TRANSSTATE		29
#define TYPW_DTB_TRANSDATA		30
#define TYPW_DTB_END			31
#define TYPW_WAVE_TABLE			230
#define TYPW_WAVE_DATA			232


//可变结构限定词
#define VSQ_COMMON1				0x81
#define VSQ_COMMON2				0x01

//传送原因
//控制方向
#define COTC_TIMESYNC	8	// 时间同步
#define COTC_GENQUERY	9	// 总查询
#define COTC_COMMAND	20	// 一般命令
#define COTC_DTB_DATA	31	// 扰动数据
#define COTC_WAVE_DATA	32	// 波形文件
#define COTC_GRC_WRITE	40	// 通用分类写命令
#define COTC_GRC_READ	42  // 通用分类读命令
#define COTC_HIS		128	// 历史查询

//监视方向
#define COT_SPONTANEOUS	1	// 突发
#define COT_CYCLIC		2	// 循环
#define COT_RESET_FCB	3	// 复位帧计数器
#define COT_RESET_CU	4	// 复位
#define COT_START		5	// 启动	
#define COT_POWER_ON	6	// 上电
#define COT_TEST_MODE	7	// 测试模式	
#define COT_TIMESYNC	8	// 时间同步
#define COT_GI			9	// 总查询
#define COT_TERM_GI		10  // 总查询中止
#define COT_LOCAL_OP	11  // 当地操作
#define COT_REMOTE_OP	12  // 远方操作
#define COT_ACK			20	// 肯定确认
#define COT_NAK			21	// 否定确认
#define COT_DTB_DATA	31	// 扰动数据
#define COT_WAVE_DATA	32	// 波形文件
#define COT_GRCW_ACK_P	40  // 通用写肯定确认
#define COT_GRCW_ACK_N	41  // 通用写否定确认
#define COT_GRCR_VALID	42	// 通用读有效
#define COT_GRCR_INV	43  // 通用读无效
#define COT_GRCW_CFM	44	// 通用写确认
#define COT_HIS			128	// 历史查询
#define COT_HISOVER		129	// 历史查询结束
#define COT_TELECONTROL 200	// 突发


//扰动数据命令TOO
#define TOO_FAULT_SELECT		1
#define TOO_DTB_REQ				2
#define TOO_DTB_STOP			3
#define TOO_CHANNEL_REQ			8
#define TOO_CHANNEL_STOP		9
#define TOO_TAGS_REQ			16
#define TOO_TAGS_STOP			17
#define TOO_DTB_OVER			32
#define TOO_DTB_OVEREX_SYS		33
#define TOO_DTB_OVEREX_DEV		34
#define TOO_CHANNEL_OVER		35
#define TOO_CHANNEL_OVEREX_SYS	36
#define TOO_CHANNEL_OVEREX_DEV	37
#define TOO_TAGS_OVER			38
#define TOO_TAGS_OVEREX_SYS		39
#define TOO_TAGS_OVEREX_DEV		40
#define TOO_WAVE_TRANS_OK		42
#define TOO_WAVE_TRANS_FAIL		43
#define TOO_DTB_TRANS_OK		64
#define TOO_DTB_TRANS_FAIL		65
#define TOO_CHANNEL_TRANS_OK	66
#define TOO_CHANNEL_TRANS_FAIL	67
#define TOO_TAGS_TRANS_OK		68
#define TOO_TAGS_TRANS_FAIL		69

#define SCCINF_FAIL		0
#define SCCINF_SCCU		1
#define SCCINF_PROC		2
//双命令
#define DCO_ON  2
#define DCO_OFF 1

//定时器定义(秒)
#define OVERTIMER_GENCMD		 20
#define OVERTIMER_YKYTCMD		 20
#define OVERTIMER_DTBCMD		 20
#define OVERTIMER_HISCMD		 20

#define OVERTIMER_DBGENWAITCMD	 120
#define OVERTIMER_DBHISWAITCMD	 100
#define OVERTIMER_DBDTBWAITCMD	 120
#define OVERTIMER_DBYKWAITCMD	 30

#define OVERTIMER_GENWAITDATA	 120
#define OVERTIMER_GENERALQUARY	 20

#define OVERTIMER_SYNCTIMER		 30
#define OVERTIMER_DTBFILEENTERDB 30

#define OVERTIMER_WAVEFILETRANS	 15
#define MAXTIMES_WAVEFILETRANS	 3

#define MAX_WAITDATA			1000
#define MAX_RAWDATA				10000
#define MAXLISTITEM				5000
#define MAX_DISTURBTABLE		20

#define DBCMD_GEN	1
#define DBCMD_DTB	2
#define DBCMD_YKYT	3
#define DBCMD_HIS	4

#define READ_ITEMALLDATA	1
#define READ_ALLITEMDATA	2
#define READ_ITEMDATA		3


//显示颜色
#define PRO_TRACE_BLACK			0
#define PRO_TRACE_GRAY			1
#define PRO_TRACE_BLUE			2
#define PRO_TRACE_GREEN			3
#define PRO_TRACE_DARK_RED		4
#define PRO_TRACE_DARK_CYAN		5
#define PRO_TRACE_PURPLE		6
#define PRO_TRACE_DARK_YELLOW	7
#define PRO_TRACE_LIGHT_BLUE	8
#define PRO_TRACE_LIGHT_GREEN	9
#define PRO_TRACE_RED			10
#define PRO_TRACE_CYAN			11
#define PRO_TRACE_PINK			12
#define PRO_TRACE_YELLOW		13
#define PRO_TRACE_DARK_GRAY		14
#define PRO_TRACE_WHITE			15
/*
#define WAVEFILE_TYPE_START		0
#define WAVEFILE_TYPE_HDR		0
#define WAVEFILE_TYPE_CFG		1
#define WAVEFILE_TYPE_DAT		2
#define WAVEFILE_TYPE_END		2
*/
#define WAVEFILE_STATE_CATALOGSEL	0
#define WAVEFILE_STATE_FILESEL		1
#define WAVEFILE_STATE_FILEASK		2
#define WAVEFILE_STATE_SECTIONASK	3
#define WAVEFILE_STATE_SECTIONEND	4
#define WAVEFILE_STATE_FILEEND		5

#define WAVEFILE_CMD_CATALOGSEL		7
#define WAVEFILE_CMD_FILESEL		1
#define WAVEFILE_CMD_FILEASK		2
#define WAVEFILE_CMD_SECTIONASK		5

#define WAVEFILE_CONFIRM_FILE_OK	1	
#define WAVEFILE_CONFIRM_FILE_NO	2	
#define WAVEFILE_CONFIRM_SEC_OK		3	
#define WAVEFILE_CONFIRM_SEC_NO		4	


//ASDU 命令结构
#define ASDU_TYPE	0
#define ASDU_VSQ	1
#define ASDU_COT	2
#define ASDU_ADDR	3
#define ASDU_FUN	4
#define ASDU_INF	5

#define ASDU20_CC	6
#define ASDU20_RII	7

#define ASDU21_RII	6
#define ASDU21_NOG	7

#define ASDU24_TOO	6
#define ASDU24_TOV	7
#define ASDU24_FAN	8
#define ASDU24_ACC	10


//START 通用分类数据结构
//-------------------------扰动数据------------------------


#define DISTR_FUN	0
#define DISTR_RES1	1
#define DISTR_RES2	2
#define DISTR_TOV	3
#define DISTR_FAN	4
#define DISTR_NOF	6
#define DISTR_NOC	8
#define DISTR_NOE	9
#define DISTR_INT	11
#define DISTR_TIM	13


#define DISTC_FUN	0
#define DISTC_RES1	1
#define DISTC_RES2	2
#define DISTC_TOV	3
#define DISTC_FAN	4
#define DISTC_ACC	6
#define DISTC_RPV	7
#define DISTC_RSV	11
#define DISTC_RFA	15


#define DISTT_FUN	0
#define DISTT_RES1	1
#define DISTT_RES2	2
#define DISTT_RES3	3
#define DISTT_FAN	4

#define DISTTA_FUN 0
#define DISTTA_INF 1
#define DISTTA_DPI 2

#define DISTTT_FUN 0
#define DISTTT_RES 1
#define DISTTT_FAN 2
#define DISTTT_NOT 4
#define DISTTT_TAP 5
#define DISTTT_TAG 7


#define DISTCT_FUN	0
#define DISTCT_RES1	1
#define DISTCT_RES2	2
#define DISTCT_TOV	3
#define DISTCT_FAN	4
#define DISTCT_ACC	6
#define DISTCT_NDV	7
#define DISTCT_NFE	8
#define DISTCT_SDV	10

#define DISTO_FUN	0
#define DISTO_RES1	1
#define DISTO_TOO	2
#define DISTO_TOV	3
#define DISTO_FAN	4
#define DISTO_ACC	6


#endif

