/*******************************************************************************
 *
 * MODULE :Application Event header file
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
#ifndef APP_EVENT_H_INCLUDED
#define APP_EVENT_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/******************************************************************************/
/***        Include files                                                   ***/
/******************************************************************************/
#include "app_io.h"

/******************************************************************************/
/***        Macro Definitions                                               ***/
/******************************************************************************/

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/
// Application Status
typedef enum {
	E_APP_STS_INITIALIZE,
	E_APP_STS_UNLOCK,
	E_APP_STS_LOCK,
	E_APP_STS_IN_CAUTION,
	E_APP_STS_ALARM_UNLOCK,
	E_APP_STS_ALARM_LOCK,
	E_APP_STS_MST_UNLOCK
} teAppStatus;

// Application Event
typedef enum {
	E_EVENT_EMPTY = ToCoNet_EVENT_APP_BASE,
	E_EVENT_INITIALIZE,
	E_EVENT_CHK_BTN,
	E_EVENT_SCR_SEL_DEV_INIT,
	E_EVENT_SCR_SEL_DEV_CHG,
	E_EVENT_SCR_SEL_CMD_INIT,
	E_EVENT_SCR_SEL_CMD_CHG,
	E_EVENT_EXEC_CMD_0,
	E_EVENT_EXEC_CMD_1,
	E_EVENT_EXEC_AUTH_CMD_0,
	E_EVENT_EXEC_AUTH_CMD_1,
	E_EVENT_EXEC_AUTH_CMD_2,
	E_EVENT_EXEC_AUTH_CMD_3,
	E_EVENT_EXEC_AUTH_CMD_4,
	E_EVENT_EXEC_AUTH_CMD_5,
	E_EVENT_RX_TIMEOUT,
	E_EVENT_HASH_ST
} teAppEvent;

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

// 構造体：アプリケーションイベントマップ
typedef struct {
	teAppEvent eEvtBtn_0;				// ボタン０
	teAppEvent eEvtBtn_1;				// ボタン１
	teAppEvent eEvtBtn_2;				// ボタン２
	teAppEvent eEvtBtn_3;				// ボタン３
	teAppEvent eEvtComplete;			// 処理完了
	teAppEvent eEvtTimeout;				// タイムアウト
} tsAppEventMap;

// 構造体：送受信トランザクション情報
typedef struct {
	teAppCommand eCommand;						// 実行コマンド
	int iRemoteInfoIdx;							// リモートデバイス情報のインデックス
	tsAuthRemoteDevInfo sRemoteInfo;			// リモートデバイス情報
	uint32 u32RefMin;							// 基準時刻
	int iTimeoutEvtID;							// タイムアウトイベントID
	uint32 u32OneTimeVal;						// ワンタイム乱数
	uint8 u8OneTimeTkn[APP_AUTH_TOKEN_SIZE];	// ワンタイムトークン
	tsWirelessMsg sRxMsg;						// 受信メッセージ
	tsWirelessMsg sTxMsg;						// 送信メッセージ
} tsAppTxRxTrnsInfo;

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/
/** アプリケーションイベントマップ */
PUBLIC tsAppEventMap sAppEventMap;
/** 送受信トランザクション情報 */
PUBLIC tsAppTxRxTrnsInfo sAppTxRxTrns;
/** ハッシュ値生成情報 */
PUBLIC tsAuthHashGenState sHashGenInfo;

///** デバッグステータス値 */
#ifdef DEBUG
PUBLIC uint8 u8DebugSts;
PUBLIC uint32 u32DebugVal;
#endif

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/

/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/
// イベント処理の登録
PUBLIC void vEvent_RegistTask();
// イベント処理：初期化
PUBLIC void vEvent_Init(uint32 u32EvtTimeMs);
// イベント処理：ボタン入力イベントチェック
PUBLIC void vEvent_CheckBtn(uint32 u32EvtTimeMs);
// イベント処理：リモートデバイス選択初期処理
PUBLIC void vEvent_SelDev_init(uint32 u32EvtTimeMs);
// イベント処理：リモートデバイス選択
PUBLIC void vEvent_SelDev_chg(uint32 u32EvtTimeMs);
// イベント処理：コマンド選択初期処理
PUBLIC void vEvent_SelCmd_init(uint32 u32EvtTimeMs);
// イベント処理：コマンド選択
PUBLIC void vEvent_SelCmd_chg(uint32 u32EvtTimeMs);
// イベント処理：通常コマンド送信
PUBLIC void vEvent_Exec_Cmd_0(uint32 u32EvtTimeMs);
// イベント処理：通常コマンド返信
PUBLIC void vEvent_Exec_Cmd_1(uint32 u32EvtTimeMs);
// イベント処理：認証コマンド（ステップ０）
PUBLIC void vEvent_Exec_AuthCmd_0(uint32 u32EvtTimeMs);
// イベント処理：認証コマンド（ステップ１）
PUBLIC void vEvent_Exec_AuthCmd_1(uint32 u32EvtTimeMs);
// イベント処理：認証コマンド（ステップ２）
PUBLIC void vEvent_Exec_AuthCmd_2(uint32 u32EvtTimeMs);
// イベント処理：認証コマンド（ステップ３）
PUBLIC void vEvent_Exec_AuthCmd_3(uint32 u32EvtTimeMs);
// イベント処理：認証コマンド（ステップ４）
PUBLIC void vEvent_Exec_AuthCmd_4(uint32 u32EvtTimeMs);
// イベント処理：認証コマンド（ステップ５）
PUBLIC void vEvent_Exec_AuthCmd_5(uint32 u32EvtTimeMs);
// イベント処理：無線パケット受信タイムアウト
PUBLIC void vEvent_RxTimeout(uint32 u32EvtTimeMs);
// 拡張ハッシュストレッチング処理イベントプロセス
PUBLIC void vEvent_HashStretching(uint32 u32EvtTimeMs);

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* APP_EVENT_H_INCLUDED */

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
