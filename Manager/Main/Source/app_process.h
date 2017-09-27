/****************************************************************************
 *
 * MODULE :Application Process header file
 *
 * CREATED:2016/07/10 12:42:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:各画面プロセス毎の処理を実装
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 ****************************************************************************
 * Copyright (c) 2016, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 ****************************************************************************/
#ifndef APP_PROCESS_H_INCLUDED
#define APP_PROCESS_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "ToCoNet.h"
#include "app_auth.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// Process Layer Size
#define PROCESS_LAYER_SIZE               (4)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// Process
typedef enum {
	E_PROCESS_HOME,
	E_PROCESS_MAIN_MENU,
	E_PROCESS_DEV_CONNECT,
	E_PROCESS_DATE_TIME_MENU,
	E_PROCESS_DATE_SET,
	E_PROCESS_WEEK_SET,
	E_PROCESS_TIME_SET,
	E_PROCESS_DEV_INFO_MENU,
	E_PROCESS_DEV_INFO_01,
	E_PROCESS_DEV_INFO_02,
	E_PROCESS_DEV_INFO_03,
	E_PROCESS_DEV_INFO_04,
	E_PROCESS_DEV_INFO_05,
	E_PROCESS_DEV_INFO_06,
	E_PROCESS_DEV_EDIT_01,
	E_PROCESS_DEV_EDIT_02,
	E_PROCESS_DEV_EDIT_03,
	E_PROCESS_DEV_EDIT_04,
	E_PROCESS_DEV_EDIT_05,
	E_PROCESS_DEV_STS_CLR,
	E_PROCESS_REMOTE_DEV_MENU,
	E_PROCESS_REMOTE_DEV_INFO,
	E_PROCESS_SYNCHRONIZING,
	E_PROCESS_DEL_REMOTE,
	E_PROCESS_DEL_REMOTE_ALL,
	E_PROCESS_LOG_MENU,
	E_PROCESS_DISPLAY_LOG,
	E_PROCESS_LOG_CLEAR,
	E_PROCESS_EXEC_CMD_MENU,
	E_PROCESS_EXEC_CMD_STATUS,
	E_PROCESS_EXEC_CMD_OPEN,
	E_PROCESS_EXEC_CMD_CLOSE,
	E_PROCESS_EXEC_CMD_ALERT,
	E_PROCESS_EXEC_CMD_MPW_OPEN_01,
	E_PROCESS_EXEC_CMD_MPW_OPEN_02,
	E_PROCESS_SHOW_MSG,
	E_PROCESS_SELECT_REMOTE_DEV,
	E_PROCESS_SELECT_DEV,
	E_PROCESS_INPUT_TOKEN,
	E_PROCESS_END
} teAppProcess;

// Process State
typedef enum {
	E_PROCESS_STATE_BEGIN,
	E_PROCESS_STATE_RUNNING,
	E_PROCESS_STATE_REFRESH,
	E_PROCESS_STATE_RETURN
} teAppProcStatus;

// Application Command
typedef enum {
	E_APP_CMD_CHK_STATUS = (0x00),	// 送受信コマンド：ステータス要求
	E_APP_CMD_UNLOCK     = (0x01),	// 送受信コマンド：開錠要求
	E_APP_CMD_LOCK       = (0x02),	// 送受信コマンド：施錠要求
	E_APP_CMD_ALERT      = (0x03),	// 送受信コマンド：警戒施錠要求
	E_APP_CMD_MST_UNLOCK = (0x7F),	// 送受信コマンド：マスター開錠要求
	E_APP_CMD_ACK        = (0x80),	// 送受信コマンド：認証なしACK
	E_APP_CMD_AUTH_ACK   = (0x81),	// 送受信コマンド：認証ありACK
	E_APP_CMD_NACK       = (0xFF)	// 送受信コマンド：NACK
} teAppCommand;

// Message Code
typedef enum {
	E_MSG_CD_TX_ERR = 0x0001,				// 送信エラー
	E_MSG_CD_TX_AUTH_CMD_ERR,				// 認証コマンド送信エラー
	E_MSG_CD_TX_MST_CMD_ERR,				// マスター認証コマンド送信エラー
	E_MSG_CD_RX_TIMEOUT_ERR,				// 受信タイムアウトエラー
	E_MSG_CD_RX_DATETIME_ERR,				// 受信日時エラー
	E_MSG_CD_RX_CRC_ERR,					// 受信データCRCチェックエラー
	E_MSG_CD_RX_CMD_ERR,					// 受信コマンドエラー
	E_MSG_CD_RX_DATE_ERR,					// 受信日付エラー
	E_MSG_CD_RX_TIME_ERR,					// 受信時刻エラー
	E_MSG_CD_RX_STS_ERR,					// 受信ステータスエラー
	E_MSG_CD_RX_ST_CNT_ERR,					// 受信ストレッチング回数エラー
	E_MSG_CD_RX_MST_TKN_ERR,				// 受信マスタートークンエラー
	E_MSG_CD_RX_AUTH_TKN_ERR,				// 受信認証トークンエラー
	E_MSG_CD_READ_RMT_DEV_ERR,				// リモートデバイス情報の読み込みエラー
	E_MSG_CD_WRITE_RMT_DEV_ERR,				// リモートデバイス情報の書き込みエラー
	E_MSG_CD_TEMPERATURE_SENS_ERR,			// 温度センサーエラー
	E_MSG_CD_SERVO_ERR,						// サーボ位置エラー
	E_MSG_CD_OPEN_SENS_ERR,					// 開放センサーエラー
	E_MSG_CD_BUTTON_ERR,					// ボタン操作エラー
	E_MSG_CD_IR_SENS_ERR					// 赤外線センサーエラー
} teAppMsgCD;

