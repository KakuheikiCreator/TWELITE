/*******************************************************************************
 *
 * MODULE :Application Event source file
 *
 * CREATED:2017/08/29 00:00:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:イベント処理を実装
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 *******************************************************************************
 * Copyright (c) 2017, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 ******************************************************************************/

/******************************************************************************/
/***        Include files                                                   ***/
/******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <jendefs.h>
#include <AppHardwareApi.h>

/******************************************************************************/
/***        ToCoNet Definitions                                             ***/
/******************************************************************************/
#include "serial.h"
#include "sprintf.h"

/******************************************************************************/
/***        ToCoNet Include files                                           ***/
/******************************************************************************/
#include "ToCoNet_use_mod.h"
#include "ToCoNet.h"
#include "ToCoNet_mod_prototype.h"
#include "ccitt8.h"

/******************************************************************************/
/***        User Include files                                              ***/
/******************************************************************************/
#include "config.h"
#include "config_default.h"
#include "framework.h"
#include "io_util.h"
//#include "pwm_util.h"
#include "sha256.h"
#include "app_event.h"
#include "app_io.h"
#include "app_auth.h"
#include "app_main.h"
#include "value_util.h"

/******************************************************************************/
/***        Macro Definitions                                               ***/
/******************************************************************************/

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/
// イベントマップ編集（エラー）
PRIVATE void vEvt_InitEvtMap();
// 受信メッセージ基本チェック
PRIVATE bool_t bEvt_DefaultMsgChk(tsWirelessMsg* psWlsMsg, teAppCommand eCmd, tsAuthRemoteDevInfo* psRmtDevInfo);

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/

/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: vEvent_RegistTask
 *
 * DESCRIPTION:タスクの登録
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
PUBLIC void vEvent_RegistTask() {
	//==========================================================================
	// ハードウェアイベント処理タスク
	//==========================================================================
	// システムコントロールの割り込み処理
	vRegisterHwIntTask(E_AHI_DEVICE_SYSCTRL, u8EventSysCtrl);
	// Tick Timerの割り込み処理
	vRegisterHwIntTask(E_AHI_DEVICE_TICK_TIMER, u8EventTickTimer);

	//==========================================================================
	// ユーザーイベント処理タスク
	//==========================================================================
	bRegisterEvtTask(E_EVENT_INITIALIZE, vEvent_Init);
	bRegisterEvtTask(E_EVENT_CHK_BTN, vEvent_CheckBtn);
	bRegisterEvtTask(E_EVENT_SCR_SEL_DEV_INIT, vEvent_SelDev_init);
	bRegisterEvtTask(E_EVENT_SCR_SEL_DEV_CHG, vEvent_SelDev_chg);
	bRegisterEvtTask(E_EVENT_SCR_SEL_CMD_INIT, vEvent_SelCmd_init);
	bRegisterEvtTask(E_EVENT_SCR_SEL_CMD_CHG, vEvent_SelCmd_chg);
	bRegisterEvtTask(E_EVENT_EXEC_CMD_0, vEvent_Exec_Cmd_0);
	bRegisterEvtTask(E_EVENT_EXEC_CMD_1, vEvent_Exec_Cmd_1);
	bRegisterEvtTask(E_EVENT_EXEC_AUTH_CMD_0, vEvent_Exec_AuthCmd_0);
	bRegisterEvtTask(E_EVENT_EXEC_AUTH_CMD_1, vEvent_Exec_AuthCmd_1);
	bRegisterEvtTask(E_EVENT_EXEC_AUTH_CMD_2, vEvent_Exec_AuthCmd_2);
	bRegisterEvtTask(E_EVENT_EXEC_AUTH_CMD_3, vEvent_Exec_AuthCmd_3);
	bRegisterEvtTask(E_EVENT_EXEC_AUTH_CMD_4, vEvent_Exec_AuthCmd_4);
	bRegisterEvtTask(E_EVENT_EXEC_AUTH_CMD_5, vEvent_Exec_AuthCmd_5);
	bRegisterEvtTask(E_EVENT_RX_TIMEOUT, vEvent_RxTimeout);
	bRegisterEvtTask(E_EVENT_HASH_ST, vEvent_HashStretching);

	//==========================================================================
	// スケジュールイベント登録
	//==========================================================================
	// タスク登録：アプリケーション初期化
	iEntryScheduleEvt(E_EVENT_INITIALIZE, 0, 0, FALSE);
	// タスク登録：ボタン入力イベントチェック
	iEntryScheduleEvt(E_EVENT_CHK_BTN, 20, 30, TRUE);
}

