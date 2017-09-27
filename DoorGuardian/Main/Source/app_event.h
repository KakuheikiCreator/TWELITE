/*******************************************************************************
 *
 * MODULE :Application Event header file
 *
 * CREATED:2017/05/07 23:49:00
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
#include "ToCoNet.h"
#include "config.h"
#include "config_default.h"
#include "framework.h"
#include "app_auth.h"
#include "app_io.h"

/******************************************************************************/
/***        Macro Definitions                                               ***/
/******************************************************************************/
// Process Layer Size
#define PROCESS_LAYER_SIZE               (4)

// 温度センサー閾値（整数で扱う為に100倍の値）
#define APP_TEMPERATURE_THRESHOLD        (5500)

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

// Application Status Map
typedef enum {
	APP_STS_MAP_IO_ERR       = (0x01),	// IOエラー
	APP_STS_MAP_AUTH_ERR     = (0x02),	// 通常認証エラー
	APP_STS_MAP_MST_AUTH_ERR = (0x04),	// マスター認証エラー
	APP_STS_MAP_IR_SENS      = (0x08),	// 人感センサー
	APP_STS_MAP_BUTTON       = (0x10),	// ボタン
	APP_STS_MAP_OPEN_SENS    = (0x20),	// 開放センサー
	APP_STS_MAP_SERVO_SENS   = (0x40),	// サーボ位置
	APP_STS_MAP_TEMPERATURE  = (0x80)	// 温度センサー
} teAppStatusMap;

// Application Event
typedef enum {
	E_EVENT_EMPTY = ToCoNet_EVENT_APP_BASE,
	E_EVENT_INITIALIZE,
	E_EVENT_UPD_BUFFER,
	E_EVENT_SECOND,
	E_EVENT_RX_PKT_CHK,
	E_EVENT_RX_MST_AUTH_00,
	E_EVENT_RX_MST_AUTH_01,
	E_EVENT_RX_AUTH_00,
	E_EVENT_RX_AUTH_01,
	E_EVENT_RX_AUTH_02,
	E_EVENT_RX_AUTH_03,
	E_EVENT_RX_AUTH_04,
	E_EVENT_TX_DATA,
	E_EVENT_SENSOR_CHK,
	E_EVENT_SETTING_CHK,
	E_EVENT_STS_UNLOCK,
	E_EVENT_STS_LOCK,
	E_EVENT_STS_IN_CAUTION,
	E_EVENT_STS_ALARM_UNLOCK,
	E_EVENT_STS_ALARM_LOCK,
	E_EVENT_STS_ALARM_LOG,
	E_EVENT_STS_MST_UNLOCK,
	E_EVENT_LCD_DRAWING,
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
	E_MSG_CD_RX_ST_CNT_ERR,					// 受信ストレッチング回数エラー
	E_MSG_CD_RX_MST_TKN_ERR,				// 受信マスタートークンエラー
	E_MSG_CD_RX_AUTH_TKN_ERR,				// 受信認証トークンエラー
	E_MSG_CD_RX_STS_UPD,					// 受信ステータス更新エラー
	E_MSG_CD_READ_RMT_DEV_ERR,				// リモートデバイス情報の読み込みエラー
	E_MSG_CD_WRITE_RMT_DEV_ERR,				// リモートデバイス情報の書き込みエラー
	E_MSG_CD_TEMPERATURE_SENS_ERR,			// 温度センサーエラー
	E_MSG_CD_SERVO_ERR,						// サーボ位置エラー
	E_MSG_CD_OPEN_SENS_ERR,					// 開放センサーエラー
	E_MSG_CD_BUTTON_ERR,					// ボタン操作エラー
	E_MSG_CD_IR_SENS_ERR					// 赤外線センサーエラー
} teAppMsgCD;

// 構造体：アプリケーションステータス情報
typedef struct {
	teAppStatus eAppStatus;						// アプリケーション状態
	bool_t bInProgressFlg;						// ステータス移行中フラグ
} tsAppStatusInfo;

// 構造体：アプリケーションイベントマップ
typedef struct {
	teAppEvent eEvtIRSensor;			// 人感センサー反応時
	teAppEvent eEvtTouchSensor	;		// タッチセンサー反応時
	teAppEvent eEvtOpenSensor;			// 開放センサー反応時
	teAppEvent eEvtServoSensor;			// サーボ位置不正時
	teAppEvent eEvtUnlockReq;			// 開錠リクエスト受信時
	teAppEvent eEvtLockReq;				// 施錠リクエスト受信時
	teAppEvent eEvtInCautionReq;		// 警戒施錠リクエスト受信時
	teAppEvent eEvtMstUnlockReq;		// マスターパスワード開錠リクエスト受信時
} tsAppEventMap;

