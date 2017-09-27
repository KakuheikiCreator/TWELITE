/****************************************************************************
 *
 * MODULE :Framework functions source file
 *
 * CREATED:2015/07/28 05:47:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:基本的な機能と処理フローを提供するフレームワーク
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 ****************************************************************************
 * Copyright (c) 2015, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <jendefs.h>
#include <AppHardwareApi.h>

/****************************************************************************/
/***        ToCoNet Include files                                         ***/
/****************************************************************************/
#include "ToCoNet_use_mod.h"
#include "ToCoNet.h"
#include "ToCoNet_mod_prototype.h"
#include "ccitt8.h"
#include "utils.h"

/****************************************************************************/
/***        User Include files                                            ***/
/****************************************************************************/
// User Include files
#include "framework.h"
#include "config.h"
#include "config_default.h"
#include "timer_util.h"
#include "app_main.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// 構造体：タスク情報（ハードウェアイベント）
typedef struct {
	// 関数ポインタ：Timer 0
	uint8 (*pTimer_0_Task)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：Timer 1
	uint8 (*pTimer_1_Task)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：Timer 2
	uint8 (*pTimer_2_Task)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：Timer 3
	uint8 (*pTimer_3_Task)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：Timer 4
	uint8 (*pTimer_4_Task)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：System controller
	uint8 (*pSysCtrlTask)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：Baseband controller
	uint8 (*pBBCTask)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：Encryption engine
	uint8 (*pAESTask)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：Phy controller
	uint8 (*pPhyCtrlTask)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：UART0
	uint8 (*pUART_0_Task)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：UART1
	uint8 (*pUART_1_Task)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：Serial Interface (2 wire)
	uint8 (*pSITask)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：SPI master
	uint8 (*pSPIMTask)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：SPI slave
	uint8 (*pSPISTask)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：Analogue peripherals
	uint8 (*pAnalogueTask)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：Tick timer
	uint8 (*pTickTimerTask)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：Flash and EEPROM Controller
	uint8 (*pFlashEEPTask)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// 関数ポインタ：Infrared
	uint8 (*pInfraredTask)(uint32 u32DeviceId, uint32 u32ItemBitmap);
} tsHwIntTask;

// 構造体：イベントタスク情報
typedef struct {
	// イベント種別
	teFwkEvent eEvent;
	// 処理タスク関数ポインタ
	void (*vpFunc)(uint32 u32EvtTimeMs);
	// 最長処理時間
	uint32 u32MaxExecTime;
} tsEventTask;

// 構造体：スケジュールイベント
typedef struct stScheduleEvt {
	// イベントインデックス
	uint8 u8EvtIdx;
	// イベント種類
	teFwkEvent eEvent;
	// 実行間隔
	uint32 u32Interval;
	// 次回処理開始時刻
	uint32 u32NextExecMs;
	// 反復実行フラグ
	bool_t bRepeatFlg;
	// 次のイベント情報へのポインタ
	struct stScheduleEvt *spNextEvt;
} tsScheduleEvt;

// 構造体：スケジュールタスク情報リスト
typedef struct {
	// 周期タスク情報（未使用）：先端
	struct stScheduleEvt *spEmptyEvt;
	// 周期タスク情報（使用中）：先端
	struct stScheduleEvt *spBeginEvt;
	// イベント情報配列（動的にメモリ確保が出来ないので配列を使用）
	tsScheduleEvt sEventList[APP_SCHEDULE_EVT_SIZE];
} tsScheduleEvtTaskInfo;

// 構造体：順次実行イベント情報
typedef struct {
	// イベント種類
	teFwkEvent eEvent;
	// イベント発生時刻
	uint32 u32EntryTimeMs;
} tsSequentialEvt;

// 構造体：順次実行イベントタスク情報
typedef struct {
	// リストサイズ
	uint8 u8Size;
	// イベント情報：先頭
	uint8 u8BeginIdx;
	// イベント情報配列（動的にメモリ確保が出来ないので配列を使用）
	tsSequentialEvt sEventList[APP_SEQUENTIAL_EVT_SIZE];
} tsSeqEvtTaskInfo;

