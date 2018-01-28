/*******************************************************************************
 *
 * MODULE :Application Process source file
 *
 * CREATED:2016/07/10 12:42:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:各画面プロセス毎の処理を実装
 *
 * CHANGE HISTORY:
 * 2018/01/27 23:19:00 認証時の通信データをAES暗号化
 *
 * LAST MODIFIED BY:
 *
 *******************************************************************************
 * Copyright (c) 2016, Nakanohito
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
#include "aes.h"
#include "sha256.h"
#include "value_util.h"
#include "app_process.h"
#include "app_io.h"
#include "app_auth.h"
#include "app_main.h"

/******************************************************************************/
/***        Macro Definitions                                               ***/
/******************************************************************************/

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/
// Process Change Type
typedef enum {
	E_PROCESS_CHG_PREV,
	E_PROCESS_CHG_REPLACE,
	E_PROCESS_CHG_NEXT
} teAppProcChgType;

// 構造体：アプリケーション情報
typedef struct {
	// 画面レイヤー番号
	uint8 u8ProcessLayer;
} tsAppInfo;

// 構造体：画面パラメータ
typedef struct {
	teAppProcChgType eChgTypeOK;				// プロセス切替タイプ（OKボタン）
	teAppProcess eProcessIdOK;					// プロセスID（OKボタン）
	teAppProcChgType eChgTypeCANCEL;			// プロセス切替タイプ（CANCELボタン）
	teAppProcess eProcessIdCANCEL;				// プロセスID（CANCELボタン）
	char cDispMsg[3][17];						// メッセージ
} tsAppScrParam;

// 構造体：送受信トランザクション情報
typedef struct {
	uint32 u32RefMin;							// 基準時刻
	uint32 u32OneTimeVal;						// ワンタイム乱数
	uint8 u8OneTimeTkn[APP_AUTH_TOKEN_SIZE];	// ワンタイムトークン
	tsWirelessMsg sWlsMsg;						// 送受信メッセージ
} tsAppTxRxTrnsInfo;

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/
// 前画面プロセスへ遷移
PRIVATE void vProc_Prev();
// 他画面プロセスへ遷移
PRIVATE void vProc_Replace(teAppProcess eAppProcess);
// 次画面プロセスへ遷移
PRIVATE void vProc_Next(teAppProcess eAppProcess);
// エラーメッセージ設定
PRIVATE void vProc_SetMessage(char *pcMsg1, char *pcMsg2);
// リモートデバイス選択画面への遷移パラメータ設定
PRIVATE void vProc_SetSelectRemoteDev(teAppProcess eProcIdOk);
// トークン入力画面への遷移パラメータ設定
PRIVATE void vProc_SetInputToken(teAppProcess eProcIdOk, teAppProcess eProcIdCancel, bool_t bPrevFlg);
// 受信メッセージ基本チェック
PRIVATE bool_t bProc_DefaultChkRx(tsWirelessMsg* psWlsMsg, uint8 u8Cmd);

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/
/** アプリケーション情報 */
PUBLIC tsAppInfo sAppInfo;
/** プロセス情報 */
PUBLIC tsAppProcessInfo sProcessInfoList[PROCESS_LAYER_SIZE];
/** 画面遷移用のパラメータ */
PUBLIC tsAppScrParam sAppScrParam;
/** ハッシュ値生成情報 */
PUBLIC tsAuthHashGenState sHashGenInfo;
/** デバイス情報 */
PUBLIC tsAuthDeviceInfo sDevInfo;
/** デバイス情報（編集用） */
PUBLIC tsAuthDeviceInfo sEditDevInfo;
/** 主リモートデバイス情報 */
PUBLIC tsAuthRemoteDevInfo sRemoteInfoMain;
/** 副リモートデバイス情報 */
PUBLIC tsAuthRemoteDevInfo sRemoteInfoSub;
/** 送受信メッセージ */
PUBLIC tsAppTxRxTrnsInfo sTxRxTrnsInfo;
/** 編集日付情報 */
PUBLIC tsDate sEditDate;
/** 編集時刻情報 */
PUBLIC tsTime sEditTime;

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/

/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: vProc_Init
 *
 * DESCRIPTION:アプリケーションプロセス初期処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_Init() {
	// プロセス情報初期化
	sAppInfo.u8ProcessLayer = 0;								// プロセス階層
	sProcessInfoList[0].eProcessID  = E_PROCESS_HOME;			// 現プロセスID
	sProcessInfoList[0].eProcStatus = E_PROCESS_STATE_BEGIN;	// ステータス：開始
	sProcessInfoList[0].u8CursorPosRow = 0;						// カーソル位置（行）
	sProcessInfoList[0].u8CursorPosCol = 0;						// カーソル位置（列）
}