/*******************************************************************************
 *
 * NAME: vEvent_Init
 *
 * DESCRIPTION:アプリケーション初期化イベント処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_Init(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 設定初期化
	//==========================================================================
	// アプリケーションイベントマップ
	vEvt_InitEvtMap();
	// 送受信トランザクション
	memset(&sAppTxRxTrns, 0x00, sizeof(tsAppTxRxTrnsInfo));
	sAppTxRxTrns.iTimeoutEvtID = -1;
	// ハッシュ値生成情報
	memset(&sHashGenInfo, 0x00, sizeof(tsAuthHashGenState));
	// 入力バッファの更新
	vUpdInputBuffer();
	// 受信待ち設定
	vWirelessRxDisabled();

	//==========================================================================
	// 初期イベント実行
	//==========================================================================
	// デバイス選択初期イベント
	iEntrySeqEvt(E_EVENT_SCR_SEL_DEV_INIT);
}

/*******************************************************************************
 *
 * NAME: vEvent_CheckBtn
 *
 * DESCRIPTION:ボタン入力イベントチェック処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_CheckBtn(uint32 u32EvtTimeMs) {
	// 入力バッファの更新
	vUpdInputBuffer();
	// キー読み込み
	switch (iReadButton()) {
	case -1:
		// 有効なキー入力なし
		break;
	case BTN_NO_0:
		if (sAppEventMap.eEvtBtn_0 != 0x00) {
			iEntrySeqEvt(sAppEventMap.eEvtBtn_0);
		}
		break;
	case BTN_NO_1:
		if (sAppEventMap.eEvtBtn_1 != 0x00) {
			iEntrySeqEvt(sAppEventMap.eEvtBtn_1);
		}
		break;
	case BTN_NO_2:
		if (sAppEventMap.eEvtBtn_2 != 0x00) {
			iEntrySeqEvt(sAppEventMap.eEvtBtn_2);
		}
		break;
	case BTN_NO_3:
		if (sAppEventMap.eEvtBtn_3 != 0x00) {
			iEntrySeqEvt(sAppEventMap.eEvtBtn_3);
		}
		break;
	}
}

/*******************************************************************************
 *
 * NAME: vEvent_SelDev_init
 *
 * DESCRIPTION:リモートデバイス選択の初期処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
PUBLIC void vEvent_SelDev_init(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 初期処理
	//==========================================================================
	// リモートデバイス情報初期化
	sAppTxRxTrns.iRemoteInfoIdx = -1;
	// イベントマップの初期化
	sAppEventMap.eEvtBtn_0    = E_EVENT_SCR_SEL_DEV_CHG;
	sAppEventMap.eEvtBtn_1    = E_EVENT_SCR_SEL_CMD_INIT;	// コマンド選択へ
	sAppEventMap.eEvtBtn_2    = E_EVENT_SCR_SEL_DEV_INIT;	// リモートデバイス選択へ
	sAppEventMap.eEvtBtn_3    = E_EVENT_SCR_SEL_DEV_CHG;
	sAppEventMap.eEvtComplete = 0x00;
	sAppEventMap.eEvtTimeout  = 0x00;
	// リモートデバイス選択
	iEntrySeqEvt(E_EVENT_SCR_SEL_DEV_CHG);
}

/*******************************************************************************
 *
 * NAME: vEvent_SelDev_chg
 *
 * DESCRIPTION:リモートデバイス選択
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
PUBLIC void vEvent_SelDev_chg(uint32 u32EvtTimeMs) {
	//==========================================================================
	// リモートデバイス選択
	//==========================================================================
	bool_t bForwardFlg = TRUE;
	if (sAppTxRxTrns.iRemoteInfoIdx < 0) {
		// 初回および前回エラー時
		sAppTxRxTrns.iRemoteInfoIdx = 0;
	} else {
		// キー判定
		if (sAppIO.iButtonNo == BTN_NO_0) {
			sAppTxRxTrns.iRemoteInfoIdx = (sAppTxRxTrns.iRemoteInfoIdx + 1) % 32;
		} else {
			bForwardFlg = FALSE;
			sAppTxRxTrns.iRemoteInfoIdx = 31;
		}
	}
	// リモートデバイス情報
	sAppTxRxTrns.iRemoteInfoIdx =
			iEEPROMReadRemoteInfo(&sAppTxRxTrns.sRemoteInfo, sAppTxRxTrns.iRemoteInfoIdx, bForwardFlg);
	if (sAppTxRxTrns.iRemoteInfoIdx == -1) {
		vLCDdrawing("I2C Err!", "IdxInfo ");
		// リトライ
		vEvt_InitEvtMap();
		return;
	}
	if (sAppTxRxTrns.iRemoteInfoIdx == -2) {
		vLCDdrawing("RmtInfo ", "NotFound");
		// リトライ
		vEvt_InitEvtMap();
		return;
	}
	if (sAppTxRxTrns.iRemoteInfoIdx < 0) {
		vLCDdrawing("I2C Err!", "RmtInfo ");
		// リトライ
		vEvt_InitEvtMap();
		return;
	}
	//==========================================================================
	// 画面編集
	//==========================================================================
	char cMsgLine0[LCD_BUFF_COL_SIZE + 1];
	char cMsgLine1[LCD_BUFF_COL_SIZE + 1];
	sprintf(cMsgLine0, "%08u", (int)(sAppTxRxTrns.sRemoteInfo.u32DeviceID % 100000000));
	memcpy(cMsgLine1, sAppTxRxTrns.sRemoteInfo.cDeviceName, 8);
	cMsgLine0[8] = '\0';
	cMsgLine1[8] = '\0';
	// メッセージ表示
	vLCDdrawing(cMsgLine0, cMsgLine1);
}

/*******************************************************************************
 *
 * NAME: vEvent_SelCmd_init
 *
 * DESCRIPTION:コマンド選択の初期処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
PUBLIC void vEvent_SelCmd_init(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 初期処理
	//==========================================================================
	// コマンド初期化
	sAppTxRxTrns.eCommand = E_APP_CMD_ALERT;
	// イベントマップの初期化
	sAppEventMap.eEvtBtn_0    = E_EVENT_SCR_SEL_CMD_CHG;
	sAppEventMap.eEvtBtn_1    = E_EVENT_EXEC_CMD_0;			// 通常コマンド実行へ
	sAppEventMap.eEvtBtn_2    = E_EVENT_SCR_SEL_DEV_INIT;	// リモートデバイス選択へ
	sAppEventMap.eEvtBtn_3    = E_EVENT_SCR_SEL_CMD_CHG;
	sAppEventMap.eEvtComplete = 0x00;
	sAppEventMap.eEvtTimeout  = 0x00;
	// コマンド選択
	iEntrySeqEvt(E_EVENT_SCR_SEL_CMD_CHG);
}

/*******************************************************************************
 *
 * NAME: vEvent_SelCmd_init
 *
 * DESCRIPTION:コマンド選択
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
PUBLIC void vEvent_SelCmd_chg(uint32 u32EvtTimeMs) {
	//==========================================================================
	// コマンドとイベントマップ更新
	//==========================================================================
	// コマンドリスト
	teAppCommand eCommands[] =
		{E_APP_CMD_CHK_STATUS, E_APP_CMD_LOCK, E_APP_CMD_UNLOCK, E_APP_CMD_ALERT};
	// コマンドインデックス探索
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < 4 && sAppTxRxTrns.eCommand != eCommands[u8Idx]; u8Idx++);
	// キー判定
	uint8 u8NextIdx;
	if (sAppIO.iButtonNo == BTN_NO_3) {
		u8NextIdx = (u8Idx + 3) % 4;
	} else {
		u8NextIdx = (u8Idx + 1) % 4;
	}
	// コマンド更新
	sAppTxRxTrns.eCommand = eCommands[u8NextIdx];
	// 選択ボタンのイベント更新
	teAppEvent eAppEvent[] =
		{E_EVENT_EXEC_CMD_0, E_EVENT_EXEC_AUTH_CMD_0, E_EVENT_EXEC_AUTH_CMD_0, E_EVENT_EXEC_AUTH_CMD_0};
	sAppEventMap.eEvtBtn_1 = eAppEvent[u8NextIdx];

	//==========================================================================
	// 画面表示処理
	//==========================================================================
	char cMsgList[4][9] = {"Status  ", "Lock    ", "Unlock  ", "Alert   "};
	// メッセージ表示
	vLCDdrawing("Command:", cMsgList[u8NextIdx]);
}

/*******************************************************************************
 *
 * NAME: vEvent_Exec_Cmd_0
 *
 * DESCRIPTION:通常コマンド送信
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
PUBLIC void vEvent_Exec_Cmd_0(uint32 u32EvtTimeMs) {
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d vEvent_Exec_Cmd_0\n", u32TickCount_ms);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	//==========================================================================
	// 初期処理
	//==========================================================================
	sAppEventMap.eEvtBtn_0    = 0x00;
	sAppEventMap.eEvtBtn_1    = 0x00;
	sAppEventMap.eEvtBtn_2    = 0x00;
	sAppEventMap.eEvtBtn_3    = 0x00;
	sAppEventMap.eEvtComplete = E_EVENT_EXEC_CMD_1;
	sAppEventMap.eEvtTimeout  = E_EVENT_RX_TIMEOUT;

	//==========================================================================
	// ステータス要求コマンド送信処理
	//==========================================================================
	// 宛先アドレス
	tsWirelessMsg sWlsMsg;
	memset(&sWlsMsg, 0x00, sizeof(tsWirelessMsg));
	sWirelessInfo.u32TgtAddr = sAppTxRxTrns.sRemoteInfo.u32DeviceID;
	sWlsMsg.u32DstAddr = sAppTxRxTrns.sRemoteInfo.u32DeviceID;
	// コマンド
	sWlsMsg.u8Command = (uint8)sAppTxRxTrns.eCommand;
	// CRC8編集
	sWlsMsg.u8CRC = u8CCITT8((uint8*)&sWlsMsg, sizeof(tsWirelessMsg));
	// 送信
	if (bWirelessTx(sDevInfo.u32DeviceID, FALSE, (uint8*)&sWlsMsg, sizeof(tsWirelessMsg)) == FALSE) {
		// メッセージ表示
		vLCDdrawing("Cmd Err ", "Transmit");
		iEEPROMWriteLog(E_MSG_CD_TX_ERR, &sWlsMsg);
		return;
	}
	// 受信待ち設定
	vWirelessRxEnabled(sAppTxRxTrns.sRemoteInfo.u32DeviceID, &sAppTxRxTrns.sRxMsg);
	// タイムアウトタスク登録
	sAppTxRxTrns.iTimeoutEvtID = iEntryScheduleEvt(E_EVENT_RX_TIMEOUT, 0, RX_TIMEOUT_S, FALSE);

	//==========================================================================
	// メッセージ表示
	//==========================================================================
	vLCDdrawing("Exec Cmd", "........");
}

/*******************************************************************************
 *
 * NAME: vEvent_Exec_Cmd_1
 *
 * DESCRIPTION:通常コマンド返信
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_Exec_Cmd_1(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 初期処理
	//==========================================================================
	// タイムアウトタスクキャンセル
	bCancelScheduleEvt(sAppTxRxTrns.iTimeoutEvtID);
	// イベントマップ
	sAppEventMap.eEvtComplete = 0x00;
	sAppEventMap.eEvtTimeout  = 0x00;

	//==========================================================================
	// 受信パケットチェック
	//==========================================================================
	tsAuthRemoteDevInfo* psRmtDevInfo = &sAppTxRxTrns.sRemoteInfo;
	tsWirelessMsg* psWlsMsg = &sAppTxRxTrns.sRxMsg;
	// デフォルトチェック
	if (bEvt_DefaultMsgChk(psWlsMsg, E_APP_CMD_ACK, psRmtDevInfo) == FALSE) {
		// リモートデバイス選択へ
		vEvt_InitEvtMap();
		return;
	}

	//==========================================================================
	// メッセージ表示
	//==========================================================================
	// ステータス情報を表示
	char cMsgLine1[LCD_BUFF_COL_SIZE + 1];
	char cMsgLine0[LCD_BUFF_COL_SIZE + 1];
	sprintf(cMsgLine0, "%04d%02d%02d", psWlsMsg->u16Year % 10000, psWlsMsg->u8Month, psWlsMsg->u8Day);
	sprintf(cMsgLine1, "%02d:%02d %02X", psWlsMsg->u8Hour, psWlsMsg->u8Minute, psWlsMsg->u8StatusMap);
	vLCDdrawing(cMsgLine0, cMsgLine1);
	// イベントマップ初期化
	vEvt_InitEvtMap();
}

/*******************************************************************************
 *
 * NAME: vEvent_Exec_AuthCmd_0
 *
 * DESCRIPTION:認証コマンド（ステップ０）
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_Exec_AuthCmd_0(uint32 u32EvtTimeMs) {
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d vEvent_Exec_AuthCmd_0\n", u32TickCount_ms);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	//==========================================================================
	// 初期処理
	//==========================================================================
	// イベントマップ設定
	sAppEventMap.eEvtBtn_0    = 0x00;
	sAppEventMap.eEvtBtn_1    = 0x00;
	sAppEventMap.eEvtBtn_2    = 0x00;
	sAppEventMap.eEvtBtn_3    = 0x00;
	sAppEventMap.eEvtComplete = E_EVENT_EXEC_AUTH_CMD_1;
	sAppEventMap.eEvtTimeout  = E_EVENT_RX_TIMEOUT;

	//==========================================================================
	// ステータス要求コマンド送信処理
	//==========================================================================
	// 宛先アドレス
	tsWirelessMsg sWlsMsg;
	memset(&sWlsMsg, 0x00, sizeof(tsWirelessMsg));
	sWirelessInfo.u32TgtAddr = sAppTxRxTrns.sRemoteInfo.u32DeviceID;
	sWlsMsg.u32DstAddr = sAppTxRxTrns.sRemoteInfo.u32DeviceID;
	// コマンド
	sWlsMsg.u8Command = (uint8)E_APP_CMD_CHK_STATUS;
	// CRC8編集
	sWlsMsg.u8CRC = u8CCITT8((uint8*)&sWlsMsg, sizeof(tsWirelessMsg));
	// 送信
	if (bWirelessTx(sDevInfo.u32DeviceID, FALSE, (uint8*)&sWlsMsg, sizeof(tsWirelessMsg)) == FALSE) {
		// メッセージ表示
		vLCDdrawing("Cmd Err ", "Transmit");
		iEEPROMWriteLog(E_MSG_CD_TX_ERR, &sWlsMsg);
		// リモートデバイス選択へ
		vEvt_InitEvtMap();
		return;
	}
	// 受信待ち設定
	vWirelessRxEnabled(sAppTxRxTrns.sRemoteInfo.u32DeviceID, &sAppTxRxTrns.sRxMsg);
	// タイムアウトタスク登録
	sAppTxRxTrns.iTimeoutEvtID = iEntryScheduleEvt(E_EVENT_RX_TIMEOUT, 0, RX_TIMEOUT_S, FALSE);

	//==========================================================================
	// メッセージ表示
	//==========================================================================
	vLCDdrawing("Exec Cmd", "........");
}

/*******************************************************************************
 *
 * NAME: vEvent_Exec_AuthCmd_1
 *
 * DESCRIPTION:認証コマンド（ステップ１）
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_Exec_AuthCmd_1(uint32 u32EvtTimeMs) {
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d vEvent_Exec_AuthCmd_1\n", u32TickCount_ms);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	//==========================================================================
	// 初期処理
	//==========================================================================
	// タイムアウトタスクキャンセル
	bCancelScheduleEvt(sAppTxRxTrns.iTimeoutEvtID);
	// イベントマップ設定
	sAppEventMap.eEvtComplete = E_EVENT_EXEC_AUTH_CMD_2;
	sAppEventMap.eEvtTimeout  = 0x00;

	//==========================================================================
	// 受信データチェック
	//==========================================================================
	tsAuthRemoteDevInfo* psRmtDevInfo = &sAppTxRxTrns.sRemoteInfo;
	tsWirelessMsg* psWlsMsg = &sAppTxRxTrns.sRxMsg;
	// デフォルトチェック
	if (bEvt_DefaultMsgChk(psWlsMsg, E_APP_CMD_ACK, psRmtDevInfo) == FALSE) {
		// リモートデバイス選択へ
		vEvt_InitEvtMap();
		return;
	}
	// 基準日時を算出
	uint32 u32WkRefMin =
			u32ValUtil_dateToDays(psWlsMsg->u16Year, psWlsMsg->u8Month, psWlsMsg->u8Day) * 1440;
	u32WkRefMin = u32WkRefMin + psWlsMsg->u8Hour * 60;
	u32WkRefMin = u32WkRefMin + psWlsMsg->u8Minute;
	// 基準日時をチェック
	if (u32WkRefMin < psRmtDevInfo->u32StartDateTime) {
		vLCDdrawing("Cmd Err!", "RespTime");
		iEEPROMWriteLog(E_MSG_CD_RX_DATETIME_ERR, psWlsMsg);
		// リモートデバイス選択へ
		vEvt_InitEvtMap();
		return;
	}
	// 秒数判定
	if (psWlsMsg->u8Second < 59) {
		// 基準時刻保持
		sAppTxRxTrns.u32RefMin  = u32WkRefMin;
	} else {
		// 59秒以降の場合には１秒ウェイトするので加算
		// 基準時刻保持
		sAppTxRxTrns.u32RefMin  = u32WkRefMin + 1;
	}

	//==========================================================================
	// ワンタイムトークン生成
	//==========================================================================
	// ワンタイム乱数生成
	sAppTxRxTrns.u32OneTimeVal = u32ValUtil_getRandVal();
	// 同期トークンとワンタイム乱数を元にハッシュ関数を利用してワンタイムトークンを生成
	sHashGenInfo = sAuth_generateHashInfo(psRmtDevInfo->u8SyncToken, (sAppTxRxTrns.u32OneTimeVal % 240) + 15);
	sHashGenInfo.u32ShufflePtn = sAppTxRxTrns.u32OneTimeVal;
	// ハッシュ化処理をバックグラウンドプロセスとして起動
	iEntrySeqEvt(E_EVENT_HASH_ST);
}

/*******************************************************************************
 *
 * NAME: vEvent_Exec_AuthCmd_2
 *
 * DESCRIPTION:認証コマンド（ステップ２）
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_Exec_AuthCmd_2(uint32 u32EvtTimeMs) {
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d vEvent_Exec_AuthCmd_2\n", u32TickCount_ms);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	// イベントマップ設定
	sAppEventMap.eEvtComplete = 0x00;
	// ワンタイムトークンの編集
	memcpy(sAppTxRxTrns.u8OneTimeTkn, sHashGenInfo.u8HashCode, APP_AUTH_TOKEN_SIZE);
	memset(&sHashGenInfo, 0x00, sizeof(tsAuthHashGenState));
	// 次のステップを実行
	if (sAppTxRxTrns.sRxMsg.u8Second < 59) {
		// 次処理を遅延実行（ダブルタップ返信により誤受信しない為）
		iEntryScheduleEvt(E_EVENT_EXEC_AUTH_CMD_3, 0, 100, FALSE);
	} else {
		// 次処理を1秒後に実行
		iEntryScheduleEvt(E_EVENT_EXEC_AUTH_CMD_3, 0, 1000, FALSE);
	}
}

/*******************************************************************************
 *
 * NAME: vEvent_Exec_AuthCmd_3
 *
 * DESCRIPTION:認証コマンド（ステップ３）
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_Exec_AuthCmd_3(uint32 u32EvtTimeMs) {
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d vEvent_Exec_AuthCmd_3\n", u32TickCount_ms);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	//==========================================================================
	// 初期処理
	//==========================================================================
	// イベントマップ設定
	sAppEventMap.eEvtComplete = E_EVENT_EXEC_AUTH_CMD_4;
	sAppEventMap.eEvtTimeout  = E_EVENT_RX_TIMEOUT;

	//==========================================================================
	// 認証要求コマンド送信処理
	//==========================================================================
	// 電文編集
	tsAuthRemoteDevInfo* psRmtDevInfo = &sAppTxRxTrns.sRemoteInfo;
	tsWirelessMsg* psWlsMsg = &sAppTxRxTrns.sTxMsg;
	memset(psWlsMsg, 0x00, sizeof(tsWirelessMsg));
	psWlsMsg->u32DstAddr = psRmtDevInfo->u32DeviceID;		// 宛先アドレス
	psWlsMsg->u8Command  = (uint8)sAppTxRxTrns.eCommand;	// コマンド
	psWlsMsg->u32SyncVal = sAppTxRxTrns.u32OneTimeVal;		// ワンタイム乱数
	// ストレッチング回数（ワンタイムトークンでマスク）
	psWlsMsg->u8AuthStCnt =
		u8ValUtil_masking(psRmtDevInfo->u8SndStretching, sAppTxRxTrns.u8OneTimeTkn, APP_AUTH_TOKEN_SIZE);
	// 認証トークンに認証ハッシュを編集
	memcpy(psWlsMsg->u8AuthToken, psRmtDevInfo->u8AuthHash, APP_AUTH_TOKEN_SIZE);
	// ワンタイムトークンでマスキング
	vValUtil_masking(psWlsMsg->u8AuthToken, sAppTxRxTrns.u8OneTimeTkn, APP_AUTH_TOKEN_SIZE);
	// CRC8編集
	psWlsMsg->u8CRC = u8CCITT8((uint8*)psWlsMsg, sizeof(tsWirelessMsg));
	// 送信
	if (bWirelessTx(sDevInfo.u32DeviceID, FALSE, (uint8*)psWlsMsg, sizeof(tsWirelessMsg)) == FALSE) {
		vLCDdrawing("Cmd Err!", "Transmit");
		iEEPROMWriteLog(E_MSG_CD_TX_AUTH_CMD_ERR, psWlsMsg);
		// リモートデバイス選択へ
		vEvt_InitEvtMap();
		return;
	}
	// 受信待ち設定
	vWirelessRxEnabled(sAppTxRxTrns.sRemoteInfo.u32DeviceID, &sAppTxRxTrns.sRxMsg);
	// タイムアウトタスク登録
	sAppTxRxTrns.iTimeoutEvtID = iEntryScheduleEvt(E_EVENT_RX_TIMEOUT, 0, RX_TIMEOUT_L, FALSE);
}

/*******************************************************************************
 *
 * NAME: vEvent_Exec_AuthCmd_4
 *
 * DESCRIPTION:認証コマンド（ステップ４）
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_Exec_AuthCmd_4(uint32 u32EvtTimeMs) {
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d vEvent_Exec_AuthCmd_4\n", u32TickCount_ms);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	//==========================================================================
	// 初期処理
	//==========================================================================
	// タイムアウトタスクキャンセル
	bCancelScheduleEvt(sAppTxRxTrns.iTimeoutEvtID);
	// イベントマップ設定
	sAppEventMap.eEvtComplete = E_EVENT_EXEC_AUTH_CMD_5;
	sAppEventMap.eEvtTimeout  = 0x00;

	//==========================================================================
	// レスポンスデータチェック
	//==========================================================================
	// 基本的なチェックを実施
	tsAuthRemoteDevInfo* psRmtDevInfo = &sAppTxRxTrns.sRemoteInfo;
	tsWirelessMsg* psWlsMsg = &sAppTxRxTrns.sRxMsg;
	// デフォルトチェック
	if (bEvt_DefaultMsgChk(psWlsMsg, E_APP_CMD_AUTH_ACK, psRmtDevInfo) == FALSE) {
		// リモートデバイス選択へ
		vEvt_InitEvtMap();
		return;
	}
	// 基準点までの経過時間を算出
	uint32 u32WkRefMin =
			u32ValUtil_dateToDays(psWlsMsg->u16Year, psWlsMsg->u8Month, psWlsMsg->u8Day) * 1440;
	u32WkRefMin = u32WkRefMin + psWlsMsg->u8Hour * 60;
	u32WkRefMin = u32WkRefMin + psWlsMsg->u8Minute;
	// 前回基準日時と比較チェック
	if (u32WkRefMin < sAppTxRxTrns.u32RefMin || u32WkRefMin > (sAppTxRxTrns.u32RefMin + 1)) {
		vLCDdrawing("Cmd Err!", "RespTime");
		iEEPROMWriteLog(E_MSG_CD_RX_DATETIME_ERR, psWlsMsg);
		// リモートデバイス選択へ
		vEvt_InitEvtMap();
		return;
	}

	//==========================================================================
	// 受信情報を復元
	//==========================================================================
	// 認証ストレッチング回数を復元
	psWlsMsg->u8AuthStCnt =
		u8ValUtil_masking(psWlsMsg->u8AuthStCnt, sAppTxRxTrns.u8OneTimeTkn, APP_AUTH_TOKEN_SIZE);
	// 認証トークンを復元
	vValUtil_masking(psWlsMsg->u8AuthToken, sAppTxRxTrns.u8OneTimeTkn, APP_AUTH_TOKEN_SIZE);
	// 更新ストレッチング回数を復元
	psWlsMsg->u8UpdAuthStCnt =
		u8ValUtil_masking(psWlsMsg->u8UpdAuthStCnt, sAppTxRxTrns.u8OneTimeTkn, APP_AUTH_TOKEN_SIZE);
	// 更新トークンマップを復元
	vValUtil_masking(psWlsMsg->u8UpdAuthToken, sAppTxRxTrns.u8OneTimeTkn, APP_AUTH_TOKEN_SIZE);

	//==========================================================================
	// 認証情報をチェック
	//==========================================================================
	// ストレッチング回数チェック
	if (psWlsMsg->u8AuthStCnt == 0 || psWlsMsg->u8UpdAuthStCnt == 0) {
		iEEPROMWriteLog(E_MSG_CD_RX_ST_CNT_ERR, psWlsMsg);
		// リモートデバイス選択へ
		vEvt_InitEvtMap();
		return;
	}
	// ストレッチング回数算出
	uint16 u16StCnt = psRmtDevInfo->u8SndStretching + psWlsMsg->u8AuthStCnt;
	u16StCnt = u16StCnt + ((sAppTxRxTrns.u32RefMin - psRmtDevInfo->u32StartDateTime) % STRETCHING_CNT_BASE);
	// ハッシュ生成情報
	sHashGenInfo = sAuth_generateHashInfo(psWlsMsg->u8AuthToken, u16StCnt);
	vAuth_setSyncToken(&sHashGenInfo, psRmtDevInfo->u8SyncToken);
	// ハッシュ化処理をバックグラウンドプロセスとして起動
	iEntrySeqEvt(E_EVENT_HASH_ST);
}

/*******************************************************************************
 *
 * NAME: vEvent_Exec_AuthCmd_5
 *
 * DESCRIPTION:認証コマンド（ステップ５）
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_Exec_AuthCmd_5(uint32 u32EvtTimeMs) {
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d vEvent_Exec_AuthCmd_5\n", u32TickCount_ms);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	//==========================================================================
	// 初期処理
	//==========================================================================
	// イベントマップ設定
	sAppEventMap.eEvtComplete = 0x00;
	sAppEventMap.eEvtTimeout  = 0x00;

	//==========================================================================
	// 認証処理
	//==========================================================================
	// 認証ハッシュ値の検証
	tsAuthRemoteDevInfo* psRmtDevInfo = &sAppTxRxTrns.sRemoteInfo;
	tsWirelessMsg* psWlsMsg = &sAppTxRxTrns.sRxMsg;
	int iResult = memcmp(psRmtDevInfo->u8AuthHash, sHashGenInfo.u8HashCode, APP_AUTH_TOKEN_SIZE);
	memset(&sHashGenInfo, 0x00, sizeof(tsAuthHashGenState));
	if (iResult != 0) {
		// エラーメッセージ表示
		vLCDdrawing("Cmd Err!", "Auth Err ");
		iEEPROMWriteLog(E_MSG_CD_RX_AUTH_TKN_ERR, psWlsMsg);
		// リモートデバイス選択へ
		vEvt_InitEvtMap();
		return;
	}
	// 認証情報を更新
	psRmtDevInfo->u8SndStretching = psWlsMsg->u8UpdAuthStCnt;
	memcpy(psRmtDevInfo->u8AuthHash, psWlsMsg->u8UpdAuthToken, APP_AUTH_TOKEN_SIZE);
	vValUtil_masking(psRmtDevInfo->u8SyncToken, sAppTxRxTrns.u8OneTimeTkn, APP_AUTH_TOKEN_SIZE);
	// 認証情報を書き込み
	if (iEEPROMWriteRemoteInfo(psRmtDevInfo) < 0) {
		// エラーメッセージ表示
		vLCDdrawing("Cmd Err!", "Auth Upd ");
		iEEPROMWriteLog(E_MSG_CD_WRITE_RMT_DEV_ERR, psWlsMsg);
		// リモートデバイス選択へ
		vEvt_InitEvtMap();
		return;
	}

	//==========================================================================
	// メッセージ表示
	//==========================================================================
	// ステータス情報を表示
	char cMsgLine0[LCD_BUFF_COL_SIZE + 1];
	char cMsgLine1[LCD_BUFF_COL_SIZE + 1];
	sprintf(cMsgLine0, "%04d%02d%02d", (int)psWlsMsg->u16Year, (int)psWlsMsg->u8Month, (int)psWlsMsg->u8Day);
	sprintf(cMsgLine1, "%02d:%02d %02X", (int)psWlsMsg->u8Hour, (int)psWlsMsg->u8Minute,
			(int)u32ValUtil_u8ToBinary(psWlsMsg->u8StatusMap));
	vLCDdrawing(cMsgLine0, cMsgLine1);
	// イベントマップの初期化
	vEvt_InitEvtMap();
}

/*******************************************************************************
 *
 * NAME: vEvent_RxTimeout
 *
 * DESCRIPTION:イベント処理：無線パケット受信タイムアウト
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_RxTimeout(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 初期処理
	//==========================================================================
	// 受信待ち無効化
	vWirelessRxDisabled(sDevInfo.u32DeviceID);
	// タイムアウトタスクを初期化
	sAppTxRxTrns.iTimeoutEvtID = -1;
	// イベントマップの初期化
	vEvt_InitEvtMap();

	//==========================================================================
	// 画面編集
	//==========================================================================
	// メッセージ表示
	vLCDdrawing("Message:", "Timeout ");
}

/*******************************************************************************
 *
 * NAME: vEvent_HashStretching
 *
 * DESCRIPTION:拡張ハッシュストレッチング処理イベントプロセス
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_HashStretching(uint32 u32EvtTimeMs) {
	// ハッシュ関数実行
	if (bAuth_hashStretching(&sHashGenInfo)) {
		// ハッシュ生成完了時には元のイベントに戻る
		iEntrySeqEvt(sAppEventMap.eEvtComplete);
	} else {
		// 次回ストレッチング処理
		iEntrySeqEvt(E_EVENT_HASH_ST);
	}
}

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: vEvt_InitEvtMap
 *
 * DESCRIPTION:イベントマップ初期化処理
 *
 * PARAMETERS:        Name            RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PRIVATE void vEvt_InitEvtMap() {
	sAppEventMap.eEvtBtn_0    = E_EVENT_SCR_SEL_DEV_INIT;
	sAppEventMap.eEvtBtn_1    = E_EVENT_SCR_SEL_DEV_INIT;
	sAppEventMap.eEvtBtn_2    = E_EVENT_SCR_SEL_DEV_INIT;
	sAppEventMap.eEvtBtn_3    = E_EVENT_SCR_SEL_DEV_INIT;
	sAppEventMap.eEvtComplete = 0x00;
	sAppEventMap.eEvtTimeout  = 0x00;
}

/*******************************************************************************
 *
 * NAME: bEvt_DefaultMsgChk
 *
 * DESCRIPTION:受信メッセージ基本チェック
 *
 * PARAMETERS:          Name            RW  Usage
 *   tsWirelessMsg*     psWlsMsg        R   受信メッセージ
 *   teAppCommand       eCmd            R   返信コマンド
 *
 * RETURNS:
 *   True:チェックOK
 *
 ******************************************************************************/