// 構造体：送信パケット
typedef struct {
	// パケット種別
	uint8 u8PaketType;
	// コマンド
	uint8 u8Command;
	// 送信ソルト
	uint16 u16Solt[3];
	// データ
	uint8 u8Data[32];
} tsSendMessage;


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
// ハードウェア割り込みハンドラ（即時実行）
PRIVATE tsHwIntTask sHwEventTask;
// イベントタスク情報リスト
PRIVATE tsEventTask sEventTaskList[APP_EVENT_TASK_SIZE];
// スケジュールイベント情報
PRIVATE tsScheduleEvtTaskInfo sScheduleEvtInfo;
// 順次実行イベント情報
PRIVATE tsSeqEvtTaskInfo sSeqEvtInfo;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
// ハードウェアイベントタスクの初期処理
PRIVATE void vInitHWEvtTask();
// イベントタスクの初期処理
PRIVATE void vInitEventTask();
// スケジュールイベントの初期処理
PRIVATE void vInitScheduleEvent();
// 順次実行イベントの初期処理
PRIVATE void vInitSeqEvent();
// ユーザーイベント処理
PRIVATE void vProcessEvCore(tsEvent *psEv, teEvent eEvent, uint32 u32EvArg);
// スケジュールイベントのエンキュー処理
PRIVATE int iEnqueueScheduleEvt(tsScheduleEvt *spAddEvt);
// スケジュール処理
PRIVATE void vExecScheduleTask();
// 特定のスケジュールイベントの初期化処理
PRIVATE void vClearScheduleEvt(tsScheduleEvt *spEvt);
// 次回実行時刻更新処理
PRIVATE void vUpdNextExec(uint32 u32RefTime, tsScheduleEvt *spEvt);
// イベント処理
PRIVATE void vExecEventTask(uint32 u32StartTime);
// タスク処理：空ハードウェアイベントタスク
PRIVATE uint8 u8EmptyHwIntTask(uint32 u32DeviceId, uint32 u32ItemBitmap);
// タスク処理：空タスク
PRIVATE void vEmptyTask(uint32 u32EvtTimeMs);

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 * NAME:AppColdStart
 *
 * DESCRIPTION:電源投入もしくはスリープ（メモリ非保持）からの復帰時の処理
 *             引数はハードウェアの初期化の有無
 *             ハードウェアの初期化前に利用モジュールの設定
 *             ハードウェアの初期化後に、必要に応じて下記の処理を行う
 *
 *             ・ネットワーク設定
 *             ・ハードウェアの初期化
 *             ・ユーザー定義イベントの登録
 *             ・MAC層の開始を行う
 *
 * RETURNS:
 *
 ****************************************************************************/
void cbAppColdStart(bool_t bAfterAhiInit) {
	//=========================================================================
	// ハードウェアの起動処理
	//=========================================================================
	// ハードウェアの初期化判定
	if (!bAfterAhiInit) {
		// before AHI init, very first of code.
		// Register modules
		// ToCoNetライブラリのモジュール初期化処理
		ToCoNet_REG_MOD_ALL();
		// ハードウェアの初期化前処理の終了
		return;
	}

	//=========================================================================
	// ハードウェアの初期設定処理
	//=========================================================================
	// ユーザーイベント関数の登録
	ToCoNet_Event_Register_State_Machine(vProcessEvCore);
	// ハードウェアの初期設定処理
	vInitHardware(FALSE);
	// ネットワークの初期化と接続開始処理
	ToCoNet_vMacStart();

	//=========================================================================
	// イベント処理関係の初期化
	//=========================================================================
	// ハードウェアイベントタスクの初期化
	vInitHWEvtTask();
	// イベントタスクの初期化
	vInitEventTask();
	// スケジュールイベントの初期化
	vInitScheduleEvent();
	// 順次実行イベントの初期化
	vInitSeqEvent();
}

/****************************************************************************
 * NAME:AppWarmStart
 *
 * DESCRIPTION:スリープ（メモリ保持）からの復帰時の処理
 *             引数はハードウェアの初期化の有無
 *             ハードウェアの初期化前：起床要因の判定処理
 *             ハードウェアの初期化後：ハードウェアの初期化処理
 *
 * RETURNS:
 *
 ****************************************************************************/