/*******************************************************************************
 *
 * NAME: vEventProcess
 *
 * DESCRIPTION:各画面ごとのプロセス処理
 *     cbToCoNet_vMain関数から呼び出される周期イベント処理
 *     おおよそ4ms毎に実行される（キューイングされた遅延実行と思われる）
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント情報
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
PUBLIC void vEventProcess(uint32 u32EvtTimeMs) {
	// プロセス情報の取得
	tsAppProcessInfo *psProcessInfo = &sProcessInfoList[sAppInfo.u8ProcessLayer];
	// 各画面処理
	switch (psProcessInfo->eProcessID) {
		case E_PROCESS_HOME:			// ホーム画面
			vProc_Home(psProcessInfo);
			break;
		case E_PROCESS_MAIN_MENU:		// メインメニュー
			vProc_MainMenu(psProcessInfo);
			break;
		case E_PROCESS_DEV_CONNECT:		// デバイス接続
			vProc_DevConnect(psProcessInfo);
			break;
		case E_PROCESS_DATE_TIME_MENU:	// 日時設定メニュー
			vProc_DateTimeMenu(psProcessInfo);
			break;
		case E_PROCESS_DATE_SET:		// 日付設定
			vProc_DateSetting(psProcessInfo);
			break;
		case E_PROCESS_WEEK_SET:		// 曜日設定
			vProc_WeekSetting(psProcessInfo);
			break;
		case E_PROCESS_TIME_SET:		// 時刻設定
			vProc_TimeSetting(psProcessInfo);
			break;
		case E_PROCESS_DEV_INFO_MENU:	// デバイス情報メニュー
			vProc_DeviceInfoMenu(psProcessInfo);
			break;
		case E_PROCESS_DEV_INFO_01:		// デバイス情報参照（デバイスID）
			vProc_DispDeviceInfo_01(psProcessInfo);
			break;
		case E_PROCESS_DEV_INFO_02:		// デバイス情報参照（デバイス名）
			vProc_DispDeviceInfo_02(psProcessInfo);
			break;
		case E_PROCESS_DEV_INFO_03:		// デバイス情報参照（マスターパスワードハッシュ）
			vProc_DispDeviceInfo_03(psProcessInfo);
			break;
		case E_PROCESS_DEV_INFO_04:		// デバイス情報参照（マスターパスワードストレッチング回数）
			vProc_DispDeviceInfo_04(psProcessInfo);
			break;
		case E_PROCESS_DEV_INFO_05:		// デバイス情報参照（通信暗号化パスワード）
			vProc_DispDeviceInfo_05(psProcessInfo);
			break;
		case E_PROCESS_DEV_INFO_06:		// デバイス情報参照（ステータス）
			vProc_DispDeviceInfo_06(psProcessInfo);
			break;
		case E_PROCESS_DEV_EDIT_01:		// デバイス情報設定（デバイスID）
			vProc_EditDeviceInfo_01(psProcessInfo);
			break;
		case E_PROCESS_DEV_EDIT_02:		// デバイス情報設定（デバイス名）
			vProc_EditDeviceInfo_02(psProcessInfo);
			break;
		case E_PROCESS_DEV_EDIT_03:		// デバイス情報設定（マスターパスワードストレッチング回数）
			vProc_EditDeviceInfo_03(psProcessInfo);
			break;
		case E_PROCESS_DEV_EDIT_04:		// 通信暗号化パスワード
			vProc_EditDeviceInfo_04(psProcessInfo);
			break;
		case E_PROCESS_DEV_EDIT_05:		// デバイス情報書き込み
			vProc_EditDeviceInfo_05(psProcessInfo);
			break;
		case E_PROCESS_DEV_STS_CLR:		// デバイス情報ステータスクリア
			vProc_ClearDeviceSts(psProcessInfo);
			break;
		case E_PROCESS_REMOTE_DEV_MENU:	// リモートデバイス情報メニュー
			vProc_RemoteDevMenu(psProcessInfo);
			break;
		case E_PROCESS_REMOTE_DEV_INFO:	// リモートデバイス情報表示
			vProc_RemoteDevInfo(psProcessInfo);
			break;
		case E_PROCESS_SYNCHRONIZING:	// リモートデバイス情報同期
			vProc_Synchronizing(psProcessInfo);
			break;
		case E_PROCESS_DEL_REMOTE:		// リモートデバイス情報削除（削除確認）
			vProc_DelRemoteDev(psProcessInfo);
			break;
		case E_PROCESS_DEL_REMOTE_ALL:	// リモートデバイス情報全削除
			vProc_ClearRemoteDev(psProcessInfo);
			break;
		case E_PROCESS_LOG_MENU:		// イベント履歴メニュー
			vProc_EventLogMenu(psProcessInfo);
			break;
		case E_PROCESS_DISPLAY_LOG:		// イベント履歴選択
			vProc_DispEventLog(psProcessInfo);
			break;
		case E_PROCESS_LOG_CLEAR:		// イベント履歴クリア
			vProc_ClearEventLog(psProcessInfo);
			break;
		case E_PROCESS_EXEC_CMD_MENU:	// コマンド実行メニュー
			vProc_ExecCmdMenu(psProcessInfo);
			break;
		case E_PROCESS_EXEC_CMD_STATUS:	// ステータスコマンド実行
			vProc_ExecCmd(psProcessInfo, E_APP_CMD_CHK_STATUS);
			break;
		case E_PROCESS_EXEC_CMD_OPEN:	// 開錠コマンド実行
			vProc_ExecAuthCmd(psProcessInfo, E_APP_CMD_UNLOCK);
			break;
		case E_PROCESS_EXEC_CMD_CLOSE:	// 施錠コマンド実行
			vProc_ExecAuthCmd(psProcessInfo, E_APP_CMD_LOCK);
			break;
		case E_PROCESS_EXEC_CMD_ALERT:	// 警戒コマンド実行
			vProc_ExecAuthCmd(psProcessInfo, E_APP_CMD_ALERT);
			break;
		case E_PROCESS_EXEC_CMD_MPW_OPEN_01:	// マスターパスワード開錠コマンド実行（リダイレクト）
			vProc_ExecMstCmd_01(psProcessInfo);
			break;
		case E_PROCESS_EXEC_CMD_MPW_OPEN_02:	// マスターパスワード開錠コマンド実行（コマンド送信）
			vProc_ExecMstCmd_02(psProcessInfo, E_APP_CMD_MST_UNLOCK);
			break;
		case E_PROCESS_SHOW_MSG:		// メッセージ表示
			vProc_ShowMsg(psProcessInfo);
			break;
		case E_PROCESS_SELECT_REMOTE_DEV:	// リモートデバイス情報の選択
			vProc_SelectRemoteDev(psProcessInfo);
			break;
		case E_PROCESS_INPUT_TOKEN:	// トークン入力
			vProc_InputToken(psProcessInfo);
			break;
		default:
			break;
	}
}

/*******************************************************************************
 *
 * NAME: vProc_Home
 *
 * DESCRIPTION:ホーム画面処理
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_Home(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// LCD情報初期化
			sLCDInfo.u8CurrentDispRow  = 0;	// 表示位置
			psProcInfo->u8CursorPosRow = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;	// カーソル位置（列）
		}
		// 画面制御情報設定
		uint8 u8Col;
		for (u8Col = 0; u8Col < (LCD_BUFF_COL_SIZE / 4); u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
			sLCDInfo.u8LCDCntr[1][u8Col] = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// サブデバイス接続確認
		vI2CSubConnect();
		sAppIO.u8I2CSubType = u8I2CDeviceTypeChk(&sAppIO.sDatetime);
		// メインデバイス接続確認（時刻取得）
		vI2CMainConnect();
		sAppIO.u8I2CMainType = u8I2CDeviceTypeChk(&sAppIO.sDatetime);
#ifdef DEBUG
		vfPrintf(&sSerStream, "vProc_Home No.1 Sub :%03d\n", sAppIO.u8I2CSubType);
		SERIAL_vFlush(sSerStream.u8Device);
		vfPrintf(&sSerStream, "vProc_Home No.2 Main:%03d\n", sAppIO.u8I2CMainType);
		SERIAL_vFlush(sSerStream.u8Device);
#endif
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// プロセス処理
	//==========================================================================
	// コントラストチェック
	if ((u8Key == '2' || u8Key == '3') && sLCDInfo.sLCDstate.u8Contrast < 63) {
		sLCDInfo.sLCDstate.u8Contrast++;
		bLocalEEPROMWrite();
		bST7032i_setContrast(sLCDInfo.sLCDstate.u8Contrast);
	} else if ((u8Key == '8' || u8Key == '9') && sLCDInfo.sLCDstate.u8Contrast > 0) {
		sLCDInfo.sLCDstate.u8Contrast--;
		bLocalEEPROMWrite();
		bST7032i_setContrast(sLCDInfo.sLCDstate.u8Contrast);
	} else if (u8Key == '*' || u8Key == '#') {
		// メインメニューへ遷移
		vProc_Next(E_PROCESS_MAIN_MENU);
		return;
	}

	//==========================================================================
	// LCD描画
	//==========================================================================
	// 画面編集
	char *pcMsg;
	if (sAppIO.u8I2CMainType != 0x00) {
		pcMsg = "Connected ";
	} else {
		pcMsg = "None      ";
	}
	sprintf(sLCDInfo.cLCDBuff[0], "Dev A:%s", pcMsg);
	if (sAppIO.u8I2CSubType != 0x00) {
		pcMsg = "Connected ";
	} else {
		pcMsg = "None      ";
	}
	sprintf(sLCDInfo.cLCDBuff[1], "Dev B:%s", pcMsg);
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_MainMenu
 *
 * DESCRIPTION:メインメニュー
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_MainMenu(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			sLCDInfo.u8CurrentDispRow    = 0;	// 表示位置
			psProcInfo->u8CursorPosRow   = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol   = 0;	// カーソル位置（列）
		}
		// 画面制御情報設定
		uint8 u8Cntr;
		if (sAppIO.u8I2CMainType) {
			u8Cntr = u8LCDTypeToByte(LCD_CTR_BLINK, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		} else {
			u8Cntr = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		}
		uint8 u8CntrNext = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8Cntr;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8Cntr;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8Cntr;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8Cntr;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8Cntr;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8Cntr;
			u8Cntr = u8CntrNext;
		}
		// 画面情報設定
		sprintf(sLCDInfo.cLCDBuff[0], "1:Dev Connect   ");
		if (sAppIO.u8I2CMainType) {
			sprintf(sLCDInfo.cLCDBuff[1], "2:Date Time Menu");
			sprintf(sLCDInfo.cLCDBuff[2], "3:Dev Info Menu ");
			sprintf(sLCDInfo.cLCDBuff[3], "4:RemoteDev Menu");
			sprintf(sLCDInfo.cLCDBuff[4], "5:Event Log Menu");
			sprintf(sLCDInfo.cLCDBuff[5], "6:Exec Cmd Menu ");
		} else {
			sprintf(sLCDInfo.cLCDBuff[1], "                ");
			sprintf(sLCDInfo.cLCDBuff[2], "                ");
			sprintf(sLCDInfo.cLCDBuff[3], "                ");
			sprintf(sLCDInfo.cLCDBuff[4], "                ");
			sprintf(sLCDInfo.cLCDBuff[5], "                ");
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// プロセス処理
	//==========================================================================
	// メニュー選択
	if (u8Key == '*') {
		// 画面遷移先判定
		teAppProcess eAppProcess = E_PROCESS_MAIN_MENU;
		switch (psProcInfo->u8CursorPosRow) {
			case 0:
				eAppProcess = E_PROCESS_DEV_CONNECT;
				break;
			case 1:
				eAppProcess = E_PROCESS_DATE_TIME_MENU;
				break;
			case 2:
				eAppProcess = E_PROCESS_DEV_INFO_MENU;
				break;
			case 3:
				eAppProcess = E_PROCESS_REMOTE_DEV_MENU;
				break;
			case 4:
				eAppProcess = E_PROCESS_LOG_MENU;
				break;
			case 5:
				eAppProcess = E_PROCESS_EXEC_CMD_MENU;
				break;
		}
		vProc_Next(eAppProcess);
		return;
	} else if (u8Key == '#') {
		// ホーム画面へ遷移
		vProc_Prev();
		return;
	} else if (u8Key != (uint8)NULL) {
		// カーソル移動
		bLCDCursorKeyMove(u8Key);
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
	}

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_DevConnect
 *
 * DESCRIPTION:デバイス接続
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_DevConnect(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			sLCDInfo.u8CurrentDispRow    = 0;	// 表示位置
			psProcInfo->u8CursorPosRow   = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol   = 0;	// カーソル位置（列）
		}
		// 画面制御情報設定
		uint8 u8Col;
		uint8 u8Cntr = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8Cntr;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8Cntr;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// サブデバイス接続確認
		vI2CSubConnect();
		sAppIO.u8I2CSubType = u8I2CDeviceTypeChk(&sAppIO.sDatetime);
		// メインデバイス接続確認（時刻取得）
		vI2CMainConnect();
		sAppIO.u8I2CMainType = u8I2CDeviceTypeChk(&sAppIO.sDatetime);
#ifdef DEBUG
		vfPrintf(&sSerStream, "vProc_DevConnect No.1 Sub :%03d\n", sAppIO.u8I2CSubType);
		SERIAL_vFlush(sSerStream.u8Device);
		vfPrintf(&sSerStream, "vProc_DevConnect No.2 Main:%03d\n", sAppIO.u8I2CMainType);
		SERIAL_vFlush(sSerStream.u8Device);
#endif
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// プロセス処理
	//==========================================================================
	// 画面遷移判定
	if (u8Key == '*' || u8Key == '#') {
		// メインメニューへ戻る
		vProc_Prev();
		return;
	}

	//==========================================================================
	// LCD描画
	//==========================================================================
	// 画面編集
	char *pcMsg;
	if ((sAppIO.u8I2CMainType & I2C_DEVICE_EEPROM) != 0x00) {
		pcMsg = "Connected ";
	} else {
		pcMsg = "None      ";
	}
	sprintf(sLCDInfo.cLCDBuff[0], "Dev A:%s", pcMsg);
	if ((sAppIO.u8I2CSubType & I2C_DEVICE_EEPROM) != 0x00) {
		pcMsg = "Connected ";
	} else {
		pcMsg = "None      ";
	}
	sprintf(sLCDInfo.cLCDBuff[1], "Dev B:%s", pcMsg);
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_DateTimeMenu
 *
 * DESCRIPTION:日時設定メニュー
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_DateTimeMenu(tsAppProcessInfo *psProcInfo) {
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 0;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;		// カーソル位置（列）
		}
		// 画面制御情報設定
		uint8 u8Col;
		uint8 u8CntrDisp = u8LCDTypeToByte(LCD_CTR_BLINK, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
			u8CntrDisp = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// プロセス処理
	//==========================================================================
	// メニュー選択
	if (u8Key == '*') {
		// 画面遷移先判定
		teAppProcess eAppProcess = E_PROCESS_MAIN_MENU;
		switch (psProcInfo->u8CursorPosRow) {
			case 0:
				eAppProcess = E_PROCESS_DATE_SET;
				break;
			case 1:
				eAppProcess = E_PROCESS_WEEK_SET;
				break;
			case 2:
				eAppProcess = E_PROCESS_TIME_SET;
				break;
		}
		vProc_Next(eAppProcess);
		return;
	} else if (u8Key == '#') {
		// ホーム画面へ遷移
		vProc_Prev();
		return;
	} else if (u8Key != (uint8)NULL) {
		// カーソル移動
		bLCDCursorKeyMove(u8Key);
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
	}

	//==========================================================================
	// LCD描画
	//==========================================================================
	// 画面表示内容設定
	sprintf(sLCDInfo.cLCDBuff[0], "1:Date Setting  ");
	sprintf(sLCDInfo.cLCDBuff[1], "2:Week Setting  ");
	sprintf(sLCDInfo.cLCDBuff[2], "3:Time Setting  ");
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_DateSetting
 *
 * DESCRIPTION:日付設定
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_DateSetting(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow    = 0;
			psProcInfo->u8CursorPosRow   = 1;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol   = 5;	// カーソル位置（列）
		}
		// 日付設定
		psProcInfo->u32Param_0 = u32TickCount_ms;
		sEditDate.u16Year = sAppIO.sDatetime.u16Year;
		sEditDate.u8Month = sAppIO.sDatetime.u8Month;
		sEditDate.u8Day   = sAppIO.sDatetime.u8Day;
		// 画面制御情報初期化
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		sLCDInfo.u8LCDCntr[1][0] = u8CntrNone;
		sLCDInfo.u8LCDCntr[1][1] =
			u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT);
		sLCDInfo.u8LCDCntr[1][2] =
			u8LCDTypeToByte(LCD_CTR_BLINK_INPUT, LCD_CTR_NONE, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT);
		sLCDInfo.u8LCDCntr[1][3] =
			u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_NONE);
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else {
		if ((psProcInfo->u32Param_0 + 1000) > u32TickCount_ms && u8Key == (uint8)NULL) {
			return;
		}
		psProcInfo->u32Param_0 = u32TickCount_ms;
	}

	//==========================================================================
	// プロセス処理
	//==========================================================================
	// OKボタン処理
	if (u8Key == '*') {
		// 入力値チェック
		if (!bValUtil_validDate(sEditDate.u16Year, sEditDate.u8Month, sEditDate.u8Day)) {
			return;
		}
		// 日付更新
		sAppIO.sDatetime.u16Year = sEditDate.u16Year;
		sAppIO.sDatetime.u8Month = sEditDate.u8Month;
		sAppIO.sDatetime.u8Day   = sEditDate.u8Day;
		if (bDS3231_setDatetime(&sAppIO.sDatetime) == FALSE) {
			vProc_SetMessage("I2C Access Err!!", "RTC Device Write");
			return;
		}
		vProc_Prev();
		return;
	}
	// キャンセルボタン処理
	if (u8Key == '#') {
		// キャンセルして日時メニューへ戻る
		vProc_Prev();
		return;
	}
	// 文字入力判定
	if (u8Key != (uint8)NULL) {
		// 文字編集処理
		bLCDWriteNumber(u8Key, '0');
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
		// 入力値抽出
		sEditDate.u16Year = u32ValUtil_stringToU32(sLCDInfo.cLCDBuff[1], 5, 4);
		sEditDate.u8Month = u32ValUtil_stringToU32(sLCDInfo.cLCDBuff[1], 10, 2);
		sEditDate.u8Day   = u32ValUtil_stringToU32(sLCDInfo.cLCDBuff[1], 13, 2);
	}

	//==========================================================================
	// LCD描画
	//==========================================================================
	// 画面情報設定
	sprintf(sLCDInfo.cLCDBuff[0], "Now :%04d/%02d/%02d ",
			sAppIO.sDatetime.u16Year, sAppIO.sDatetime.u8Month, sAppIO.sDatetime.u8Day);
	sprintf(sLCDInfo.cLCDBuff[1], "Edit:%04d/%02d/%02d ",
			(int)sEditDate.u16Year, (int)sEditDate.u8Month, (int)sEditDate.u8Day);
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_WeekSetting
 *
 * DESCRIPTION:日付設定
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_WeekSetting(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow    = 0;	// 表示位置
			psProcInfo->u8CursorPosRow   = 1;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol   = 5;	// カーソル位置（列）
		}
		// 曜日設定
		psProcInfo->u32Param_0 = sAppIO.sDatetime.u8Wday;
		psProcInfo->u32Param_1 = u32TickCount_ms;
		// 画面制御情報初期化
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		sLCDInfo.u8LCDCntr[1][1] =
			u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_BLINK_INPUT, LCD_CTR_NONE, LCD_CTR_NONE);
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else {
		if ((psProcInfo->u32Param_1 + 1000) > u32TickCount_ms && u8Key == (uint8)NULL) {
			return;
		}
		psProcInfo->u32Param_1 = u32TickCount_ms;
	}

	//==========================================================================
	// プロセス処理
	//==========================================================================
	// OKボタン処理
	if (u8Key == '*') {
		// 曜日更新
		sAppIO.sDatetime.u8Wday = psProcInfo->u32Param_0;
		if (!bDS3231_setDatetime(&sAppIO.sDatetime)) {
			vProc_SetMessage("I2C Access Err!!", "RTC Device Write");
			return;
		}
		vProc_Prev();
		return;
	}
	// キャンセルボタン処理
	if (u8Key == '#') {
		// キャンセルして日時メニューへ戻る
		vProc_Prev();
		return;
	}
	// 入力処理
	if (u8Key == '2' || u8Key == '3') {
		if (psProcInfo->u32Param_0 > 1) {
			psProcInfo->u32Param_0--;
		} else {
			psProcInfo->u32Param_0 = 7;
		}
	} else if (u8Key == '8' || u8Key == '9') {
		if (psProcInfo->u32Param_0 < 7) {
			psProcInfo->u32Param_0++;
		} else {
			psProcInfo->u32Param_0 = 1;
		}
	}

	//==========================================================================
	// LCD描画
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "Now :%s  ", cpDS3231_convWeekday(sAppIO.sDatetime.u8Wday));
	sprintf(sLCDInfo.cLCDBuff[1], "Edit:%s  ", cpDS3231_convWeekday(psProcInfo->u32Param_0));
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_TimeSetting
 *
 * DESCRIPTION:時刻設定
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_TimeSetting(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 1;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 5;		// カーソル位置（列）
		}
		// 時刻設定
		psProcInfo->u32Param_0 = u32TickCount_ms;
		sEditTime.u8Hour       = sAppIO.sDatetime.u8Hour;
		sEditTime.u8Minutes    = sAppIO.sDatetime.u8Minutes;
		sEditTime.u8Seconds    = sAppIO.sDatetime.u8Seconds;
		// 画面制御情報初期化
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		sLCDInfo.u8LCDCntr[1][0] = u8CntrNone;
		sLCDInfo.u8LCDCntr[1][1] =
			u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_NONE);
		sLCDInfo.u8LCDCntr[1][2] =
			u8LCDTypeToByte(LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_NONE, LCD_CTR_BLINK_INPUT);
		sLCDInfo.u8LCDCntr[1][3] =
			u8LCDTypeToByte(LCD_CTR_BLINK_INPUT, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else {
		if ((psProcInfo->u32Param_0 + 1000) > u32TickCount_ms && u8Key == (uint8)NULL) {
			return;
		}
		psProcInfo->u32Param_0 = u32TickCount_ms;
	}

	//==========================================================================
	// プロセス処理
	//==========================================================================
	// OKボタン処理
	if (u8Key == '*') {
		// 入力値チェック
		if (!bValUtil_validTime(sEditTime.u8Hour, sEditTime.u8Minutes, sEditTime.u8Seconds)) {
			return;
		}
		// 時刻更新
		sAppIO.sDatetime.u8Hour    = sEditTime.u8Hour;
		sAppIO.sDatetime.u8Minutes = sEditTime.u8Minutes;
		sAppIO.sDatetime.u8Seconds = sEditTime.u8Seconds;
		if (!bDS3231_setDatetime(&sAppIO.sDatetime)) {
			vProc_SetMessage("I2C Access Err!!", "RTC Device Write");
			return;
		}
		vProc_Prev();
		return;
	}
	// キャンセルボタン処理
	if (u8Key == '#') {
		// キャンセルして日時メニューへ戻る
		vProc_Prev();
		return;
	}
	// その他のキー入力処理
	if (u8Key != (uint8)NULL) {
		// 入力値編集
		bLCDWriteNumber(u8Key, '0');
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
		// 入力値抽出
		sEditTime.u8Hour    = u32ValUtil_stringToU32(sLCDInfo.cLCDBuff[1], 5, 2);
		sEditTime.u8Minutes = u32ValUtil_stringToU32(sLCDInfo.cLCDBuff[1], 8, 2);
		sEditTime.u8Seconds = u32ValUtil_stringToU32(sLCDInfo.cLCDBuff[1], 11, 2);
	}

	//==========================================================================
	// LCD描画
	//==========================================================================
	// 画面情報設定
	sprintf(sLCDInfo.cLCDBuff[0], "Now :%02d:%02d:%02d   ",
		sAppIO.sDatetime.u8Hour, sAppIO.sDatetime.u8Minutes, sAppIO.sDatetime.u8Seconds);
	sprintf(sLCDInfo.cLCDBuff[1], "Edit:%02d:%02d:%02d   ",
			(int)sEditTime.u8Hour, (int)sEditTime.u8Minutes, (int)sEditTime.u8Seconds);
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_DeviceInfoMenu
 *
 * DESCRIPTION:デバイス情報メニュー
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_DeviceInfoMenu(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			sLCDInfo.u8CurrentDispRow  = 0;	// 表示位置
			psProcInfo->u8CursorPosRow = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;	// カーソル位置（列）
		}
		// 画面制御情報設定
		uint8 u8Col;
		uint8 u8CntrDisp = u8LCDTypeToByte(LCD_CTR_BLINK, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
			u8CntrDisp = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// プロセス処理
	//==========================================================================
	// メニュー選択
	if (u8Key == '*') {
		// 画面遷移先判定
		teAppProcess eAppProcess = E_PROCESS_MAIN_MENU;
		switch (psProcInfo->u8CursorPosRow) {
			case 0:
				eAppProcess = E_PROCESS_DEV_INFO_01;
				break;
			case 1:
				eAppProcess = E_PROCESS_DEV_EDIT_01;
				break;
			case 2:
				eAppProcess = E_PROCESS_DEV_STS_CLR;
				break;
		}
		vProc_Next(eAppProcess);
		return;
	}
	if (u8Key == '#') {
		// メインメニュー画面へ遷移
		vProc_Prev();
		return;
	}
	// カーソル移動
	if (u8Key != (uint8)NULL) {
		bLCDCursorKeyMove(u8Key);
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
	}

	//==========================================================================
	// LCD描画
	//==========================================================================
	// 画面情報設定
	sprintf(sLCDInfo.cLCDBuff[0], "1:Dev Info Ref  ");
	sprintf(sLCDInfo.cLCDBuff[1], "2:Dev Info Edit ");
	sprintf(sLCDInfo.cLCDBuff[2], "3:Status Clear  ");
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_DispDeviceInfo_01
 *
 * DESCRIPTION:デバイス情報参照（デバイスID）
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_DispDeviceInfo_01(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// デバイス情報の読み込み
			if (!bEEPROMReadDevInfo(&sDevInfo)) {
				// 読めない場合にはエラーメッセージ表示
				vProc_SetMessage("I2C Access Err!!", "Device Info     ");
				return;
			}
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow    = 0;	// 表示位置
			psProcInfo->u8CursorPosRow   = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol   = 0;	// カーソル位置（列）
		}
		// 画面制御情報初期化
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*') {
		// デバイス名表示に遷移
		vProc_Replace(E_PROCESS_DEV_INFO_02);
		return;
	}
	if (u8Key == '#') {
		// キャンセルしてデバイス情報メニューへ戻る
		vProc_Prev();
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "Item :DeviceID  ");
	sprintf(sLCDInfo.cLCDBuff[1], "Value:%08u  ", (unsigned int)sDevInfo.u32DeviceID);

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_DispDeviceInfo_02
 *
 * DESCRIPTION:デバイス情報参照（デバイス名）
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_DispDeviceInfo_02(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow    = 0;	// 表示位置
			psProcInfo->u8CursorPosRow   = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol   = 0;	// カーソル位置（列）
		}
		// 画面制御情報初期化
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*') {
		// マスターパスワードハッシュ表示に遷移
		vProc_Replace(E_PROCESS_DEV_INFO_03);
		return;
	}
	if (u8Key == '#') {
		// キャンセルしてデバイスID表示へ戻る
		vProc_Replace(E_PROCESS_DEV_INFO_01);
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sDevInfo.cDeviceName[10] = '\0';
	sprintf(sLCDInfo.cLCDBuff[0], "Item :DeviceName");
	sprintf(sLCDInfo.cLCDBuff[1], "Value:%10s", sDevInfo.cDeviceName);

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_DispDeviceInfo_03
 *
 * DESCRIPTION:デバイス情報参照（マスターパスワードハッシュ）
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_DispDeviceInfo_03(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow    = 0;	// 表示位置
			psProcInfo->u8CursorPosRow   = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol   = 0;	// カーソル位置（列）
		}
		// 画面制御情報初期化
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_BLINK, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*') {
		// ストレッチング回数表示に遷移
		vProc_Replace(E_PROCESS_DEV_INFO_04);
		return;
	}
	if (u8Key == '#') {
		// デバイス名表示に遷移
		vProc_Replace(E_PROCESS_DEV_INFO_02);
		return;
	}
	if (u8Key == '2' || u8Key == '3') {
		// カーソル移動
		if (psProcInfo->u8CursorPosRow > 0) {
			psProcInfo->u8CursorPosRow--;
			bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		}
	} else if (u8Key == '8' || u8Key == '9') {
		// カーソル移動
		if (psProcInfo->u8CursorPosRow < 4) {
			psProcInfo->u8CursorPosRow++;
			bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		}
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	TypeConverter converter;
	memcpy(converter.u8Values, sDevInfo.u8MstPWHash, 32);
	sprintf(sLCDInfo.cLCDBuff[0], "Item :MstPW Hash");
	sprintf(sLCDInfo.cLCDBuff[1], "%08X%08X", converter.iValues[0], converter.iValues[1]);
	sprintf(sLCDInfo.cLCDBuff[2], "%08X%08X", converter.iValues[2], converter.iValues[3]);
	sprintf(sLCDInfo.cLCDBuff[3], "%08X%08X", converter.iValues[4], converter.iValues[5]);
	sprintf(sLCDInfo.cLCDBuff[4], "%08X%08X", converter.iValues[6], converter.iValues[7]);

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_DispDeviceInfo_04
 *
 * DESCRIPTION:デバイス情報参照（パスワードストレッチング回数）
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_DispDeviceInfo_04(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow    = 0;	// 表示位置
			psProcInfo->u8CursorPosRow   = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol   = 0;	// カーソル位置（列）
		}
		// 画面制御情報初期化
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*') {
		// 通信暗号化パスワード表示に遷移
		vProc_Replace(E_PROCESS_DEV_INFO_05);
		return;
	}
	if (u8Key == '#') {
		// マスターパスワードハッシュ表示に遷移
		vProc_Replace(E_PROCESS_DEV_INFO_03);
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "Item :PW Stretch");
	sprintf(sLCDInfo.cLCDBuff[1], "Value:%3d       ", sDevInfo.u16MstPWStretching);

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_DispDeviceInfo_05
 *
 * DESCRIPTION:デバイス情報参照（通信暗号化パスワード）
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_DispDeviceInfo_05(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow    = 0;	// 表示位置
			psProcInfo->u8CursorPosRow   = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol   = 0;	// カーソル位置（列）
		}
		// 画面制御情報初期化
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*') {
		// ステータス表示に遷移
		vProc_Replace(E_PROCESS_DEV_INFO_06);
		return;
	}
	if (u8Key == '#') {
		// マスターパスワードストレッチング回数表示に遷移
		vProc_Replace(E_PROCESS_DEV_INFO_04);
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sDevInfo.u8AESPassword[12] = '\0';
	sprintf(sLCDInfo.cLCDBuff[0], "Item :AES Key   ");
	sprintf(sLCDInfo.cLCDBuff[1], "Val:%12s", sDevInfo.u8AESPassword);

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_DispDeviceInfo_06
 *
 * DESCRIPTION:デバイス情報参照（ステータスマップ）
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_DispDeviceInfo_06(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow    = 0;	// 表示位置
			psProcInfo->u8CursorPosRow   = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol   = 0;	// カーソル位置（列）
		}
		// 画面制御情報初期化
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*') {
		vProc_Prev();
		return;
	}
	if (u8Key == '#') {
		// 通信暗号化パスワード表示に遷移
		vProc_Replace(E_PROCESS_DEV_INFO_05);
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "Item :Status Map");
	sprintf(sLCDInfo.cLCDBuff[1], "Value:%08d  ", (int)u32ValUtil_u8ToBinary(sDevInfo.u8StatusMap));

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_EditDeviceInfo_01
 *
 * DESCRIPTION:デバイス情報編集（デバイスID）
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_EditDeviceInfo_01(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// デバイス情報の読み込み
			if (!bEEPROMReadDevInfo(&sDevInfo)) {
				// 読めない場合にはエラーメッセージ表示
				vProc_SetMessage("I2C Access Err!!", "Device Info     ");
				return;
			}
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow   = 0;	// 表示位置
			psProcInfo->u8CursorPosRow  = 1;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol  = 6;	// カーソル位置（列）
			// 編集用領域へコピー
			sEditDevInfo = sDevInfo;
			// デバイス名終端編集
			sEditDevInfo.cDeviceName[10] = '\0';
			// 初期パスワード生成
			vSetRandString((char*)sEditDevInfo.u8MstPWHash, 32);
			// 初期ストレッチ回数生成
			sEditDevInfo.u16MstPWStretching = u32ValUtil_getRandVal() % 256 + STRETCHING_CNT_BASE;
			// 通信暗号化パスワード終端編集
			sEditDevInfo.u8AESPassword[12] = '\0';
			if (sEditDevInfo.u8AESPassword[0] == '\0') {
				memset(sEditDevInfo.u8AESPassword, '0', 12);
			}
			// ステータスマップ初期化
			sEditDevInfo.u8StatusMap = 0x00;
		}
		// 画面制御
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		uint8 u8CntrInput =
			u8LCDTypeToByte(LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT);
		sLCDInfo.u8LCDCntr[1][0] = u8CntrNone;
		sLCDInfo.u8LCDCntr[1][1] =
			u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT);
		sLCDInfo.u8LCDCntr[1][2] = u8CntrInput;
		sLCDInfo.u8LCDCntr[1][3] =
			u8LCDTypeToByte(LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_NONE, LCD_CTR_NONE);
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*') {
		// デバイス名表示に遷移
		vProc_Replace(E_PROCESS_DEV_EDIT_02);
		return;
	}
	if (u8Key == '#') {
		// キャンセルしてデバイス情報メニューへ戻る
		vProc_Prev();
		return;
	}
	if (u8Key != (uint8)NULL) {
		// 入力値を編集
		bLCDWriteNumber(u8Key, '0');
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
		// オーバーフローをした場合には、その値を編集する
		sEditDevInfo.u32DeviceID = u32ValUtil_stringToU32(sLCDInfo.cLCDBuff[1], 6, 8);
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	// デバイス名設定
	sEditDevInfo.cDeviceName[10] = '\0';
	sprintf(sLCDInfo.cLCDBuff[0], "Item :DeviceID  ");
	sprintf(sLCDInfo.cLCDBuff[1], "Value:%08u  ", (unsigned int)sEditDevInfo.u32DeviceID);

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_EditDeviceInfo_02
 *
 * DESCRIPTION:デバイス情報編集（デバイス名）
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_EditDeviceInfo_02(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 1;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 6;		// カーソル位置（列）
		}
		// 画面制御
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		sLCDInfo.u8LCDCntr[1][0] = u8CntrNone;
		sLCDInfo.u8LCDCntr[1][1] =
			u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT);
		uint8 u8CntrInput =
			u8LCDTypeToByte(LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT);
		sLCDInfo.u8LCDCntr[1][2] = u8CntrInput;
		sLCDInfo.u8LCDCntr[1][3] = u8CntrInput;
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*') {
		// マスターパスワード編集に遷移
		vProc_SetInputToken(E_PROCESS_DEV_EDIT_03, E_PROCESS_DEV_EDIT_02, FALSE);
		sprintf(sAppScrParam.cDispMsg[0], "Item :Master PW ");
		memcpy(sAppScrParam.cDispMsg[1], &sEditDevInfo.u8MstPWHash[0], LCD_BUFF_COL_SIZE);
		memcpy(sAppScrParam.cDispMsg[2], &sEditDevInfo.u8MstPWHash[16], LCD_BUFF_COL_SIZE);
		return;
	}
	if (u8Key == '#') {
		// デバイスID編集に遷移
		vProc_Replace(E_PROCESS_DEV_EDIT_01);
		return;
	}
	if (u8Key != (uint8)NULL) {
		bLCDWriteChar(u8Key);
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
		memcpy(sEditDevInfo.cDeviceName, &sLCDInfo.cLCDBuff[1][6], 10);
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	// デバイス名設定
	sEditDevInfo.cDeviceName[10] = '\0';
	sprintf(sLCDInfo.cLCDBuff[0], "Item :DeviceName");
	sprintf(sLCDInfo.cLCDBuff[1], "Value:%10s", sEditDevInfo.cDeviceName);

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_EditDeviceInfo_03
 *
 * DESCRIPTION:デバイス情報編集（マスターパスワードストレッチング）
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_EditDeviceInfo_03(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置設定
			sLCDInfo.u8CurrentDispRow  = 0;	// 表示位置
			psProcInfo->u8CursorPosRow = 1;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 6;	// カーソル位置（列）
			bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
			// マスターパスワード編集
			memcpy(&sEditDevInfo.u8MstPWHash[0], sAppScrParam.cDispMsg[1], LCD_BUFF_COL_SIZE);
			memcpy(&sEditDevInfo.u8MstPWHash[16], sAppScrParam.cDispMsg[2], LCD_BUFF_COL_SIZE);
		}
		// 画面制御情報更新
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		sLCDInfo.u8LCDCntr[1][0] = u8CntrNone;
		sLCDInfo.u8LCDCntr[1][1] =
			u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT);
		sLCDInfo.u8LCDCntr[1][2] =
			u8LCDTypeToByte(LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_NONE, LCD_CTR_NONE);
		sLCDInfo.u8LCDCntr[1][3] = u8CntrNone;
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*') {
		// 通信暗号化パスワード編集へ遷移
		vProc_Replace(E_PROCESS_DEV_EDIT_04);
		return;
	}
	if (u8Key == '#') {
		// マスターパスワード編集に遷移
		vProc_SetInputToken(E_PROCESS_DEV_EDIT_03, E_PROCESS_DEV_EDIT_02, FALSE);
		sprintf(sAppScrParam.cDispMsg[0], "Item :Master PW ");
		memcpy(sAppScrParam.cDispMsg[1], &sEditDevInfo.u8MstPWHash[0], LCD_BUFF_COL_SIZE);
		memcpy(sAppScrParam.cDispMsg[2], &sEditDevInfo.u8MstPWHash[16], LCD_BUFF_COL_SIZE);
		return;
	}
	if (u8Key != (uint8)NULL) {
		bLCDWriteNumber(u8Key, ' ');
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
		sEditDevInfo.u16MstPWStretching = u32ValUtil_stringToU32(sLCDInfo.cLCDBuff[1], 6, 4);
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	// ストレッチ設定
	sprintf(sLCDInfo.cLCDBuff[0], "Item :Stretching");
	sprintf(sLCDInfo.cLCDBuff[1], "Value:%-4d      ", sEditDevInfo.u16MstPWStretching);

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_EditDeviceInfo_04
 *
 * DESCRIPTION:デバイス情報編集（通信暗号化パスワード）
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_EditDeviceInfo_04(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 1;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 4;		// カーソル位置（列）
		}
		// 画面制御
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		uint8 u8CntrInput =
			u8LCDTypeToByte(LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT);
		sLCDInfo.u8LCDCntr[1][0] = u8CntrNone;
		sLCDInfo.u8LCDCntr[1][1] = u8CntrInput;
		sLCDInfo.u8LCDCntr[1][2] = u8CntrInput;
		sLCDInfo.u8LCDCntr[1][3] = u8CntrInput;
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*') {
		// デバイス情報の書き込み確認メッセージ表示
		//		vProc_Replace(E_PROCESS_DEV_EDIT_06);
		sAppScrParam.eChgTypeOK = E_PROCESS_CHG_REPLACE;
		sAppScrParam.eProcessIdOK = E_PROCESS_DEV_EDIT_05;
		sAppScrParam.eChgTypeCANCEL = E_PROCESS_CHG_REPLACE;
		sAppScrParam.eProcessIdCANCEL = E_PROCESS_DEV_EDIT_04;
		// 確認メッセージ編集
		sprintf(sAppScrParam.cDispMsg[0], "Do you write?   ");
		sprintf(sAppScrParam.cDispMsg[1], " OK(*)/CANCEL(#)");
		// メッセージ表示プロセスに切替
		vProc_Replace(E_PROCESS_SHOW_MSG);
		return;
	}
	if (u8Key == '#') {
		// マスターパスワードストレッチング回数編集に遷移
		vProc_Replace(E_PROCESS_DEV_EDIT_03);
		return;
	}
	if (u8Key != (uint8)NULL) {
		bLCDWriteChar(u8Key);
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
		memcpy(sEditDevInfo.u8AESPassword, &sLCDInfo.cLCDBuff[1][4], 12);
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	// 通信暗号化パスワード
	sEditDevInfo.u8AESPassword[12] = '\0';
	sprintf(sLCDInfo.cLCDBuff[0], "Item :AES Key   ");
	sprintf(sLCDInfo.cLCDBuff[1], "Val:%12s", sEditDevInfo.u8AESPassword);

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_EditDeviceInfo_05
 *
 * DESCRIPTION:デバイス情報書き込み
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_EditDeviceInfo_05(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 0;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;		// カーソル位置（列）
			psProcInfo->u32Param_0 = 0;
		}
		// 画面制御
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	}
	//==========================================================================
	// 書き込み処理
	//==========================================================================
	switch (psProcInfo->u32Param_0) {
	case 0:
		// マスターパスワードのハッシュ化処理をバックグラウンドプロセスとして起動
		sHashGenInfo = sAuth_generateHashInfo(sEditDevInfo.u8MstPWHash, sEditDevInfo.u16MstPWStretching);
		iEntrySeqEvt(E_EVENT_APP_HASH_ST);
		psProcInfo->u32Param_0 = 1;
		break;
	case 1:
		// マスターパスワードのハッシュ化処理完了判定
		if (sHashGenInfo.eStatus == E_AUTH_HASH_PROC_COMPLETE) {
			psProcInfo->u32Param_0 = 2;
		}
		return;
	case 2:
		// マスターパスワードのハッシュ値編集
		memcpy(sEditDevInfo.u8MstPWHash, sHashGenInfo.u8HashCode, 32);
		// メモリへの書き込み
		if (!bEEPROMWriteDevInfo(&sEditDevInfo)) {
			// デバイスエラー表示
			vProc_SetMessage("I2C Access Err!!", "Device Info     ");
			return;
		}
		// ソフトウェアリセット
		vAHI_SwReset();
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "Please wait...  ");
	sprintf(sLCDInfo.cLCDBuff[1], "In writing...   ");

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_ClearDeviceSts
 *
 * DESCRIPTION:デバイス情報ステータスクリア
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_ClearDeviceSts(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 0;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;		// カーソル位置（列）
			// デバイス情報の読み込み
			if (!bEEPROMReadDevInfo(&sDevInfo)) {
				// デバイスエラー表示
				vProc_SetMessage("I2C Access Err!!", "Device Info     ");
				return;
			}
			// 編集用領域へコピー
			sEditDevInfo = sDevInfo;
			// ステータスマップ初期化
			sEditDevInfo.u8StatusMap = 0x00;
		}
		// 画面制御
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// 書き込み処理
	//==========================================================================
	if (u8Key == '*') {
		// メモリへの書き込み
		if (!bEEPROMWriteDevInfo(&sEditDevInfo)) {
			// エラーメッセージを表示してデバイス情報メニューへ戻る
			vProc_SetMessage("I2C Access Err!!", "Device Info     ");
			return;
		}
		// メニュー画面に戻る
		vProc_Prev();
		return;
	}
	if (u8Key == '#') {
		// メニュー画面に戻る
		vProc_Prev();
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "Status Clear?   ");
	sprintf(sLCDInfo.cLCDBuff[1], " OK(*)/CANCEL(#)");

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_RemoteDevMenu
 *
 * DESCRIPTION:リモートデバイス情報メニュー
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_RemoteDevMenu(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			sLCDInfo.u8CurrentDispRow  = 0;	// 表示位置
			psProcInfo->u8CursorPosRow = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;	// カーソル位置（列）
		}
		// 画面制御情報設定
		uint8 u8Col;
		uint8 u8CntrDisp = u8LCDTypeToByte(LCD_CTR_BLINK, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
			u8CntrDisp = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// プロセス処理
	//==========================================================================
	// メニュー選択
	if (u8Key == '*') {
		// 画面遷移先判定
		switch (psProcInfo->u8CursorPosRow) {
			case 0:
				// ステータスチェック
				vProc_SetSelectRemoteDev(E_PROCESS_REMOTE_DEV_INFO);
				break;
			case 1:
				// 同期処理
				vProc_Next(E_PROCESS_SYNCHRONIZING);
				break;
			case 2:
				// リモートデバイス情報削除
				vProc_SetSelectRemoteDev(E_PROCESS_DEL_REMOTE);
				break;
			case 3:
				// リモートデバイス情報全削除
				vProc_Next(E_PROCESS_DEL_REMOTE_ALL);
				break;
			default:
				vProc_Prev();
		}
		return;
	}
	if (u8Key == '#') {
		// メインメニュー画面へ遷移
		vProc_Prev();
		return;
	}
	// カーソル移動
	if (bLCDCursorKeyMove(u8Key)) {
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
	}

	//==========================================================================
	// LCD描画
	//==========================================================================
	// 画面情報設定
	sprintf(sLCDInfo.cLCDBuff[0], "1:Display Info  ");
	sprintf(sLCDInfo.cLCDBuff[1], "2:Synchronizing ");
	sprintf(sLCDInfo.cLCDBuff[2], "3:Delete Info   ");
	sprintf(sLCDInfo.cLCDBuff[3], "4:Delete All    ");
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_RemoteDevInfo
 *
 * DESCRIPTION:リモートデバイス情報表示
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_RemoteDevInfo(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			sLCDInfo.u8CurrentDispRow  = 0;	// 表示位置
			psProcInfo->u8CursorPosRow = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;	// カーソル位置（列）
		}
		// 画面制御情報設定
		uint8 u8Col;
		uint8 u8CntrDisp = u8LCDTypeToByte(LCD_CTR_BLINK, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
			u8CntrDisp = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*' || u8Key == '#') {
		// リモートデバイスメニューに移動
		vProc_Prev();
		return;
	}
	// カーソル移動
	if (bLCDCursorKeyMove(u8Key)) {
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "DevID:%08u  ", (int)sRemoteInfoMain.u32DeviceID);
	sprintf(sLCDInfo.cLCDBuff[1], "Name :%10s", sRemoteInfoMain.cDeviceName);
	tsDate sDate = sValUtil_dayToDate(sRemoteInfoMain.u32StartDateTime / (24 * 60));
	sprintf(sLCDInfo.cLCDBuff[2], "Date:%04d/%02d/%02d ", sDate.u16Year, sDate.u8Month, sDate.u8Day);
	int iMinute = sRemoteInfoMain.u32StartDateTime % (60 * 24);
	sprintf(sLCDInfo.cLCDBuff[3], "Time:%02d:%02d      ", iMinute / 60, iMinute % 60);
	sprintf(sLCDInfo.cLCDBuff[4], "Status:%08d ", (int)u32ValUtil_u8ToBinary(sRemoteInfoMain.u8StatusMap));

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_Synchronizing
 *
 * DESCRIPTION:リモートデバイス情報同期処理
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_Synchronizing(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 0;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;		// カーソル位置（列）
			psProcInfo->u32Param_0     = 0;		// 処理ステップ
		}
		// 画面制御
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	}
	//==========================================================================
	// 編集書き込み処理
	//==========================================================================
	uint16 u16StCnt;
	switch (psProcInfo->u32Param_0) {
	case 0:
		// サブデバイス接続確認
		vI2CSubConnect();
		sAppIO.u8I2CSubType = u8I2CDeviceTypeChk(&sAppIO.sDatetime);
		if ((sAppIO.u8I2CSubType & I2C_DEVICE_EEPROM) != I2C_DEVICE_EEPROM) {
			// 接続エラー
			vProc_SetMessage("I2C Access Err!!", "Sub Device      ");
			return;
		}
		// メインデバイス接続確認
		vI2CMainConnect();
		sAppIO.u8I2CMainType = u8I2CDeviceTypeChk(&sAppIO.sDatetime);
		if ((sAppIO.u8I2CMainType & (I2C_DEVICE_EEPROM | I2C_DEVICE_RTC)) != (I2C_DEVICE_EEPROM | I2C_DEVICE_RTC)) {
			// 接続エラー
			vProc_SetMessage("I2C Access Err!!", "Main Device     ");
			return;
		}
		// 処理ステップ更新
		psProcInfo->u32Param_0++;
		break;
	case 1:
		// リモート情報（主＆副）初期化
		//----------------------------------------------------------------------
		// 主側に送信時のリモートデバイス情報の初期編集
		//----------------------------------------------------------------------
		// 副デバイス情報読み込み
		vI2CSubConnect();
		if (!bEEPROMReadDevInfo(&sDevInfo)) {
			// 読めない場合にはエラーメッセージ表示
			vProc_SetMessage("I2C Access Err!!", "Device Info     ");
			return;
		}
		sRemoteInfoMain.u32DeviceID = sDevInfo.u32DeviceID;			// デバイスID
		sDevInfo.cDeviceName[10] = '\0';
		strcpy(sRemoteInfoMain.cDeviceName, sDevInfo.cDeviceName);	// デバイス名
		vValUtil_setU8RandArray(sRemoteInfoMain.u8SyncToken, 32);	// 同期トークン
		vValUtil_setU8RandArray(sRemoteInfoMain.u8AuthCode, 32);	// 認証コード（受信側情報）
		// ストレッチングカウントA（送信側）
		sRemoteInfoMain.u8SndStretching =
				(u32ValUtil_getRandVal() % (256 - APP_HASH_STRETCHING_CNT_MIN)) + APP_HASH_STRETCHING_CNT_MIN;
		// ストレッチングカウントB（照合側）
		sRemoteInfoMain.u8RcvStretching =
				(u32ValUtil_getRandVal() % (256 - APP_HASH_STRETCHING_CNT_MIN)) + APP_HASH_STRETCHING_CNT_MIN;
		sRemoteInfoMain.u8StatusMap = 0x00;							// ステータスマップ
		//----------------------------------------------------------------------
		// 副側に送信時のリモートデバイス情報の初期編集
		//----------------------------------------------------------------------
		// 主デバイス情報読み込み
		vI2CMainConnect();
		if (!bEEPROMReadDevInfo(&sDevInfo)) {
			// 読めない場合にはエラーメッセージ表示
			vProc_SetMessage("I2C Access Err!!", "Device Info     ");
			return;
		}
		sRemoteInfoSub.u32DeviceID = sDevInfo.u32DeviceID;			// デバイスID
		sDevInfo.cDeviceName[10] = '\0';
		strcpy(sRemoteInfoSub.cDeviceName, sDevInfo.cDeviceName);	// デバイス名
		memcpy(sRemoteInfoSub.u8SyncToken, sRemoteInfoMain.u8SyncToken, 32);	// 同期トークン
		vValUtil_setU8RandArray(sRemoteInfoSub.u8AuthCode, 32);		// 認証コード（受信側情報）
		// ストレッチングカウントA（送信側）
		sRemoteInfoSub.u8SndStretching =
			(u32ValUtil_getRandVal() % (256 - APP_HASH_STRETCHING_CNT_MIN)) + APP_HASH_STRETCHING_CNT_MIN;
		// ストレッチングカウントB（照合側）
		sRemoteInfoSub.u8RcvStretching =
			(u32ValUtil_getRandVal() % (256 - APP_HASH_STRETCHING_CNT_MIN)) + APP_HASH_STRETCHING_CNT_MIN;
		sRemoteInfoSub.u8StatusMap = 0x00;							// ステータスマップ
		// 処理ステップ更新
		psProcInfo->u32Param_0++;
		return;
	case 2:
		//----------------------------------------------------------------------
		// 副→主に送信時のハッシュ値生成
		//----------------------------------------------------------------------
		// ハッシュ生成情報
		u16StCnt = sRemoteInfoSub.u8SndStretching + sRemoteInfoMain.u8RcvStretching + STRETCHING_CNT_BASE;
		sHashGenInfo = sAuth_generateHashInfo(sRemoteInfoMain.u8AuthCode, u16StCnt);
		vAuth_setSyncToken(&sHashGenInfo, sRemoteInfoMain.u8SyncToken);
		// ハッシュ化処理をバックグラウンドプロセスとして起動
		iEntrySeqEvt(E_EVENT_APP_HASH_ST);
		// 処理待ちに移行
		psProcInfo->u32Param_0++;
		return;
	case 3:
		// 副→主のハッシュ値生成処理の完了判定
		if (sHashGenInfo.eStatus == E_AUTH_HASH_PROC_COMPLETE) {
			psProcInfo->u32Param_0++;
		}
		return;
	case 4:
		//----------------------------------------------------------------------
		// 副→主に送信時のハッシュ値編集
		//----------------------------------------------------------------------
		memcpy(sRemoteInfoSub.u8AuthHash, sHashGenInfo.u8HashCode, APP_AUTH_TOKEN_SIZE);
		//----------------------------------------------------------------------
		// 主→副に送信時のハッシュ値生成
		//----------------------------------------------------------------------
		// ハッシュ生成情報
		u16StCnt = sRemoteInfoMain.u8SndStretching + sRemoteInfoSub.u8RcvStretching + STRETCHING_CNT_BASE;
		sHashGenInfo = sAuth_generateHashInfo(sRemoteInfoSub.u8AuthCode, u16StCnt);
		vAuth_setSyncToken(&sHashGenInfo, sRemoteInfoSub.u8SyncToken);
		// ハッシュ化処理をバックグラウンドプロセスとして起動
		iEntrySeqEvt(E_EVENT_APP_HASH_ST);
		// ハッシュ生成
		psProcInfo->u32Param_0++;
		return;
	case 5:
		// 主→副のハッシュ値生成完了判定
		if (sHashGenInfo.eStatus == E_AUTH_HASH_PROC_COMPLETE) {
			psProcInfo->u32Param_0++;
		}
		return;
	case 6:
		//----------------------------------------------------------------------
		// 主→副に送信時のハッシュ値編集
		//----------------------------------------------------------------------
		// ハッシュ値
		memcpy(sRemoteInfoMain.u8AuthHash, sHashGenInfo.u8HashCode, APP_AUTH_TOKEN_SIZE);

		//----------------------------------------------------------------------
		// 利用開始日時の編集
		//----------------------------------------------------------------------
		// 現時点での年月日時分を算出
		DS3231_datetime* psDateTime = &sAppIO.sDatetime;
		uint32 u32Days =
			u32ValUtil_dateToDays(psDateTime->u16Year, psDateTime->u8Month, psDateTime->u8Day);
		sRemoteInfoMain.u32StartDateTime =
				u32Days * 24 * 60 + psDateTime->u8Hour * 60 + psDateTime->u8Minutes;
		sRemoteInfoSub.u32StartDateTime = sRemoteInfoMain.u32StartDateTime;

		//----------------------------------------------------------------------
		// 副側のメモリへの書き込み
		//----------------------------------------------------------------------
		// リモートデバイス情報
		vI2CSubConnect();
		if (iEEPROMWriteRemoteInfo(&sRemoteInfoSub) < 0) {
			vProc_SetMessage("I2C Access Err!!", "Sub Remote Info ");
			return;
		}

		//----------------------------------------------------------------------
		// 主側のメモリへの書き込み
		//----------------------------------------------------------------------
		vI2CMainConnect();
		if (iEEPROMWriteRemoteInfo(&sRemoteInfoMain) < 0) {
			// エラーメッセージを表示してリモートデバイス情報メニューへ戻る
			vProc_SetMessage("I2C Access Err!!", "Main Remote Info");
			return;
		}

		//----------------------------------------------------------------------
		// 完了メッセージを表示
		//----------------------------------------------------------------------
		vProc_SetMessage("Remote Dev Info ", "Sync Completion!");
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "Please wait...  ");
	sprintf(sLCDInfo.cLCDBuff[1], "In writing...   ");

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_DelRemoteDev
 *
 * DESCRIPTION:リモートデバイス情報削除（削除確認）
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_DelRemoteDev(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 0;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;		// カーソル位置（列）
		}
		// 主デバイス情報の読み込み
		if (!bEEPROMReadDevInfo(&sDevInfo)) {
			// 読めない場合にはエラーメッセージ表示
			vProc_SetMessage("I2C Access Err!!", "Device Info     ");
			return;
		}
		// 画面制御
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*') {
		// 副側のメモリクリア
		vI2CSubConnect();
		int iSubIdx = iEEPROMDeleteRemoteInfo(sDevInfo.u32DeviceID);
		// 主側のメモリクリア
		vI2CMainConnect();
		char* pcMainMsg = "Main:Completion!";
		if (iEEPROMDeleteRemoteInfo(sRemoteInfoMain.u32DeviceID) < 0) {
			pcMainMsg = "Main:Not Found!!";
		}
		memset(&sRemoteInfoMain, 0x00, sizeof(tsAuthRemoteDevInfo));
		memset(&sRemoteInfoSub, 0x00, sizeof(tsAuthRemoteDevInfo));
		// メッセージを表示してメニューに遷移する
		if (iSubIdx < 0) {
			vProc_SetMessage(pcMainMsg, "Sub :Not Found!!");
		} else {
			vProc_SetMessage(pcMainMsg, "Sub :Completion!");
		}
		return;
	}
	if (u8Key == '#') {
		// メニューに遷移する
		vProc_Prev();
		return;
	}

	//========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "Delete Data?    ");
	sprintf(sLCDInfo.cLCDBuff[1], " OK(*)/CANCEL(#)");

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_ClearRemoteDev
 *
 * DESCRIPTION:リモートデバイス情報全削除
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_ClearRemoteDev(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 0;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;		// カーソル位置（列）
		}
		// 主デバイス情報の読み込み
		if (!bEEPROMReadDevInfo(&sDevInfo)) {
			// 読めない場合にはエラーメッセージ表示
			vProc_SetMessage("I2C Access Err!!", "Device Info     ");
			return;
		}
		// 画面制御
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*') {
		// 副側のメモリクリア
		vI2CSubConnect();
		int iSubIdx = iEEPROMDeleteRemoteInfo(sDevInfo.u32DeviceID);
		// 主側のメモリ全クリア
		vI2CMainConnect();
		int iResult = iEEPROMDeleteAllRemoteInfo();
		if (iResult < 0) {
			if (iResult == -1) {
				vProc_SetMessage("I2C Access Err!!", "Remote Dev Info1");
				return;
			}
			if (iResult == -2) {
				vProc_SetMessage("I2C Access Err!!", "Remote Dev Info2");
				return;
			}
			if (iResult == -3) {
				vProc_SetMessage("I2C Access Err!!", "Remote Dev Info3");
				return;
			}
			vProc_SetMessage("I2C Access Err!!", "Remote Dev Info ");
			return;
		}
		memset(&sRemoteInfoMain, 0x00, sizeof(tsAuthRemoteDevInfo));
		memset(&sRemoteInfoSub, 0x00, sizeof(tsAuthRemoteDevInfo));
		// メッセージを表示してメニューに遷移する
		if (iSubIdx < 0) {
			vProc_SetMessage("Main:Completion!", "Sub :Not Delete!");
		} else {
			vProc_SetMessage("Main:Completion!", "Sub :Completion!");
		}
		return;
	}
	if (u8Key == '#') {
		// メニューに遷移する
		vProc_Prev();
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "Delete All Data?");
	sprintf(sLCDInfo.cLCDBuff[1], " OK(*)/CANCEL(#)");

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_EventLogMenu
 *
 * DESCRIPTION:イベント履歴メニュー
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_EventLogMenu(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			sLCDInfo.u8CurrentDispRow  = 0;	// 表示位置
			psProcInfo->u8CursorPosRow = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;	// カーソル位置（列）
		}
		// 画面制御情報設定
		uint8 u8Col;
		uint8 u8CntrDisp = u8LCDTypeToByte(LCD_CTR_BLINK, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
			u8CntrDisp = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// プロセス処理
	//==========================================================================
	// メニュー選択
	if (u8Key == '*') {
		// 画面遷移先判定
		teAppProcess eAppProcess = E_PROCESS_MAIN_MENU;
		switch (psProcInfo->u8CursorPosRow) {
			case 0:
				eAppProcess = E_PROCESS_DISPLAY_LOG;
				break;
			case 1:
				eAppProcess = E_PROCESS_LOG_CLEAR;
				break;
		}
		vProc_Next(eAppProcess);
		return;
	}
	if (u8Key == '#') {
		// メインメニュー画面へ遷移
		vProc_Prev();
		return;
	}
	// カーソル移動
	if (bLCDCursorKeyMove(u8Key)) {
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
	}

	//==========================================================================
	// LCD描画
	//==========================================================================
	// 画面情報設定
	sprintf(sLCDInfo.cLCDBuff[0], "1:Display Log   ");
	sprintf(sLCDInfo.cLCDBuff[1], "2:Log Clear     ");
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_DispEventLog
 *
 * DESCRIPTION:イベント履歴表示
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_DispEventLog(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			psProcInfo->u32Param_0 = 0;
			int iIdx = iEEPROMReadLog(&sEventLog, psProcInfo->u32Param_0);
			if (iIdx < 0) {
				if (iIdx == -2) {
					vProc_SetMessage("Log Not Found!!!", "Event Log       ");
				} else {
					vProc_SetMessage("I2C Access Err!!", "Event Log       ");
				}
				return;
			}
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow    = 0;	// 表示位置
			psProcInfo->u8CursorPosRow   = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol   = 0;	// カーソル位置（列）
		}
		// 画面制御情報初期化
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*' || u8Key == '#') {
		// イベントログメニューに移動
		vProc_Prev();
		return;
	}
	int iIdx = 0;
	if (u8Key == '2' || u8Key == '3') {
		// 直前のイベントログの読み込み
		psProcInfo->u32Param_0 = (psProcInfo->u32Param_0 + MAX_EVT_LOG_CNT - 1) % MAX_EVT_LOG_CNT;
		iIdx = iEEPROMReadLog(&sEventLog, psProcInfo->u32Param_0);
	} else if (u8Key == '8' || u8Key == '9') {
		// 次のイベントログの読み込み
		psProcInfo->u32Param_0 = (psProcInfo->u32Param_0 + 1) % MAX_EVT_LOG_CNT;
		iIdx = iEEPROMReadLog(&sEventLog, psProcInfo->u32Param_0);
	}
	if (iIdx < 0) {
		// エラーメッセージを表示してイベントログメニューへ戻る
		vProc_SetMessage("I2C Access Err!!", "Event Log       ");
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "%03d M%02d C%02X S%02X ",
			sEventLog.u8SeqNo, sEventLog.u16MsgCd % 100, sEventLog.u8Command, sEventLog.u8StatusMap);
	sprintf(sLCDInfo.cLCDBuff[1], "%04d%02d%02d %02d%02d%02d ",
			sEventLog.u16Year, sEventLog.u8Month, sEventLog.u8Day,
			sEventLog.u8Hour, sEventLog.u8Minute, sEventLog.u8Second);

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_ClearEventLog
 *
 * DESCRIPTION:イベント履歴クリア
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_ClearEventLog(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 0;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;		// カーソル位置（列）
		}
		// 画面制御
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	if (u8Key == '*') {
		// ログクリア
		switch (iEEPROMDeleteAllLogInfo()) {
		case -1:
			vProc_SetMessage("I2C Access Err!!", "Index Read Err!!");
			return;
		case -2:
			vProc_SetMessage("I2C Access Err!!", "Index Write Err!");
			return;
		case -3:
			vProc_SetMessage("I2C Access Err!!", "Log Clear Err!!!");
			return;
		default:
			vProc_SetMessage("Del Completion!!", "Event Log       ");
		}
		return;
	}
	if (u8Key == '#') {
		// メニューに遷移する
		vProc_Prev();
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "All Log Clear?  ");
	sprintf(sLCDInfo.cLCDBuff[1], " OK(*)/CANCEL(#)");

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_ExecCmdMenu
 *
 * DESCRIPTION:コマンド実行メニュー
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_ExecCmdMenu(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			sLCDInfo.u8CurrentDispRow  = 0;	// 表示位置
			psProcInfo->u8CursorPosRow = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;	// カーソル位置（列）
		}
		// 画面制御情報設定
		uint8 u8Col;
		uint8 u8CntrDisp = u8LCDTypeToByte(LCD_CTR_BLINK, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrDisp;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
			u8CntrDisp = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// プロセス処理
	//==========================================================================
	// メニュー選択
	if (u8Key == '*') {
		// 画面遷移先判定
		switch (psProcInfo->u8CursorPosRow) {
			case 0:
				// ステータスチェック
				vProc_SetSelectRemoteDev(E_PROCESS_EXEC_CMD_STATUS);
				break;
			case 1:
				// 開錠
				vProc_SetSelectRemoteDev(E_PROCESS_EXEC_CMD_OPEN);
				break;
			case 2:
				// 通常施錠
				vProc_SetSelectRemoteDev(E_PROCESS_EXEC_CMD_CLOSE);
				break;
			case 3:
				// 警戒施錠
				vProc_SetSelectRemoteDev(E_PROCESS_EXEC_CMD_ALERT);
				break;
			case 4:
				// マスターパスワード開錠
				vProc_SetSelectRemoteDev(E_PROCESS_EXEC_CMD_MPW_OPEN_01);
				break;
			default:
				vProc_Prev();
		}
		return;
	}
	if (u8Key == '#') {
		// メインメニュー画面へ遷移
		vProc_Prev();
		return;
	}
	// カーソル移動
	if (bLCDCursorKeyMove(u8Key)) {
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
	}

	//==========================================================================
	// LCD描画
	//==========================================================================
	// 画面情報設定
	sprintf(sLCDInfo.cLCDBuff[0], "1:Status Check  ");
	sprintf(sLCDInfo.cLCDBuff[1], "2:Open          ");
	sprintf(sLCDInfo.cLCDBuff[2], "3:Normal Close  ");
	sprintf(sLCDInfo.cLCDBuff[3], "4:Alert Close   ");
	sprintf(sLCDInfo.cLCDBuff[4], "5:MasterPW Open ");
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_ExecCmd
 *
 * DESCRIPTION:通常コマンド実行
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 * teAppCommand     eCmd            R   実行コマンド
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_ExecCmd(tsAppProcessInfo *psProcInfo, teAppCommand eCmd) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 0;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;		// カーソル位置（列）
			psProcInfo->u32Param_0     = 0;		// 処理ステップ
		}
		// 画面制御
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	}

	//==========================================================================
	// ステータス問合せ処理
	//==========================================================================
	tsWirelessMsg* psWlsMsg = &sTxRxTrnsInfo.sWlsMsg;
	tsRxInfo sRxInfo;
	switch (psProcInfo->u32Param_0) {
	case 0:
		//----------------------------------------------------------------------
		// 処理中メッセージ表示
		//----------------------------------------------------------------------
		// 処理ステップ更新
		psProcInfo->u32Param_0 = 1;
		break;
	case 1:
		//----------------------------------------------------------------------
		// ステータス要求コマンド送信処理
		//----------------------------------------------------------------------
		// 自デバイス情報読み込み
		bEEPROMReadDevInfo(&sDevInfo);
		// 電文送信
		memset(psWlsMsg, 0x00, sizeof(tsWirelessMsg));
		// 宛先アドレス
		psWlsMsg->u32DstAddr = sRemoteInfoMain.u32DeviceID;
		// ステータス要求
		psWlsMsg->u8Command = eCmd;
		// CRC8編集
		psWlsMsg->u8CRC = u8CCITT8((uint8*)psWlsMsg, TX_REC_SIZE);
		// 送信
		if (bWirelessTx(sDevInfo.u32DeviceID, FALSE, (uint8*)psWlsMsg, TX_REC_SIZE) == FALSE) {
			vProc_SetMessage("Command Exec Err", "Transmition     ");
			iEEPROMWriteLog(E_MSG_CD_TX_ERR, psWlsMsg);
			return;
		}
		// 受信待ち設定
		vWirelessRxEnabled(sDevInfo.u32DeviceID);
		// タイムアウト時刻設定
		psProcInfo->u32Param_1 = u32TickCount_ms + RX_TIMEOUT_S;
		// 処理ステップ更新
		psProcInfo->u32Param_0 = 2;
		return;
	case 2:
		//----------------------------------------------------------------------
		// ステータス返信待ち
		//----------------------------------------------------------------------
		// タイムアウト判定
		if (psProcInfo->u32Param_1 <= u32TickCount_ms) {
			vProc_SetMessage("Command Exec Err", "Response Timeout");
			iEEPROMWriteLog(E_MSG_CD_RX_TIMEOUT_ERR, psWlsMsg);
			return;
		}
		// 受信バッファチェック
		while (bWirelessRxFetch(&sRxInfo)) {
			// 送信元判定
			if (sRxInfo.u32SrcAddr != sRemoteInfoMain.u32DeviceID) {
				continue;
			}
			// 受信無効化
			vWirelessRxDisabled();
			// 受信メッセージ
			sTxRxTrnsInfo.sWlsMsg = sRxInfo.sMsg;
			// 処理ステップ更新
			psProcInfo->u32Param_0 = 3;
		}
		return;
	case 3:
		//----------------------------------------------------------------------
		// レスポンスデータチェック
		//----------------------------------------------------------------------
		if (bProc_DefaultChkRx(psWlsMsg, E_APP_CMD_ACK) == FALSE) {
			return;
		}
		// ステータス判定
		if (psWlsMsg->u8StatusMap != sRemoteInfoMain.u8StatusMap) {
			sRemoteInfoMain.u8StatusMap = psWlsMsg->u8StatusMap;
			iEEPROMWriteRemoteInfo(&sRemoteInfoMain);
			iEEPROMWriteLog(E_MSG_CD_RX_STS_ERR, psWlsMsg);
		}
		// ステータス情報を表示
		sprintf(sLCDInfo.cLCDBuff[0], "Status:%08d ", (int)u32ValUtil_u8ToBinary(psWlsMsg->u8StatusMap));
		sprintf(sLCDInfo.cLCDBuff[1], "%04d/%02d/%02d %02d:%02d",
				(int)psWlsMsg->u16Year, (int)psWlsMsg->u8Month, (int)psWlsMsg->u8Day,
				(int)psWlsMsg->u8Hour, (int)psWlsMsg->u8Minute);
		vProc_SetMessage(sLCDInfo.cLCDBuff[0], sLCDInfo.cLCDBuff[1]);
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "Please wait...  ");
	sprintf(sLCDInfo.cLCDBuff[1], "Exec command... ");

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_ExecAuthCmd
 *
 * DESCRIPTION:認証コマンド実行
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 * teAppCommand     eCmd            R   実行コマンド
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_ExecAuthCmd(tsAppProcessInfo *psProcInfo, teAppCommand eCmd) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 0;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;		// カーソル位置（列）
			psProcInfo->u32Param_0     = 0;		// 処理ステップ
		}
		// 画面制御
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	}

	//==========================================================================
	// ステータス問合せ処理
	//==========================================================================
	uint32 u32WkRefMin;	// 基準時刻
	uint16 u16StCnt;	// ストレッチング回数
	tsWirelessMsg* psWlsMsg;
	psWlsMsg = &sTxRxTrnsInfo.sWlsMsg;
	tsAES_state sAES_state;
	tsRxInfo sRxInfo;	// 受信データ情報
	switch (psProcInfo->u32Param_0) {
	case 0:
		//----------------------------------------------------------------------
		// ステータス要求コマンド送信処理
		//----------------------------------------------------------------------
		// 自デバイス情報読み込み
		bEEPROMReadDevInfo(&sDevInfo);
		// 電文の編集と送信
		memset(psWlsMsg, 0x00, sizeof(tsWirelessMsg));
		psWlsMsg->u32DstAddr = sRemoteInfoMain.u32DeviceID;
		psWlsMsg->u8Command = E_APP_CMD_CHK_STATUS;
		// CRC8編集
		psWlsMsg->u8CRC = u8CCITT8((uint8*)psWlsMsg, TX_REC_SIZE);
		// 送信
		if (bWirelessTx(sDevInfo.u32DeviceID, FALSE, (uint8*)psWlsMsg, TX_REC_SIZE) == FALSE) {
			vProc_SetMessage("Command Exec Err", "Transmition     ");
			iEEPROMWriteLog(E_MSG_CD_TX_AUTH_CMD_ERR, psWlsMsg);
			return;
		}
		// 受信待ち設定
		vWirelessRxEnabled(sDevInfo.u32DeviceID);
		// タイムアウト時刻設定
		psProcInfo->u32Param_1 = u32TickCount_ms + RX_TIMEOUT_L;
		// 処理ステップ更新
		psProcInfo->u32Param_0++;
		break;
	case 1:
		//----------------------------------------------------------------------
		// ステータス返信待ち
		//----------------------------------------------------------------------
		// タイムアウト判定
		if (psProcInfo->u32Param_1 <= u32TickCount_ms) {
			vProc_SetMessage("Command Exec Err", "Response Timeout");
			// 受信無効化
			vWirelessRxDisabled();
			iEEPROMWriteLog(E_MSG_CD_RX_TIMEOUT_ERR, psWlsMsg);
			return;
		}
		// 受信バッファチェック
		while (bWirelessRxFetch(&sRxInfo)) {
			// 送信元判定
			if (sRxInfo.u32SrcAddr != sRemoteInfoMain.u32DeviceID) {
				continue;
			}
			// 受信無効化
			vWirelessRxDisabled();
			// 受信メッセージ
			sTxRxTrnsInfo.sWlsMsg = sRxInfo.sMsg;
			// 処理ステップ更新
			psProcInfo->u32Param_0++;
		}
		return;
	case 2:
		//----------------------------------------------------------------------
		// レスポンスデータチェック
		//----------------------------------------------------------------------
		// 基本的なチェックを実施
		if (bProc_DefaultChkRx(psWlsMsg, E_APP_CMD_ACK) == FALSE) {
			return;
		}
		// ステータス判定
		if (psWlsMsg->u8StatusMap != sRemoteInfoMain.u8StatusMap) {
			sRemoteInfoMain.u8StatusMap = psWlsMsg->u8StatusMap;
			iEEPROMWriteRemoteInfo(&sRemoteInfoMain);
			iEEPROMWriteLog(E_MSG_CD_RX_STS_ERR, psWlsMsg);
		}
		// 基準日時を算出
		u32WkRefMin = u32ValUtil_dateToDays(psWlsMsg->u16Year, psWlsMsg->u8Month, psWlsMsg->u8Day) * 1440;
		u32WkRefMin = u32WkRefMin + psWlsMsg->u8Hour * 60;
		u32WkRefMin = u32WkRefMin + psWlsMsg->u8Minute;
		// 基準日時をチェック
		if (u32WkRefMin < sRemoteInfoMain.u32StartDateTime) {
			vProc_SetMessage("Command Exec Err", "Response Time   ");
			iEEPROMWriteLog(E_MSG_CD_RX_DATETIME_ERR, psWlsMsg);
			return;
		}
		// 次回送信時刻
		psProcInfo->u32Param_1 = u32TickCount_ms;
		// 秒数判定
		if (psWlsMsg->u8Second >= 59) {
			// 59秒以降の場合には１秒ウェイトするので加算
			u32WkRefMin++;
			psProcInfo->u32Param_1 = psProcInfo->u32Param_1 + 1000;
		}
		// 基準時刻保持
		sTxRxTrnsInfo.u32RefMin = u32WkRefMin;
		//----------------------------------------------------------------------
		// ワンタイムトークン生成
		//----------------------------------------------------------------------
		// ワンタイム乱数生成
		sTxRxTrnsInfo.u32OneTimeVal = u32ValUtil_getRandVal();
		// 同期トークンとワンタイム乱数を元にハッシュ関数を利用してワンタイムトークンを生成
		u16StCnt = (sTxRxTrnsInfo.u32OneTimeVal % (256 - APP_HASH_STRETCHING_CNT_MIN)) + APP_HASH_STRETCHING_CNT_MIN;
		sHashGenInfo = sAuth_generateHashInfo(sRemoteInfoMain.u8SyncToken, u16StCnt);
		sHashGenInfo.u32ShufflePtn = sTxRxTrnsInfo.u32OneTimeVal;
		// ハッシュ化処理をバックグラウンドプロセスとして起動
		iEntrySeqEvt(E_EVENT_APP_HASH_ST);
		// 処理待ちに移行
		psProcInfo->u32Param_0++;
		return;
	case 3:
		//----------------------------------------------------------------------
		// ハッシュ値生成処理の完了判定
		//----------------------------------------------------------------------
		// 待ち時間判定
		if (u32TickCount_ms < psProcInfo->u32Param_1) {
			return;
		}
		// ハッシュ生成完了判定
		if (sHashGenInfo.eStatus == E_AUTH_HASH_PROC_COMPLETE) {
			// ワンタイムトークンの編集
			memcpy(sTxRxTrnsInfo.u8OneTimeTkn, sHashGenInfo.u8HashCode, APP_AUTH_TOKEN_SIZE);
			// 処理待ちに移行
			psProcInfo->u32Param_0++;
		}
		return;
	case 4:
		//----------------------------------------------------------------------
		// 認証要求コマンド送信処理
		//----------------------------------------------------------------------
		// 電文編集
		memset(psWlsMsg, 0x00, sizeof(tsWirelessMsg));
		psWlsMsg->u32DstAddr = sRemoteInfoMain.u32DeviceID;		// 宛先アドレス
		psWlsMsg->u8Command  = eCmd;							// コマンド
		psWlsMsg->u32SyncVal = sTxRxTrnsInfo.u32OneTimeVal;		// ワンタイム乱数
		// ストレッチング回数
		psWlsMsg->u8AuthStCnt = sRemoteInfoMain.u8SndStretching;
		// 認証トークンに認証ハッシュを編集
		memcpy(psWlsMsg->u8AuthToken, sRemoteInfoMain.u8AuthHash, APP_AUTH_TOKEN_SIZE);
		// 暗号化領域の暗号化
		sAES_state = vAES_newCBCState(AES_KEY_LEN_256, sTxRxTrnsInfo.u8OneTimeTkn, sTxRxTrnsInfo.u8OneTimeTkn);
		vAES_encrypt(&sAES_state, &psWlsMsg->u8AuthStCnt, 80);
		// CRC8編集
		psWlsMsg->u8CRC = u8CCITT8((uint8*)psWlsMsg, TX_REC_SIZE);
		// 送信
		if (bWirelessTx(sDevInfo.u32DeviceID, FALSE, (uint8*)psWlsMsg, TX_REC_SIZE) == FALSE) {
			vProc_SetMessage("Command Exec Err", "Transmition     ");
			iEEPROMWriteLog(E_MSG_CD_TX_AUTH_CMD_ERR, psWlsMsg);
			return;
		}
		// 受信待ち設定
		vWirelessRxEnabled(sDevInfo.u32DeviceID);
		// タイムアウト時刻設定
		psProcInfo->u32Param_1 = u32TickCount_ms + RX_TIMEOUT_L;
		// 処理ステップ更新
		psProcInfo->u32Param_0++;
		return;
	case 5:
		//----------------------------------------------------------------------
		// ACK返信待ち
		//----------------------------------------------------------------------
		// タイムアウト判定
		if (psProcInfo->u32Param_1 <= u32TickCount_ms) {
			// 受信無効化
			vWirelessRxDisabled();
			// エラーメッセージ表示
			vProc_SetMessage("Command Exec Err", "Response Timeout");
			iEEPROMWriteLog(E_MSG_CD_RX_TIMEOUT_ERR, psWlsMsg);
			return;
		}
		// 受信バッファチェック
		while (bWirelessRxFetch(&sRxInfo)) {
			// 送信元判定
			if (sRxInfo.u32SrcAddr != sRemoteInfoMain.u32DeviceID) {
				continue;
			}
			// 受信無効化
			vWirelessRxDisabled();
			// 受信メッセージ
			sTxRxTrnsInfo.sWlsMsg = sRxInfo.sMsg;
			// 処理ステップ更新
			psProcInfo->u32Param_0++;
		}
		return;
	case 6:
		//----------------------------------------------------------------------
		// レスポンスデータチェック
		//----------------------------------------------------------------------
		// 基本的なチェックを実施（暗号化領域を復号化）
		if (bProc_DefaultChkRx(psWlsMsg, E_APP_CMD_AUTH_ACK) == FALSE) {
			return;
		}
		// ステータス判定
		if (psWlsMsg->u8StatusMap != sRemoteInfoMain.u8StatusMap) {
			sRemoteInfoMain.u8StatusMap = psWlsMsg->u8StatusMap;
			iEEPROMWriteRemoteInfo(&sRemoteInfoMain);
			iEEPROMWriteLog(E_MSG_CD_RX_STS_ERR, psWlsMsg);
		}
		// 基準点までの経過時間を算出
		u32WkRefMin = u32ValUtil_dateToDays(psWlsMsg->u16Year, psWlsMsg->u8Month, psWlsMsg->u8Day) * 1440;
		u32WkRefMin = u32WkRefMin + psWlsMsg->u8Hour * 60;
		u32WkRefMin = u32WkRefMin + psWlsMsg->u8Minute;
		// 前回基準日時と比較チェック
		if (u32WkRefMin < sTxRxTrnsInfo.u32RefMin || u32WkRefMin > (sTxRxTrnsInfo.u32RefMin + 1)) {
			vProc_SetMessage("Command Exec Err", "Response Time   ");
			iEEPROMWriteLog(E_MSG_CD_RX_DATETIME_ERR, psWlsMsg);
			return;
		}
		//----------------------------------------------------------------------
		// 認証情報をチェック
		//----------------------------------------------------------------------
		// ストレッチング回数チェック
		if (psWlsMsg->u8AuthStCnt == 0 || psWlsMsg->u8UpdAuthStCnt == 0) {
			iEEPROMWriteLog(E_MSG_CD_RX_ST_CNT_ERR, psWlsMsg);
			return;
		}
		// ストレッチング回数算出
		u16StCnt = sRemoteInfoMain.u8SndStretching + psWlsMsg->u8AuthStCnt;
		u16StCnt = u16StCnt + ((sTxRxTrnsInfo.u32RefMin - sRemoteInfoMain.u32StartDateTime) % STRETCHING_CNT_BASE);
		// ハッシュ生成情報
		sHashGenInfo = sAuth_generateHashInfo(psWlsMsg->u8AuthToken, u16StCnt);
		vAuth_setSyncToken(&sHashGenInfo, sRemoteInfoMain.u8SyncToken);
		// ハッシュ化処理をバックグラウンドプロセスとして起動
		iEntrySeqEvt(E_EVENT_APP_HASH_ST);
		// 処理待ちに移行
		psProcInfo->u32Param_0++;
		return;
	case 7:
		// ハッシュ値復元処理の完了判定
		if (sHashGenInfo.eStatus == E_AUTH_HASH_PROC_COMPLETE) {
			psProcInfo->u32Param_0++;
		}
		return;
	case 8:
		// 認証ハッシュ値の検証
		if (memcmp(sRemoteInfoMain.u8AuthHash, sHashGenInfo.u8HashCode, APP_AUTH_TOKEN_SIZE) != 0) {
			// エラーメッセージ表示
			vProc_SetMessage("Command Exec Err", "Auth Info Error ");
			iEEPROMWriteLog(E_MSG_CD_RX_AUTH_TKN_ERR, psWlsMsg);
			return;
		}
		// 主デバイスにバス切替
		vI2CMainConnect();
		// 認証情報を更新
		sRemoteInfoMain.u8SndStretching = psWlsMsg->u8UpdAuthStCnt;
		memcpy(sRemoteInfoMain.u8AuthHash, psWlsMsg->u8UpdAuthToken, APP_AUTH_TOKEN_SIZE);
		vValUtil_masking(sRemoteInfoMain.u8SyncToken, sTxRxTrnsInfo.u8OneTimeTkn, APP_AUTH_TOKEN_SIZE);
		// 認証情報を書き込み
		if (iEEPROMWriteRemoteInfo(&sRemoteInfoMain) < 0) {
			// エラーメッセージ表示
			vProc_SetMessage("Command Exec Err", "Auth Info Update");
			iEEPROMWriteLog(E_MSG_CD_WRITE_RMT_DEV_ERR, psWlsMsg);
			return;
		}
		// ステータス情報を表示
		sprintf(sLCDInfo.cLCDBuff[0], "Status:%08d ", (int)u32ValUtil_u8ToBinary(psWlsMsg->u8StatusMap));
		sprintf(sLCDInfo.cLCDBuff[1], "%04d/%02d/%02d %02d:%02d",
				(int)psWlsMsg->u16Year, (int)psWlsMsg->u8Month, (int)psWlsMsg->u8Day,
				(int)psWlsMsg->u8Hour, (int)psWlsMsg->u8Minute);
		vProc_SetMessage(sLCDInfo.cLCDBuff[0], sLCDInfo.cLCDBuff[1]);
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "Please wait...  ");
	sprintf(sLCDInfo.cLCDBuff[1], "Exec command... ");

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_ExecMstCmd_01
 *
 * DESCRIPTION:マスター認証コマンド実行（リダイレクト）
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_ExecMstCmd_01(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// リダイレクト処理
	//==========================================================================
	// マスターパスワード入力に遷移
	vProc_SetInputToken(E_PROCESS_EXEC_CMD_MPW_OPEN_02, E_PROCESS_EXEC_CMD_MENU, FALSE);
	sprintf(sAppScrParam.cDispMsg[0], "Item :Master PW ");
}

/*******************************************************************************
 *
 * NAME: vProc_ExecMstCmd_02
 *
 * DESCRIPTION:マスター認証コマンド実行（コマンド送信）
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 * teAppCommand     eCmd            R   実行コマンド
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_ExecMstCmd_02(tsAppProcessInfo *psProcInfo, teAppCommand eCmd) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 0;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;		// カーソル位置（列）
			psProcInfo->u32Param_0     = 0;		// 処理ステップ
		}
		// 画面制御
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	}

	//==========================================================================
	// ステータス問合せ処理
	//==========================================================================
	tsWirelessMsg* psWlsMsg = &sTxRxTrnsInfo.sWlsMsg;
	tsRxInfo sRxInfo;	// 受信データ情報
	switch (psProcInfo->u32Param_0) {
	case 0:
		//----------------------------------------------------------------------
		// マスター認証要求コマンド送信処理
		//----------------------------------------------------------------------
		// 自デバイス情報読み込み
		bEEPROMReadDevInfo(&sDevInfo);
		// 電文編集
		memset(psWlsMsg, 0x00, sizeof(tsWirelessMsg));
		psWlsMsg->u32DstAddr = sRemoteInfoMain.u32DeviceID;	// 宛先アドレス
		psWlsMsg->u8Command  = eCmd;						// コマンド
		// 認証コード
		memcpy(&psWlsMsg->u8AuthToken[0], sAppScrParam.cDispMsg[1], 16);
		memcpy(&psWlsMsg->u8AuthToken[16], sAppScrParam.cDispMsg[2], 16);
		// CRC8編集
		psWlsMsg->u8CRC = u8CCITT8((uint8*)psWlsMsg, TX_REC_SIZE);
		// 送信
		if (bWirelessTx(sDevInfo.u32DeviceID, FALSE, (uint8*)psWlsMsg, TX_REC_SIZE) == FALSE) {
			vProc_SetMessage("Command Exec Err", "Transmition     ");
			iEEPROMWriteLog(E_MSG_CD_TX_MST_CMD_ERR, psWlsMsg);
			return;
		}
		// 受信待ち設定
		vWirelessRxEnabled(sDevInfo.u32DeviceID);
		// タイムアウト時刻設定
		psProcInfo->u32Param_1 = u32TickCount_ms + RX_TIMEOUT_L;
		// 処理ステップ更新
		psProcInfo->u32Param_0++;
		break;
	case 1:
		//----------------------------------------------------------------------
		// ACK返信待ち
		//----------------------------------------------------------------------
		// タイムアウト判定
		if (psProcInfo->u32Param_1 <= u32TickCount_ms) {
			// 受信無効化
			vWirelessRxDisabled();
			// エラーメッセージ表示
			vProc_SetMessage("Command Exec Err", "Response Timeout");
			iEEPROMWriteLog(E_MSG_CD_RX_TIMEOUT_ERR, psWlsMsg);
			return;
		}
		// 受信バッファチェック
		while (bWirelessRxFetch(&sRxInfo)) {
			// 送信元判定
			if (sRxInfo.u32SrcAddr != sRemoteInfoMain.u32DeviceID) {
				continue;
			}
			// 受信無効化
			vWirelessRxDisabled();
			// 受信メッセージ
			sTxRxTrnsInfo.sWlsMsg = sRxInfo.sMsg;
			// 処理ステップ更新
			psProcInfo->u32Param_0++;
		}
		return;
	case 2:
		//----------------------------------------------------------------------
		// レスポンスデータチェック
		//----------------------------------------------------------------------
		// 基本的なチェックを実施
		if (!bProc_DefaultChkRx(psWlsMsg, E_APP_CMD_ACK)) {
			return;
		}
		// ステータス判定
		if (psWlsMsg->u8StatusMap != sRemoteInfoMain.u8StatusMap) {
			sRemoteInfoMain.u8StatusMap = psWlsMsg->u8StatusMap;
			iEEPROMWriteRemoteInfo(&sRemoteInfoMain);
			iEEPROMWriteLog(E_MSG_CD_RX_STS_ERR, psWlsMsg);
		}
		// ステータス情報を表示
		sprintf(sLCDInfo.cLCDBuff[0], "Status:%08d ", (int)u32ValUtil_u8ToBinary(psWlsMsg->u8StatusMap));
		sprintf(sLCDInfo.cLCDBuff[1], "%04d/%02d/%02d %02d:%02d",
				(int)psWlsMsg->u16Year, (int)psWlsMsg->u8Month, (int)psWlsMsg->u8Day,
				(int)psWlsMsg->u8Hour, (int)psWlsMsg->u8Minute);
		vProc_SetMessage(sLCDInfo.cLCDBuff[0], sLCDInfo.cLCDBuff[1]);
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "Please wait...  ");
	sprintf(sLCDInfo.cLCDBuff[1], "Exec command... ");

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_ShowMsg
 *
 * DESCRIPTION:メッセージ表示
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_ShowMsg(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;		// 表示位置
			psProcInfo->u8CursorPosRow = 0;		// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;		// カーソル位置（列）
		}
		// 画面制御
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	// OKボタン処理
	if (u8Key == '*') {
		switch (sAppScrParam.eChgTypeOK) {
		case E_PROCESS_CHG_PREV:
			// 起動元画面へ遷移
			vProc_Prev();
			break;
		case E_PROCESS_CHG_REPLACE:
			// プロセス置き換え
			vProc_Replace(sAppScrParam.eProcessIdOK);
			break;
		case E_PROCESS_CHG_NEXT:
			// 次プロセスへ遷移
			vProc_Next(sAppScrParam.eProcessIdOK);
			break;
		}
		return;
	}
	// キャンセルボタン処理
	if (u8Key == '#') {
		switch (sAppScrParam.eChgTypeCANCEL) {
		case E_PROCESS_CHG_PREV:
			// 起動元画面へ遷移
			vProc_Prev();
			break;
		case E_PROCESS_CHG_REPLACE:
			// プロセス置き換え
			vProc_Replace(sAppScrParam.eProcessIdCANCEL);
			break;
		case E_PROCESS_CHG_NEXT:
			// 次プロセスへ遷移
			vProc_Next(sAppScrParam.eProcessIdCANCEL);
			break;
		}
		return;
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sAppScrParam.cDispMsg[0][16] = '\0';
	sAppScrParam.cDispMsg[1][16] = '\0';
	sprintf(sLCDInfo.cLCDBuff[0], sAppScrParam.cDispMsg[0]);
	sprintf(sLCDInfo.cLCDBuff[1], sAppScrParam.cDispMsg[1]);

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_SelectRemoteDev
 *
 * DESCRIPTION:リモートデバイス情報選択画面への遷移パラメータ設定
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_SelectRemoteDev(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// リモートデバイス情報
			int iIdx = iEEPROMReadRemoteInfo(&sRemoteInfoMain, 0, TRUE);
			if (iIdx == -1) {
				vProc_SetMessage("I2C Access Err!!", "Index Info      ");
				return;
			}
			if (iIdx == -2) {
				vProc_SetMessage("Remote Dev Info ", "Data Not Found!!");
				return;
			}
			if (iIdx < 0) {
				vProc_SetMessage("I2C Access Err!!", "Remote Dev Info ");
				return;
			}
			psProcInfo->u32Param_0 = iIdx;
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow    = 0;	// 表示位置
			psProcInfo->u8CursorPosRow   = 0;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol   = 0;	// カーソル位置（列）
		}
		// 画面制御情報初期化
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	// OKボタン処理
	if (u8Key == '*') {
		switch (sAppScrParam.eChgTypeOK) {
		case E_PROCESS_CHG_PREV:
			// 起動元画面へ遷移
			vProc_Prev();
			break;
		case E_PROCESS_CHG_REPLACE:
			// プロセス置き換え
			vProc_Replace(sAppScrParam.eProcessIdOK);
			break;
		case E_PROCESS_CHG_NEXT:
			// 次プロセスへ遷移
			vProc_Next(sAppScrParam.eProcessIdOK);
			break;
		}
		return;
	}
	// キャンセルボタン処理
	if (u8Key == '#') {
		switch (sAppScrParam.eChgTypeCANCEL) {
		case E_PROCESS_CHG_PREV:
			// 起動元画面へ遷移
			vProc_Prev();
			break;
		case E_PROCESS_CHG_REPLACE:
			// プロセス置き換え
			vProc_Replace(sAppScrParam.eProcessIdCANCEL);
			break;
		case E_PROCESS_CHG_NEXT:
			// 次プロセスへ遷移
			vProc_Next(sAppScrParam.eProcessIdCANCEL);
			break;
		}
		return;
	}
	// リモートデバイスの選択処理
	int iIdx = psProcInfo->u32Param_0;
	if (u8Key == '2' || u8Key == '3') {
		// 直前のリモートデバイス情報の読み込み
		iIdx = (iIdx + MAX_REMOTE_DEV_CNT - 1) % MAX_REMOTE_DEV_CNT;
		iIdx = iEEPROMReadRemoteInfo(&sRemoteInfoMain, iIdx, FALSE);
	} else if (u8Key == '8' || u8Key == '9') {
		// 次のリモートデバイス情報の読み込み
		iIdx = (iIdx + 1) % MAX_REMOTE_DEV_CNT;
		iIdx = iEEPROMReadRemoteInfo(&sRemoteInfoMain, iIdx, TRUE);
	}
	if (iIdx < 0) {
		// エラーメッセージを表示してリモートデバイス情報メニューへ戻る
		vProc_SetMessage("I2C Access Err!!", "Remote Dev Info ");
		return;
	}
	psProcInfo->u32Param_0 = iIdx;

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "DevID:%08u  ", (int)sRemoteInfoMain.u32DeviceID);
	sprintf(sLCDInfo.cLCDBuff[1], "Name :%10s", sRemoteInfoMain.cDeviceName);

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vProc_InputToken
 *
 * DESCRIPTION:パスワード等のトークン入力画面
 *
 * PARAMETERS:      Name            RW  Usage
 * tsProcessInfo*   psProcInfo      R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vProc_InputToken(tsAppProcessInfo *psProcInfo) {
	//==========================================================================
	// プロセス初期処理
	//==========================================================================
	uint8 u8Key = u8ReadKeyPad();
	if (psProcInfo->eProcStatus != E_PROCESS_STATE_RUNNING) {
		// 初期化判定
		if (psProcInfo->eProcStatus == E_PROCESS_STATE_BEGIN) {
			// カーソル位置初期化
			sLCDInfo.u8CurrentDispRow  = 0;	// 表示位置
			psProcInfo->u8CursorPosRow = 1;	// カーソル位置（行）
			psProcInfo->u8CursorPosCol = 0;	// カーソル位置（列）
		}
		// 画面制御情報更新
		uint8 u8CntrNone = u8LCDTypeToByte(LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE, LCD_CTR_NONE);
		uint8 u8CntrInput =
			u8LCDTypeToByte(LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT, LCD_CTR_BLINK_INPUT);
		uint8 u8Col;
		for (u8Col = 0; u8Col < 4; u8Col++) {
			sLCDInfo.u8LCDCntr[0][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[1][u8Col] = u8CntrInput;
			sLCDInfo.u8LCDCntr[2][u8Col] = u8CntrInput;
			sLCDInfo.u8LCDCntr[3][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[4][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[5][u8Col] = u8CntrNone;
			sLCDInfo.u8LCDCntr[6][u8Col] = u8CntrNone;
		}
		// カーソル位置設定
		bLCDCursorSet(psProcInfo->u8CursorPosRow, psProcInfo->u8CursorPosCol);
		// ステータス更新
		psProcInfo->eProcStatus = E_PROCESS_STATE_RUNNING;
	} else if (u8Key == (uint8)NULL) {
		return;
	}

	//==========================================================================
	// キー判定
	//==========================================================================
	// OKボタン処理
	if (u8Key == '*') {
		switch (sAppScrParam.eChgTypeOK) {
		case E_PROCESS_CHG_PREV:
			// 起動元画面へ遷移
			vProc_Prev();
			break;
		case E_PROCESS_CHG_REPLACE:
			// プロセス置き換え
			vProc_Replace(sAppScrParam.eProcessIdOK);
			break;
		case E_PROCESS_CHG_NEXT:
			// 次プロセスへ遷移
			vProc_Next(sAppScrParam.eProcessIdOK);
			break;
		}
		return;
	}
	// キャンセルボタン処理
	if (u8Key == '#') {
		switch (sAppScrParam.eChgTypeCANCEL) {
		case E_PROCESS_CHG_PREV:
			// 起動元画面へ遷移
			vProc_Prev();
			break;
		case E_PROCESS_CHG_REPLACE:
			// プロセス置き換え
			vProc_Replace(sAppScrParam.eProcessIdCANCEL);
			break;
		case E_PROCESS_CHG_NEXT:
			// 次プロセスへ遷移
			vProc_Next(sAppScrParam.eProcessIdCANCEL);
			break;
		}
		return;
	}
	// 他のボタン処理
	if (u8Key != (uint8)NULL) {
		bLCDWriteChar(u8Key);
		psProcInfo->u8CursorPosRow = sLCDInfo.u8CursorPosRow;
		psProcInfo->u8CursorPosCol = sLCDInfo.u8CursorPosCol;
		memcpy(sAppScrParam.cDispMsg[1], sLCDInfo.cLCDBuff[1], LCD_BUFF_COL_SIZE);
		memcpy(sAppScrParam.cDispMsg[2], sLCDInfo.cLCDBuff[2], LCD_BUFF_COL_SIZE);
	}

	//==========================================================================
	// 画面編集
	//==========================================================================
	sprintf(sLCDInfo.cLCDBuff[0], "%16s", sAppScrParam.cDispMsg[0]);
	sprintf(sLCDInfo.cLCDBuff[1], "%16s", sAppScrParam.cDispMsg[1]);
	sprintf(sLCDInfo.cLCDBuff[2], "%16s", sAppScrParam.cDispMsg[2]);
	sLCDInfo.cLCDBuff[0][LCD_BUFF_COL_SIZE] = '\0';
	sLCDInfo.cLCDBuff[1][LCD_BUFF_COL_SIZE] = '\0';
	sLCDInfo.cLCDBuff[2][LCD_BUFF_COL_SIZE] = '\0';

	//==========================================================================
	// LCD描画
	//==========================================================================
	// LCD描画イベント
	iEntrySeqEvt(E_EVENT_APP_LCD_DRAWING);
}

/*******************************************************************************
 *
 * NAME: vEventHashStretching
 *
 * DESCRIPTION:拡張ハッシュストレッチング処理イベントプロセス
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEventHashStretching(uint32 u32EvtTimeMs) {
	// ハッシュ関数実行
	if (bAuth_hashStretching(&sHashGenInfo)) {
		return;
	}
	// 次回ストレッチング処理
	iEntrySeqEvt(E_EVENT_APP_HASH_ST);
}

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: vProc_Prev
 *
 * DESCRIPTION:前画面プロセスへ遷移
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PRIVATE void vProc_Prev() {
	if (sAppInfo.u8ProcessLayer <= 0) {
		return;
	}
	// 次の画面階層へ遷移
	sAppInfo.u8ProcessLayer--;
	tsAppProcessInfo *psProcessInfo = &sProcessInfoList[sAppInfo.u8ProcessLayer];
	psProcessInfo->eProcStatus      = E_PROCESS_STATE_RETURN;
}

/*******************************************************************************
 *
 * NAME: vProc_Replace
 *
 * DESCRIPTION:他画面プロセスへ置き換え遷移
 *
 * PARAMETERS:      Name            RW  Usage
 * teAppProcess     eAppProcess     R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PRIVATE void vProc_Replace(teAppProcess eAppProcess) {
	// 次の画面階層へ遷移
	tsAppProcessInfo *psProcessInfo = &sProcessInfoList[sAppInfo.u8ProcessLayer];
	psProcessInfo->eProcessID       = eAppProcess;
	psProcessInfo->eProcStatus      = E_PROCESS_STATE_BEGIN;
}

/*******************************************************************************
 *
 * NAME: vProc_Next
 *
 * DESCRIPTION:次画面プロセスへ遷移
 *
 * PARAMETERS:      Name            RW  Usage
 * teAppProcess     eAppProcess     R   プロセス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PRIVATE void vProc_Next(teAppProcess eAppProcess) {
	if (sAppInfo.u8ProcessLayer >= (PROCESS_LAYER_SIZE - 1)) {
		return;
	}
	// 次の画面階層へ遷移
	sAppInfo.u8ProcessLayer++;
	tsAppProcessInfo *psProcessInfo = &sProcessInfoList[sAppInfo.u8ProcessLayer];
	psProcessInfo->eProcessID       = eAppProcess;
	psProcessInfo->eProcStatus      = E_PROCESS_STATE_BEGIN;
}

/*******************************************************************************
 *
 * NAME: vProc_SetMessage
 *
 * DESCRIPTION:エラーメッセージ設定
 *
 * PARAMETERS:      Name            RW  Usage
 *     char*        pcMsg1          R   1行目の表示メッセージ
 *     char*        pcMsg2          R   2行目の表示メッセージ
 *
 * RETURNS:
 *
 ******************************************************************************/