PRIVATE bool_t bEvt_DefaultMsgChk(tsWirelessMsg* psWlsMsg, teAppCommand eCmd, tsAuthRemoteDevInfo* psRmtDevInfo) {
	//----------------------------------------------------------------------
	// レスポンスデータチェック
	//----------------------------------------------------------------------
	// CRCチェック
	uint8 u8WkCRC = psWlsMsg->u8CRC;
	psWlsMsg->u8CRC = 0;
	if (u8CCITT8((uint8*)psWlsMsg, sizeof(tsWirelessMsg)) != u8WkCRC) {
		vLCDdrawing("ExecErr ", "CRC Chk ");
		iEEPROMWriteLog(E_MSG_CD_RX_CRC_ERR, psWlsMsg);
		return FALSE;
	}
	// コマンドチェック
	if (psWlsMsg->u8Command != eCmd) {
		// ステータス情報を表示
		char cMsg[LCD_BUFF_COL_SIZE + 1];
		sprintf(cMsg, "RxCmd %02X", psWlsMsg->u8Command);
		vLCDdrawing("ExecErr ", cMsg);
		iEEPROMWriteLog(E_MSG_CD_RX_CMD_ERR, psWlsMsg);
		return FALSE;
	}
	// 日付チェック
	if (bValUtil_validDate(psWlsMsg->u16Year, psWlsMsg->u8Month, psWlsMsg->u8Day) == FALSE) {
		vLCDdrawing("ExecErr ", "Rx Date ");
		iEEPROMWriteLog(E_MSG_CD_RX_DATE_ERR, psWlsMsg);
		return FALSE;
	}
	// 時刻チェック
	if (bValUtil_validTime(psWlsMsg->u8Hour, psWlsMsg->u8Minute, psWlsMsg->u8Second) == FALSE) {
		vLCDdrawing("ExecErr ", "Rx Time ");
		iEEPROMWriteLog(E_MSG_CD_RX_TIME_ERR, psWlsMsg);
		return FALSE;
	}
	// ステータス判定
	if (psWlsMsg->u8StatusMap != psRmtDevInfo->u8StatusMap) {
		psRmtDevInfo->u8StatusMap = psWlsMsg->u8StatusMap;
		iEEPROMWriteRemoteInfo(psRmtDevInfo);
		iEEPROMWriteLog(E_MSG_CD_RX_STS_ERR, psWlsMsg);
	}
	// チェックOK
	return TRUE;
}

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