void cbAppWarmStart(bool_t bAfterAhiInit) {
	//=========================================================================
	// ハードウェアの初期化前の処理
	//=========================================================================
	if (!bAfterAhiInit) {
		return;
	}
	// MAC start
	// ネットワークの初期化と接続開始処理
	ToCoNet_vMacStart();
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 * NAME:vMain
 *
 * DESCRIPTION:主処理、各種イベントの最後に実行される
 *             何らかの状態をチェックして実施する処理を実装
 *             長時間（10ms掛かるだけでも不可の様子）掛かる処理の実行は出来ない
 *
 * RETURNS:
 *
 ****************************************************************************/
void cbToCoNet_vMain() {
}

/****************************************************************************
 * NAME:cbToCoNet_u8HwInt
 *
 * DESCRIPTION:ハードウェア割り込みハンドラ、即時性の高い短い処理のみ実装すること
 *             TICK_TIMERに対してTRUEを返すとToCoNetは動作しなくなります。
 *   called during an interrupt
 *
 * PARAMETERS:      Name            RW  Usage
 *     uint32       u32DeviceId     R   デバイスID
 *     uint32       u32ItemBitmap   R   ビットマップ
 *
 * RETURNS:
 *                  FALSE -  interrupt is not handled, escalated to further
 *                           event call (cbToCoNet_vHwEvent).
 *                  TRUE  -  interrupt is handled, no further call.
 *
 * NOTES:
 *   Do not put a big job here.
 ****************************************************************************/
uint8 cbToCoNet_u8HwInt(uint32 u32DeviceId, uint32 u32ItemBitmap) {
	// イベント処理関数ポインタ
	uint8 (*pHwEvtTask)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	// イベント発生源の判定
	switch (u32DeviceId) {
	case E_AHI_DEVICE_TIMER0:		/* Timer 0 */
		pHwEvtTask = sHwEventTask.pTimer_0_Task;
		break;
	case E_AHI_DEVICE_TIMER1:		/* Timer 1 */
		pHwEvtTask = sHwEventTask.pTimer_1_Task;
		break;
	case E_AHI_DEVICE_TIMER2:		/* Timer 2 */
		pHwEvtTask = sHwEventTask.pTimer_2_Task;
		break;
	case E_AHI_DEVICE_TIMER3:		/* Timer 3 */
		pHwEvtTask = sHwEventTask.pTimer_3_Task;
		break;
	case E_AHI_DEVICE_TIMER4:		/* Timer 4 */
		pHwEvtTask = sHwEventTask.pTimer_4_Task;
		break;
	case E_AHI_DEVICE_SYSCTRL:		/* System controller */
		pHwEvtTask = sHwEventTask.pSysCtrlTask;
		break;
	case E_AHI_DEVICE_BBC:			/* Baseband controller */
		pHwEvtTask = sHwEventTask.pBBCTask;
		break;
	case E_AHI_DEVICE_AES:			/* Encryption engine */
		pHwEvtTask = sHwEventTask.pAESTask;
		break;
	case E_AHI_DEVICE_PHYCTRL:		/* Phy controller */
		pHwEvtTask = sHwEventTask.pPhyCtrlTask;
		break;
	case E_AHI_DEVICE_UART0:		/* UART 0 */
		pHwEvtTask = sHwEventTask.pUART_0_Task;
		break;
	case E_AHI_DEVICE_UART1:		/* UART 1 */
		pHwEvtTask = sHwEventTask.pUART_1_Task;
		break;
	case E_AHI_DEVICE_SI:			/* Serial Interface (2 wire) */
		pHwEvtTask = sHwEventTask.pSITask;
		break;
	case E_AHI_DEVICE_SPIM:			/* SPI master */
		pHwEvtTask = sHwEventTask.pSPIMTask;
		break;
	case E_AHI_DEVICE_SPIS:			/* SPI slave */
		pHwEvtTask = sHwEventTask.pSPISTask;
		break;
	case E_AHI_DEVICE_ANALOGUE:		/* Analogue peripherals */
		pHwEvtTask = sHwEventTask.pAnalogueTask;
		break;
	case E_AHI_DEVICE_TICK_TIMER:	/* Tick timer */
		if (sHwEventTask.pTickTimerTask != NULL) {
			(*sHwEventTask.pTickTimerTask)(u32DeviceId, u32ItemBitmap);
		}
		return FALSE;
	case E_AHI_DEVICE_FEC:			/* Flash and EEPROM Controller */
		pHwEvtTask = sHwEventTask.pFlashEEPTask;
		break;
	case E_AHI_DEVICE_INFRARED:		/* Infrared */
		pHwEvtTask = sHwEventTask.pInfraredTask;
		break;
	default:
		return TRUE;
	}
	return (*pHwEvtTask)(u32DeviceId, u32ItemBitmap);
}

/****************************************************************************
 * NAME:cbToCoNet_vHwEvent
 *
 * DESCRIPTION:ハードウェアイベント処理、即時性が必要な短い処理は
 *             cbToCoNet_u8HWInt関数に実装する
 *             cbToCoNet_u8HWInt関数の戻り値がFALSEの時にのみ
 *             この関数が呼び出される
 * Process any hardware events.
 *
 * PARAMETERS:      Name            RW  Usage
 *     uint32       u32DeviceId	    R
 *     uint32       u32ItemBitmap   R
 *
 * RETURNS:
 *
 ****************************************************************************/
void cbToCoNet_vHwEvent(uint32 u32DeviceId, uint32 u32ItemBitmap) {
}

/****************************************************************************
 * NAME:vRegisterHwIntTask
 *
 * DESCRIPTION:ハードウェア割り込み処理の登録処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32DeviceId     R   デバイスID
 *   uint8*         pFunc           R   割り込み関数のポインタ
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PUBLIC bool_t vRegisterHwIntTask(uint32 u32DeviceId
			,uint8 (*u8pFunc)(uint32 u32DeviceId, uint32 u32ItemBitmap)) {
	// タスク判定
	uint8 (*u8pHwEvtTask)(uint32 u32DeviceId, uint32 u32ItemBitmap) = u8pFunc;
	if (u8pHwEvtTask == NULL) {
		u8pHwEvtTask = u8EmptyHwIntTask;
	}
	// ハードウェアの判定
	switch (u32DeviceId) {
	case E_AHI_DEVICE_TIMER0:		/* Timer 0 */
		sHwEventTask.pTimer_0_Task = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_TIMER1:		/* Timer 1 */
		sHwEventTask.pTimer_1_Task = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_TIMER2:		/* Timer 2 */
		sHwEventTask.pTimer_2_Task = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_TIMER3:		/* Timer 3 */
		sHwEventTask.pTimer_3_Task = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_TIMER4:		/* Timer 4 */
		sHwEventTask.pTimer_4_Task = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_SYSCTRL:		/* System controller */ //DIOもこれ
		sHwEventTask.pSysCtrlTask = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_BBC:		/* Baseband controller */
		sHwEventTask.pBBCTask = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_AES:		/* Encryption engine */
		sHwEventTask.pAESTask = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_PHYCTRL:		/* Phy controller */
		sHwEventTask.pPhyCtrlTask = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_UART0:		/* UART 0 */
		sHwEventTask.pUART_0_Task = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_UART1:		/* UART 1 */
		sHwEventTask.pUART_1_Task = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_SI:		/* Serial Interface (2 wire) */
		sHwEventTask.pSITask = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_SPIM:		/* SPI master */
		sHwEventTask.pSPIMTask = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_SPIS:		/* SPI slave */
		sHwEventTask.pSPISTask = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_ANALOGUE:		/* Analogue peripherals */
		sHwEventTask.pAnalogueTask = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_TICK_TIMER:	/* Tick timer */
		sHwEventTask.pTickTimerTask = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_FEC:			/* Flash and EEPROM Controller */
		sHwEventTask.pFlashEEPTask = u8pHwEvtTask;
		break;
	case E_AHI_DEVICE_INFRARED:		/* Infrared */
		sHwEventTask.pInfraredTask = u8pHwEvtTask;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

/****************************************************************************
 * NAME:bRegisterEvtTask
 *
 * DESCRIPTION:イベントタスク登録処理
 *
 * PARAMETERS:      Name            RW  Usage
 * 	   teFwkEvent   eEvt            R   イベント種別
 * 	   void         *pFunc          R   実行処理関数のポインタ
 *
 * RETURNS:
 *     bool_t       登録結果、登録出来なかった場合にはFALSE
 *
 * NOTES:
 ****************************************************************************/
PUBLIC bool_t bRegisterEvtTask(teFwkEvent eEvt, void (*vpFunc)(uint32 u32EvtTimeMs)) {
	// 登録可否判定
	uint8 u8TaskIdx = eEvt % APP_EVENT_TASK_SIZE;
	if (sEventTaskList[u8TaskIdx].eEvent != ToCoNet_EVENT_APP_BASE) {
		return FALSE;
	}
	// タスク情報登録
	sEventTaskList[u8TaskIdx].eEvent = eEvt;
	sEventTaskList[u8TaskIdx].u32MaxExecTime = 0;
	sEventTaskList[u8TaskIdx].vpFunc = vpFunc;
	// 登録完了
	return TRUE;
}

/****************************************************************************
 * NAME:bUnregisterEventTask
 *
 * DESCRIPTION:イベントタスク登録解除処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   teFwkEvent     eEvt            R   イベント種別
 *
 * RETURNS:
 *     bool_t       TRUE：登録解除成功
 *
 * NOTES:
 ****************************************************************************/
PUBLIC bool_t bUnregisterEventTask(teFwkEvent eEvt) {
	// タスク情報クリア
	uint16 u16TaskIdx = (uint16)eEvt % APP_EVENT_TASK_SIZE;
	if (sEventTaskList[u16TaskIdx].eEvent == ToCoNet_EVENT_APP_BASE) {
		// 登録解除対象無し
		return FALSE;
	}
	sEventTaskList[u16TaskIdx].eEvent          = ToCoNet_EVENT_APP_BASE;
	sEventTaskList[u16TaskIdx].u32MaxExecTime  = 0;
	sEventTaskList[u16TaskIdx].vpFunc          = vEmptyTask;
	// 登録解除完了
	return TRUE;
}

/****************************************************************************
 *
 * NAME:iEntryScheduleEvt
 *
 * DESCRIPTION:スケジュール実行イベントの登録処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   teFwkEvent     eEvt            R   イベント種別
 *   uint32         u32Interval     R   実行間隔（ミリ秒単位）
 *   uint32         u32Offset       R   実行開始オフセット
 *   bool_t         bRepeatFlg      R   反復実行フラグ
 *
 * RETURNS:
 *     int          登録したイベントID、登録出来なかった場合には-1
 *
 * NOTES:
 ****************************************************************************/
PUBLIC int iEntryScheduleEvt(teFwkEvent eEvt, uint32 u32Interval, uint32 u32Offset, bool_t bRepeatFlg) {
	// 登録可否判定
	if (sScheduleEvtInfo.spEmptyEvt == NULL) {
		return -1;
	}
	// 空タスク情報を取得
	tsScheduleEvt *psAddEvt;
	psAddEvt = sScheduleEvtInfo.spEmptyEvt;
	sScheduleEvtInfo.spEmptyEvt = psAddEvt->spNextEvt;
	// タスク情報登録
	psAddEvt->eEvent        = eEvt;
	psAddEvt->u32Interval   = u32Interval;
	psAddEvt->u32NextExecMs = u32TickCount_ms + u32Offset;
	psAddEvt->bRepeatFlg    = bRepeatFlg;
	// リンクリストへのエンキュー
	return iEnqueueScheduleEvt(psAddEvt);
}

/****************************************************************************
 * NAME:bCancelScheduleEvt
 *
 * DESCRIPTION:スケジュール実行イベントの登録解除処理
 *
 * PARAMETERS:      Name            RW  Usage
 *  int             iEvtID          R   イベントID
 *
 * RETURNS:
 *  bool_t          TRUE：登録解除成功
 *
 * NOTES:
 ****************************************************************************/
PUBLIC bool_t bCancelScheduleEvt(int iEvtID) {
	// タスク有無判定
	tsScheduleEvt *spTargetEvt = &sScheduleEvtInfo.sEventList[iEvtID];
	if (spTargetEvt->eEvent == ToCoNet_EVENT_APP_BASE) {
		return FALSE;
	}
	// 直前要素処理
	tsScheduleEvt *spChkEvt = sScheduleEvtInfo.spBeginEvt;
	while (spChkEvt != NULL) {
		if (spChkEvt->spNextEvt == spTargetEvt) {
			spChkEvt->spNextEvt = spTargetEvt->spNextEvt;
			break;
		}
		spChkEvt = spChkEvt->spNextEvt;
	}
	if (spChkEvt == NULL) {
		sScheduleEvtInfo.spBeginEvt = spTargetEvt->spNextEvt;
	}
	// イベント情報初期化
	vClearScheduleEvt(spTargetEvt);
	// 未使用タスクリストに追加
	sScheduleEvtInfo.spEmptyEvt = spTargetEvt;
	// 登録解除完了
	return TRUE;
}

/****************************************************************************
 * NAME:iEntrySeqEvt
 *
 * DESCRIPTION:順次実行イベントの登録処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   teFwkEvent     eEvt            R   登録イベント
 *
 * RETURNS:
 *     int          登録したイベントID、登録出来なかった場合には-1
 *
 * NOTES:
 ****************************************************************************/
PUBLIC int iEntrySeqEvt(teFwkEvent eEvt) {
	// 最大イベントキューサイズを確認
	if (sSeqEvtInfo.u8Size >= APP_SEQUENTIAL_EVT_SIZE) return -1;
	// イベント情報の割り当て
	uint8 u8EntryIdx = (sSeqEvtInfo.u8BeginIdx + sSeqEvtInfo.u8Size) % APP_SEQUENTIAL_EVT_SIZE;
	tsSequentialEvt *psEntryEvt = &sSeqEvtInfo.sEventList[u8EntryIdx];
	psEntryEvt->eEvent = eEvt;
	psEntryEvt->u32EntryTimeMs = u32TickCount_ms;
	sSeqEvtInfo.u8Size++;
	return u8EntryIdx;
}

/****************************************************************************
 * NAME:bCancelSeqEvt
 *
 * DESCRIPTION:順次実行イベントの登録解除処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   int            iEvtID          R   対象イベントインデックス
 *
 * RETURNS:
 *  bool_t          TRUE：登録解除成功
 *
 * NOTES:
 ****************************************************************************/
PUBLIC bool_t bCancelSeqEvt(int iEvtID) {
	tsSequentialEvt *sEntryEvt = &sSeqEvtInfo.sEventList[iEvtID];
	sEntryEvt->eEvent = ToCoNet_EVENT_APP_BASE;
	sEntryEvt->u32EntryTimeMs = 0;
	return TRUE;
}

#ifndef USER_DEFINE_STACK_OVER_FLOW
/****************************************************************************
 * NAME:vException_StackOverflow
 *
 * DESCRIPTION:スタックオーバーフロー発生時の処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PUBLIC void vException_StackOverflow() {
	// 最終イベント
#ifdef DEBUG
	vfPrintf(&sSerStream, "\nMS:%08d StackOverflow!!! Err:%d\n", u32TickCount_ms, errno);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
}
#endif

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 * NAME:vInitHWEvtTask
 *
 * DESCRIPTION:ハードウェアイベントタスクの初期処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ****************************************************************************/
PRIVATE void vInitHWEvtTask() {
	// ハードウェアイベント処理の初期化
	vRegisterHwIntTask(E_AHI_DEVICE_TIMER0, u8EmptyHwIntTask);		// 関数ポインタ：Timer 0
	vRegisterHwIntTask(E_AHI_DEVICE_TIMER1, u8EmptyHwIntTask);		// 関数ポインタ：Timer 1
	vRegisterHwIntTask(E_AHI_DEVICE_TIMER2, u8EmptyHwIntTask);		// 関数ポインタ：Timer 2
	vRegisterHwIntTask(E_AHI_DEVICE_TIMER3, u8EmptyHwIntTask);		// 関数ポインタ：Timer 3
	vRegisterHwIntTask(E_AHI_DEVICE_TIMER4, u8EmptyHwIntTask);		// 関数ポインタ：Timer 4
	vRegisterHwIntTask(E_AHI_DEVICE_SYSCTRL, u8EmptyHwIntTask);		// 関数ポインタ：System controller
	vRegisterHwIntTask(E_AHI_DEVICE_BBC, u8EmptyHwIntTask);			// 関数ポインタ：Baseband controller
	vRegisterHwIntTask(E_AHI_DEVICE_AES, u8EmptyHwIntTask);			// 関数ポインタ：Encryption engine
	vRegisterHwIntTask(E_AHI_DEVICE_PHYCTRL, u8EmptyHwIntTask);		// 関数ポインタ：Phy controller
	vRegisterHwIntTask(E_AHI_DEVICE_UART0, u8EmptyHwIntTask);		// 関数ポインタ：UART0
	vRegisterHwIntTask(E_AHI_DEVICE_UART1, u8EmptyHwIntTask);		// 関数ポインタ：UART1
	vRegisterHwIntTask(E_AHI_DEVICE_SI, u8EmptyHwIntTask);			// 関数ポインタ：Serial Interface (2 wire)
	vRegisterHwIntTask(E_AHI_DEVICE_SPIM, u8EmptyHwIntTask);		// 関数ポインタ：SPI master
	vRegisterHwIntTask(E_AHI_DEVICE_SPIS, u8EmptyHwIntTask);		// 関数ポインタ：SPI slave
	vRegisterHwIntTask(E_AHI_DEVICE_ANALOGUE, u8EmptyHwIntTask);	// 関数ポインタ：Analogue peripherals
	vRegisterHwIntTask(E_AHI_DEVICE_TICK_TIMER, u8EmptyHwIntTask);	// 関数ポインタ：Tick timer
	vRegisterHwIntTask(E_AHI_DEVICE_FEC, u8EmptyHwIntTask);			// 関数ポインタ：Flash and EEPROM Controller
	vRegisterHwIntTask(E_AHI_DEVICE_INFRARED, u8EmptyHwIntTask);	// 関数ポインタ：Infrared
}

/****************************************************************************
 * NAME:vInitEventTask
 *
 * DESCRIPTION:イベントタスクの初期処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ****************************************************************************/
PRIVATE void vInitEventTask() {
	tsEventTask *sTask;
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < APP_EVENT_TASK_SIZE; u8Idx++) {
		sTask = &sEventTaskList[u8Idx];
		sTask->vpFunc = vEmptyTask;
		sEventTaskList[u8Idx].u32MaxExecTime = 0;
		sEventTaskList[u8Idx].eEvent = ToCoNet_EVENT_APP_BASE;
	}
}