PRIVATE void vProc_SetMessage(char *pcMsg1, char *pcMsg2) {
	sAppScrParam.eChgTypeOK = E_PROCESS_CHG_PREV;
	sAppScrParam.eChgTypeCANCEL = E_PROCESS_CHG_PREV;
	sprintf(sAppScrParam.cDispMsg[0], pcMsg1);
	sprintf(sAppScrParam.cDispMsg[1], pcMsg2);
	// エラーメッセージ画面に遷移
	vProc_Replace(E_PROCESS_SHOW_MSG);
}

/*******************************************************************************
 *
 * NAME: vProc_SetSelectRemoteDev
 *
 * DESCRIPTION:共通系画面への遷移パラメータ設定
 *
 * PARAMETERS:          Name            RW  Usage
 *   teAppProcess       eProcIdOk       R   プロセスID（OKボタン）
 *
 * RETURNS:
 *
 ******************************************************************************/
PRIVATE void vProc_SetSelectRemoteDev(teAppProcess eProcIdOk) {
	sAppScrParam.eProcessIdOK   = eProcIdOk;
	sAppScrParam.eChgTypeOK     = E_PROCESS_CHG_REPLACE;
	sAppScrParam.eChgTypeCANCEL = E_PROCESS_CHG_PREV;
	vProc_Next(E_PROCESS_SELECT_REMOTE_DEV);
}