// 構造体：アプリケーションイベント情報
typedef struct {
	uint8 u8UpdStsMap;					// 更新ステータス
	uint8 u8LogMsgCd;					// ログ出力メッセ―ジ
	uint32 u32PwrOffTime;				// 5V電源オフ時刻
} tsAppEventInfo;

// 構造体：送受信トランザクション情報
typedef struct {
	DS3231_datetime sRefDatetime;				// 基準日時
	uint32 u32DstAddr;							// 宛先アドレス
	tsAuthRemoteDevInfo sRemoteInfo;			// リモートデバイス情報
	uint16 u16RespStCnt;						// 返信ストレッチング回数
	uint8 u8OneTimeTkn[APP_AUTH_TOKEN_SIZE];	// ワンタイムトークン
	uint8 u8ResponseTkn[APP_AUTH_TOKEN_SIZE];	// レスポンストークン
	uint8 u8UpdStretchingCntS;					// 更新ストレッチング回数（送信側）
	uint8 u8UpdStretchingCntR;					// 更新ストレッチング回数（受信側）
	uint8 u8UpdateTkn[APP_AUTH_TOKEN_SIZE];		// 更新認証トークン
	uint8 u8UpdSyncTkn[APP_AUTH_TOKEN_SIZE];	// 更新同期トークン
	tsWirelessMsg sRxWlsMsg;					// 受信メッセージ
	teAppEvent eRtnAppEvt;						// 復帰イベント
	teAppEvent eOkAppEvt;						// 認証成功後イベント
} tsAppTxRxTrnsInfo;

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/
/** アプリケーションステータス情報 */
PUBLIC tsAppStatusInfo sAppStsInfo;
/** アプリケーションイベントマップ */
PUBLIC tsAppEventMap sAppEventMap;
/** アプリケーションイベントパラメータ */
PUBLIC tsAppEventInfo sAppEventInfo;

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
// イベント処理：各種バッファ更新
PUBLIC void vEvent_UpdBuffer(uint32 u32EvtTimeMs);
// イベント処理：秒間隔イベント処理
PUBLIC void vEvent_Second(uint32 u32EvtTimeMs);
// イベント処理：無線パケット受信チェック処理
PUBLIC void vEvent_RxPacketCheck(uint32 u32EvtTimeMs);
// イベント処理：マスターパスワード認証00
PUBLIC void vEvent_RxMstAuth_00(uint32 u32EvtTimeMs);
// イベント処理：マスターパスワード認証01
PUBLIC void vEvent_RxMstAuth_01(uint32 u32EvtTimeMs);
// イベント処理：通常認証処理00
PUBLIC void vEvent_RxAuth_00(uint32 u32EvtTimeMs);
// イベント処理：通常認証処理01
PUBLIC void vEvent_RxAuth_01(uint32 u32EvtTimeMs);
// イベント処理：通常認証処理02
PUBLIC void vEvent_RxAuth_02(uint32 u32EvtTimeMs);
// イベント処理：通常認証処理03
PUBLIC void vEvent_RxAuth_03(uint32 u32EvtTimeMs);
// イベント処理：通常認証処理04
PUBLIC void vEvent_RxAuth_04(uint32 u32EvtTimeMs);
// イベント処理：通常認証処理05
PUBLIC void vEvent_RxAuth_05(uint32 u32EvtTimeMs);
// イベント処理：無線パケット送信処理
PUBLIC void vEvent_TxData(uint32 u32EvtTimeMs);
// イベント処理：センサーチェック処理
PUBLIC void vEvent_SensorCheck(uint32 u32EvtTimeMs);
// イベント処理：設定チェック処理
PUBLIC void vEvent_SettingCheck(uint32 u32EvtTimeMs);
// イベント処理：通常開錠状態への移行処理
PUBLIC void vEvent_StsUnlock(uint32 u32EvtTimeMs);
// イベント処理：通常施錠状態への移行処理
PUBLIC void vEvent_StsLock(uint32 u32EvtTimeMs);
// イベント処理：警戒施錠状態への移行処理
PUBLIC void vEvent_StsInCaution(uint32 u32EvtTimeMs);
// イベント処理：警報開錠状態への移行処理
PUBLIC void vEvent_StsAlarmUnlock(uint32 u32EvtTimeMs);
// イベント処理：警報施錠状態への移行処理
PUBLIC void vEvent_StsAlarmLock(uint32 u32EvtTimeMs);
// イベント処理：警報ログ出力処理
PUBLIC void vEvent_StsAlarmtLog(uint32 u32EvtTimeMs);
// イベント処理：マスターパスワード開錠状態への移行処理
PUBLIC void vEvent_StsMstUnlock(uint32 u32EvtTimeMs);
// イベント処理：LCD描画
PUBLIC void vEvent_LCDdrawing(uint32 u32EvtTimeMs);
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