/****************************************************************************
 * NAME:vInitScheduleEvent
 *
 * DESCRIPTION:スケジュールイベントの初期処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ****************************************************************************/
PRIVATE void vInitScheduleEvent() {
	// 先頭のイベント情報初期化
	tsScheduleEvt *prevEvt = &sScheduleEvtInfo.sEventList[0];
	vClearScheduleEvt(prevEvt);
	prevEvt->u8EvtIdx  = 0;		// イベントインデックス
	prevEvt->spNextEvt = NULL;	// 次イベント
	// ２件目以降のイベント情報初期化
	tsScheduleEvt *curEvt;
	uint8 u8Idx = 0;
	for (u8Idx = 1; u8Idx < APP_SCHEDULE_EVT_SIZE; u8Idx++) {
		curEvt  = &sScheduleEvtInfo.sEventList[u8Idx];
		vClearScheduleEvt(curEvt);
		curEvt->u8EvtIdx   = u8Idx;
		prevEvt->spNextEvt = curEvt;
		prevEvt = curEvt;
	}
	curEvt->spNextEvt = NULL;
	// 各リンクリスト情報を初期化
	sScheduleEvtInfo.spEmptyEvt = &sScheduleEvtInfo.sEventList[0];
	sScheduleEvtInfo.spBeginEvt = NULL;
}