/*******************************************************************************
 *
 * NAME: vProc_SetInputToken
 *
 * DESCRIPTION:トークン入力画面への遷移パラメータ設定
 *
 * PARAMETERS:          Name            RW  Usage
 *   teAppProcess       eProcIdOk       R   プロセスID（OKボタン）
 *   teAppProcess       eProcIdCancel   R   プロセスID（CANCELボタン）
 *   bool_t             bPrevFlg        R   戻り先フラグ（TRUE:Prev, FALSE:Replace）
 *
 * RETURNS:
 *
 ******************************************************************************/
PRIVATE void vProc_SetInputToken(teAppProcess eProcIdOk, teAppProcess eProcIdCancel, bool_t bPrevFlg) {
	sAppScrParam.eProcessIdOK     = eProcIdOk;
	sAppScrParam.eChgTypeOK       = E_PROCESS_CHG_REPLACE;
	sAppScrParam.eProcessIdCANCEL = eProcIdCancel;
	if (bPrevFlg) {
		sAppScrParam.eChgTypeCANCEL   = E_PROCESS_CHG_PREV;
	} else {
		sAppScrParam.eChgTypeCANCEL   = E_PROCESS_CHG_REPLACE;
	}
	sprintf(sAppScrParam.cDispMsg[0], "                ");
	sprintf(sAppScrParam.cDispMsg[1], "                ");
	sprintf(sAppScrParam.cDispMsg[2], "                ");
	vProc_Replace(E_PROCESS_INPUT_TOKEN);
}