// 構造体：プロセス情報
typedef struct {
	teAppProcess eProcessID;		// 現プロセスID
	teAppProcStatus eProcStatus;	// プロセスステータス
	uint8 u8CursorPosRow;			// カーソル位置（行）
	uint8 u8CursorPosCol;			// カーソル位置（列）
	uint32 u32Param_0;				// 汎用パラメータ１
	uint32 u32Param_1;				// 汎用パラメータ２
	uint32 u32Param_2;				// 汎用パラメータ３
} tsAppProcessInfo;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
// アプリケーションプロセス初期処理
PUBLIC void vProc_Init();
// 各画面ごとのプロセス処理
PUBLIC void vEventProcess(uint32 u32EvtTimeMs);
// 画面プロセス：ホーム
PUBLIC void vProc_Home(tsAppProcessInfo *psProcInfo);
// 画面プロセス：メインメニュー
PUBLIC void vProc_MainMenu(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス接続
PUBLIC void vProc_DevConnect(tsAppProcessInfo *psProcInfo);
// 画面プロセス：日時設定メニュー
PUBLIC void vProc_DateTimeMenu(tsAppProcessInfo *psProcInfo);
// 画面プロセス：日付設定
PUBLIC void vProc_DateSetting(tsAppProcessInfo *psProcInfo);
// 画面プロセス：曜日設定
PUBLIC void vProc_WeekSetting(tsAppProcessInfo *psProcInfo);
// 画面プロセス：時刻設定
PUBLIC void vProc_TimeSetting(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報メニュー
PUBLIC void vProc_DeviceInfoMenu(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報参照
PUBLIC void vProc_DispDeviceInfo(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報参照（デバイスID）
PUBLIC void vProc_DispDeviceInfo_01(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報参照（デバイス名）
PUBLIC void vProc_DispDeviceInfo_02(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報参照（マスターパスワードハッシュ）
PUBLIC void vProc_DispDeviceInfo_03(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報参照（マスターパスワードストレッチング回数）
PUBLIC void vProc_DispDeviceInfo_04(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報参照（通信暗号化パスワード）
PUBLIC void vProc_DispDeviceInfo_05(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報参照（ステータスマップ）
PUBLIC void vProc_DispDeviceInfo_06(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報編集（デバイスID）
PUBLIC void vProc_EditDeviceInfo_01(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報編集（デバイス名）
PUBLIC void vProc_EditDeviceInfo_02(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報編集（マスターパスワードストレッチング回数）
PUBLIC void vProc_EditDeviceInfo_03(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報編集（通信暗号化パスワード）
PUBLIC void vProc_EditDeviceInfo_04(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報編集（EEPROMへの書き込み処理）
PUBLIC void vProc_EditDeviceInfo_05(tsAppProcessInfo *psProcInfo);
// 画面プロセス：デバイス情報ステータスクリア
PUBLIC void vProc_ClearDeviceSts(tsAppProcessInfo *psProcInfo);
// 画面プロセス：リモートデバイス情報メニュー
PUBLIC void vProc_RemoteDevMenu(tsAppProcessInfo *psProcInfo);
// 画面プロセス：リモートデバイス情報表示
PUBLIC void vProc_RemoteDevInfo(tsAppProcessInfo *psProcInfo);
// 画面プロセス：リモートデバイス情報同期処理
PUBLIC void vProc_Synchronizing(tsAppProcessInfo *psProcInfo);
// 画面プロセス：リモートデバイス情報削除（削除確認）
PUBLIC void vProc_DelRemoteDev(tsAppProcessInfo *psProcInfo);
// 画面プロセス：リモートデバイス情報全削除
PUBLIC void vProc_ClearRemoteDev(tsAppProcessInfo *psProcInfo);
// 画面プロセス：イベント履歴メニュー
PUBLIC void vProc_EventLogMenu(tsAppProcessInfo *psProcInfo);
// 画面プロセス：イベント履歴表示
PUBLIC void vProc_DispEventLog(tsAppProcessInfo *psProcInfo);
// 画面プロセス：イベント履歴クリア
PUBLIC void vProc_ClearEventLog(tsAppProcessInfo *psProcInfo);
// 画面プロセス：コマンド実行メニュー
PUBLIC void vProc_ExecCmdMenu(tsAppProcessInfo *psProcInfo);
// 画面プロセス：通常コマンド実行
PUBLIC void vProc_ExecCmd(tsAppProcessInfo *psProcInfo, teAppCommand eCmd);
// 画面プロセス：認証コマンド実行
PUBLIC void vProc_ExecAuthCmd(tsAppProcessInfo *psProcInfo, teAppCommand eCmd);
// 画面プロセス：マスター認証コマンド実行（リダイレクト）
PUBLIC void vProc_ExecMstCmd_01(tsAppProcessInfo *psProcInfo);
// 画面プロセス：マスター認証コマンド実行（コマンド送信）
PUBLIC void vProc_ExecMstCmd_02(tsAppProcessInfo *psProcInfo, teAppCommand eCmd);
// 画面プロセス：メッセージ表示
PUBLIC void vProc_ShowMsg(tsAppProcessInfo *psProcInfo);
// 画面プロセス：リモートデバイス情報の選択
PUBLIC void vProc_SelectRemoteDev(tsAppProcessInfo *psProcInfo);
// 画面プロセス：トークン入力
PUBLIC void vProc_InputToken(tsAppProcessInfo *psProcInfo);
// 拡張ハッシュストレッチング処理イベントプロセス
PUBLIC void vEventHashStretching(uint32 u32EvtTimeMs);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* APP_PROCESS_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