/****************************************************************************
 * NAME:vInitSeqEvent
 *
 * DESCRIPTION:順次実行イベントの初期処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ****************************************************************************/
PRIVATE void vInitSeqEvent() {
	// イベントキューの初期化
	sSeqEvtInfo.u8Size = 0;			// リストサイズ
	sSeqEvtInfo.u8BeginIdx = 0;		// イベント情報：先頭
	// イベント情報配列を初期化
	tsSequentialEvt *sEvt;
	uint8 idx;
	for (idx = 0; idx < APP_SEQUENTIAL_EVT_SIZE; idx++) {
		sEvt = &sSeqEvtInfo.sEventList[idx];
		sEvt->eEvent = ToCoNet_EVENT_APP_BASE;		// イベント種別
		sEvt->u32EntryTimeMs = 0;					// イベント発生時刻
	}
}

/****************************************************************************
 * NAME:vProcessEvent
 *
 * DESCRIPTION:ユーザーイベント関数
 *
 * PARAMETERS:      Name            RW  Usage
 *   tsEvent        *sEv            R   イベント情報の構造体へのポインタ
 *   teEvent        eEvent          R   イベント種別
 *   uint32         u32evarg        R   イベント引数
 *
 * RETURNS:
 *
 ****************************************************************************/
PRIVATE void vProcessEvCore(tsEvent *psEv, teEvent eEvent, uint32 u32EvArg) {
	// イベント処理開始時刻
	uint32 u32EvtBegin = u32TickCount_ms;
	// イベント判定
	switch (eEvent) {
	// システム始動
	case E_EVENT_START_UP:
		vEventStartup(psEv, eEvent, u32EvArg);
		break;
	// Tick Timer割り込み（4ms周期）
	case E_EVENT_TICK_TIMER:
		// 周期処理の実行
		vExecScheduleTask();
		// イベント処理の実行
		vExecEventTask(u32EvtBegin);
		break;
	// Tick Timer割り込み（1秒周期）
	case E_EVENT_TICK_SECOND:
		break;
	// 他の状態遷移が発生した場合
	case E_EVENT_NEW_STATE:
		break;
	default:
		break;
	}
}