/*******************************************************************************
 *
 * NAME: bProc_DefaultChkRx
 *
 * DESCRIPTION:受信メッセージ基本チェック
 *
 * PARAMETERS:          Name            RW  Usage
 *   tsWirelessMsg*     psWlsMsg        R   受信メッセージ
 *   uint8              u8Cmd           R   返信コマンド
 *
 * RETURNS:
 *   TRUE:チェックOK
 *
 ******************************************************************************/
PRIVATE bool_t bProc_DefaultChkRx(tsWirelessMsg* psWlsMsg, uint8 u8Cmd) {
	//----------------------------------------------------------------------
	// レスポンスデータチェック
	//----------------------------------------------------------------------
	// CRCチェック
	uint8 u8WkCRC = psWlsMsg->u8CRC;
	psWlsMsg->u8CRC = 0;
	if (u8CCITT8((uint8*)psWlsMsg, TX_REC_SIZE) != u8WkCRC) {
		vProc_SetMessage("Command Exec Err", "Data CRC Check  ");
		iEEPROMWriteLog(E_MSG_CD_RX_CRC_ERR, psWlsMsg);
		return FALSE;
	}
	// コマンドチェック
	if (psWlsMsg->u8Command != u8Cmd) {
		vProc_SetMessage("Command Exec Err", "Response Command");
		iEEPROMWriteLog(E_MSG_CD_RX_CMD_ERR, psWlsMsg);
		return FALSE;
	}
	// 受信メッセージの暗号化された領域を復号化
	if (psWlsMsg->u8Command == E_APP_CMD_AUTH_ACK) {
		tsAES_state sAES_state =
				vAES_newCBCState(AES_KEY_LEN_256, sTxRxTrnsInfo.u8OneTimeTkn, sTxRxTrnsInfo.u8OneTimeTkn);
		vAES_decrypt(&sAES_state, &psWlsMsg->u8AuthStCnt, 80);
	}
	// 日付チェック
	if (bValUtil_validDate(psWlsMsg->u16Year, psWlsMsg->u8Month, psWlsMsg->u8Day) == FALSE) {
		vProc_SetMessage("Command Exec Err", "Response Date   ");
		iEEPROMWriteLog(E_MSG_CD_RX_DATE_ERR, psWlsMsg);
		return FALSE;
	}
	// 時刻チェック
	if (bValUtil_validTime(psWlsMsg->u8Hour, psWlsMsg->u8Minute, psWlsMsg->u8Second) == FALSE) {
		vProc_SetMessage("Command Exec Err", "Response Time   ");
		iEEPROMWriteLog(E_MSG_CD_RX_TIME_ERR, psWlsMsg);
		return FALSE;
	}
	// チェックOK
	return TRUE;
}

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