/****************************************************************************
 * NAME:iEnqueueScheduleEvt
 *
 * DESCRIPTION:スケジュール実行イベントのエンキュー処理
 *
 * PARAMETERS:         Name          RW  Usage
 *   tsScheduleEvt*    spAddEvt      R   追加するイベント情報のポインタ
 *
 * RETURNS:
 *   int   追加成功時はタスク情報インデックス、失敗時は-1
 *
 ****************************************************************************/
PRIVATE int iEnqueueScheduleEvt(tsScheduleEvt *spAddEvt) {
	// エンキュー可否判定
	if (spAddEvt == NULL) {
		return -1;
	}
	// リンクリストの挿入ポイントを末端から探索
	tsScheduleEvt *spPrevEvt = NULL;
	tsScheduleEvt *spChkEvt  = sScheduleEvtInfo.spBeginEvt;
	while (spChkEvt != NULL) {
		if (spAddEvt->u32NextExecMs < spChkEvt->u32NextExecMs) {
			break;
		}
		spPrevEvt = spChkEvt;
		spChkEvt  = spChkEvt->spNextEvt;
	}
	// 後続要素を結合
	spAddEvt->spNextEvt = spChkEvt;
	// 直前の要素に結合
	if (spPrevEvt != NULL) {
		spPrevEvt->spNextEvt = spAddEvt;
	} else {
		sScheduleEvtInfo.spBeginEvt = spAddEvt;
	}
	// 追加したイベント情報インデックスを返す
	return (int)spAddEvt->u8EvtIdx;
}


/****************************************************************************
 * NAME:vExecScheduleTask
 *
 * DESCRIPTION:スケジュール実行される処理タスクを実行する
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vExecScheduleTask() {
	// 現在時刻（ミリ秒）を取得
	uint32 u32CurrentTimeMs = u32TickCount_ms;
	// イベントタスク情報
	tsEventTask *spEvtTask;
	// 周期タスクの実行ループ
	tsScheduleEvt *spTargetEvt = sScheduleEvtInfo.spBeginEvt;
	while (spTargetEvt != NULL) {
		// 開始時刻判定
		if (spTargetEvt->u32NextExecMs > u32CurrentTimeMs) {
			break;
		}
		// デキュー処理
		sScheduleEvtInfo.spBeginEvt = spTargetEvt->spNextEvt;
		// スケジュールタスクの実行
		spEvtTask = &sEventTaskList[spTargetEvt->eEvent % APP_EVENT_TASK_SIZE];
		(spEvtTask->vpFunc)(spTargetEvt->u32NextExecMs);
		// 反復実行判定
		if (spTargetEvt->bRepeatFlg) {
			// 次回の開始時刻を概算し、その概算値から端数を切り捨て
			vUpdNextExec(u32CurrentTimeMs, spTargetEvt);
			// 次サイクルの処理情報をエンキュー
			iEnqueueScheduleEvt(spTargetEvt);
		} else {
			// 初期化して未使用タスクリストに追加
			vClearScheduleEvt(spTargetEvt);
			sScheduleEvtInfo.spEmptyEvt = spTargetEvt;
		}
		// 次のタスク情報を参照
		spTargetEvt = sScheduleEvtInfo.spBeginEvt;
	}
}

/****************************************************************************
 * NAME:vClearScheduleEvt
 *
 * DESCRIPTION:特定のスケジュールイベントのクリア処理
 *
 * PARAMETERS:         Name          RW  Usage
 *   tsScheduleEvt*    spEvt         R   初期化対象のイベント
 *
 * RETURNS:
 *
 ****************************************************************************/
PRIVATE void vClearScheduleEvt(tsScheduleEvt *spEvt) {
	spEvt->eEvent = ToCoNet_EVENT_APP_BASE;	// イベント種別
	spEvt->u32Interval     = 0;				// 処理間隔
	spEvt->u32NextExecMs   = 0;				// 処理開始時刻
	spEvt->bRepeatFlg      = FALSE;			// 反復実行フラグ
	spEvt->spNextEvt       = sScheduleEvtInfo.spEmptyEvt;	// 次のタスク情報
}

/****************************************************************************
 * NAME:vUpdNextExec
 *
 * DESCRIPTION:次回実行時刻更新処理
 *
 * PARAMETERS:       Name       RW  Usage
 *   uint32          u32RefTime R   基準時刻
 *   tsScheduleEvt*  spEvt      R   対象イベント情報
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vUpdNextExec(uint32 u32RefTime, tsScheduleEvt *spEvt) {
	// 次回開始時刻までの概算値 = 経過時間（現在時刻 - 実行開始時刻） + 実行間隔
	uint32 u32ExecMs = u32RefTime - spEvt->u32NextExecMs + spEvt->u32Interval;
	// 実行間隔以下の端数を切り捨てて加算
	spEvt->u32NextExecMs += u32ExecMs - u32ExecMs % spEvt->u32Interval;
}

/****************************************************************************
 * NAME:vExecEventTask
 *
 * DESCRIPTION:
 *   イベントキューに登録された順次実行イベントに対応する処理を実行する
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32StartTime    R   ユーザー定義タスク開始時刻（タイムアウト判定に利用）
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vExecEventTask(uint32 u32StartTime) {
	// タイムアウト時刻算出
	uint32 u32TimeOut = u32StartTime + APP_EVENT_TIMEOUT;
	// イベント処理時間
	tsSequentialEvt *spTargetEvt;		// イベント情報
	tsEventTask *spTargetTask;			// イベントタスク情報
	uint32 u32BefExecMs, u32AftExecMs;	// タスク実行前時間
	uint32 u32ExecTime;					// 推定処理完了時間
	uint8 u8ExecSw = 0;					// 先頭イベントのタイムアウト判定OFF
	// イベントキューループ
	while (sSeqEvtInfo.u8Size > 0) {
		// デキュー処理
		spTargetEvt = &sSeqEvtInfo.sEventList[sSeqEvtInfo.u8BeginIdx];
		// イベント処理判定
		if (spTargetEvt->eEvent > ToCoNet_EVENT_APP_BASE) {
			// イベントタスク
			spTargetTask = &sEventTaskList[spTargetEvt->eEvent % APP_EVENT_TASK_SIZE];
			// 推定処理完了時刻を算出してタイムアウト判定
			u32BefExecMs = u32TickCount_ms;
			u32AftExecMs = (u32BefExecMs + spTargetTask->u32MaxExecTime) * u8ExecSw;
			if (u32AftExecMs >= u32TimeOut) return;
			// イベントタスク実行
			(*spTargetTask->vpFunc)(spTargetEvt->u32EntryTimeMs);
			// 最長処理時間の更新
			u32ExecTime = (u32TickCount_ms - u32BefExecMs) / 1000;
			if (u32ExecTime > spTargetTask->u32MaxExecTime) {
				spTargetTask->u32MaxExecTime = u32ExecTime;
			}
			// 先頭イベントのタイムアウト判定ON
			u8ExecSw = 1;
		}
		// キュー情報の更新
		sSeqEvtInfo.u8Size--;
		sSeqEvtInfo.u8BeginIdx = (sSeqEvtInfo.u8BeginIdx + 1) % APP_SEQUENTIAL_EVT_SIZE;
	}
	sSeqEvtInfo.u8BeginIdx = (sSeqEvtInfo.u8BeginIdx + APP_SEQUENTIAL_EVT_SIZE - 1) % APP_SEQUENTIAL_EVT_SIZE;
}

/****************************************************************************
 * NAME:u8EmptyHwIntTask
 *
 * DESCRIPTION:ハードウェアイベント空タスク
 *
 * PARAMETERS:      Name            RW  Usage
 *     uint32       u32DeviceId     R   デバイスID
 *     uint32       u32ItemBitmap   R   ビットマップ
 *
 * RETURNS:
 *     TRUE  -  interrupt is handled, no further call.
 *
 * NOTES:
 ****************************************************************************/
PRIVATE uint8 u8EmptyHwIntTask(uint32 u32DeviceId, uint32 u32ItemBitmap){
	return TRUE;
}

/****************************************************************************
 * NAME:vEmptyTask
 *
 * DESCRIPTION:空タスク
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻（ミリ秒）
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vEmptyTask(uint32 u32EvtTimeMs){
	return;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
