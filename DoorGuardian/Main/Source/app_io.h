/*******************************************************************************
 *
 * MODULE :Application IO functions header file
 *
 * CREATED:2016/05/30 01:32:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:画面表示やキー入力等の基本的な入出力機能のヘッダファイル
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 *******************************************************************************
 * Copyright (c) 2016, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 ******************************************************************************/
#ifndef APP_IO_H_INCLUDED
#define APP_IO_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/******************************************************************************/
/***        Include files                                                   ***/
/******************************************************************************/
#include "ToCoNet.h"
#include "app_auth.h"
#include "io_util.h"
#include "st7032i.h"
#include "ds3231.h"
#include "eeprom.h"

/******************************************************************************/
/***        Macro Definitions                                               ***/
/******************************************************************************/
// DI Check Cnt
#define DI_BUFF_CHK_CNT              (5)
// ADC Check Cnt
#define ADC_BUFF_CHK_CNT             (3)
// ADC Check Tolerance
#define ADC_CHK_TOLERANCE            (20)

// Pin No:サーボチェック
#define PIN_NO_SERVO_CHK             (E_AHI_ADC_SRC_ADC_1)
// Pin No:サーボ設定A
#define PIN_NO_SERVO_SET_A           (E_AHI_ADC_SRC_ADC_4)
// Pin No:サーボ設定B
#define PIN_NO_SERVO_SET_B           (E_AHI_ADC_SRC_ADC_3)

// Pin Map:テストスイッチ
#define PIN_MAP_TEST_SW              (0x01 << 16)
// Pin Map:人感センサー
#define PIN_MAP_IR_SENS              (0x01 << 18)
// Pin Map:タッチセンサー
#define PIN_MAP_TOUCH_SENS           (0x01 << 19)
// Pin Map:開放取り外しセンサー
#define PIN_MAP_OPEN_SENS            (0x01 << 11)
// Pin Map:サーボチェック
#define PIN_MAP_SERVO_CHK            (0x01 << PIN_NO_SERVO_CHK)
// Pin Map:サーボ設定A
#define PIN_MAP_SERVO_SET_A          (0x01 << PIN_NO_SERVO_SET_A)
// Pin Map:サーボ設定B
#define PIN_MAP_SERVO_SET_B          (0x01 << PIN_NO_SERVO_SET_B)
// Pin Map:5V電源制御
#define PIN_MAP_5V_CNTR              (0x01 << 9)
// Pin Map:サーボ制御
#define PIN_MAP_SERVO_CNTR           (0x01 << 8)


// I2C Device Type:EEPROM
#define I2C_DEVICE_EEPROM          (0x01)
// I2C Device Type:RTC
#define I2C_DEVICE_RTC             (0x02)

// LCD Row Size
#define LCD_BUFF_ROW_SIZE          (2)
// LCD Column Size
#define LCD_BUFF_COL_SIZE          (16)

// 受信バッファサイズ
#ifndef RX_BUFFER_SIZE
	#define RX_BUFFER_SIZE         (3)
#endif

// 送信バッファサイズ
#ifndef TX_BUFFER_SIZE
	#define TX_BUFFER_SIZE         (3)
#endif

// 受信タイムアウト
#ifndef RX_TIMEOUT
	#define RX_TIMEOUT             (3000)
#endif

// サーボ待ち時間
#define SERVO_WAIT                 (1000)

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/
// 構造体：入出力情報
typedef struct {
	// デジタル入力ピンマップ
	uint32 u32DiMap;
	// デジタル入力ピンマップ（前回分）
	uint32 u32DiMapBef;
	// 5V電源供給有無
	bool_t b5VPwSupply;
	// サーボ監視値
	int iServoPos;
	// サーボ設定値
	int iServoSetA;
	// サーボ設定値
	int iServoSetB;
	// サーボ監視バッファ
	IOUtil_tsADCBuffer sServoPosBuffer;
	// サーボ設定Aバッファ
	IOUtil_tsADCBuffer sServoSetABuffer;
	// サーボ設定Bバッファ
	IOUtil_tsADCBuffer sServoSetBBuffer;
	// 現在日時
	DS3231_datetime sDatetime;
	// 現在気温
	int iTemperature;
	// マスタートークンマスク
	uint8  u8MstTokenMask[APP_AUTH_TOKEN_SIZE];
} tsAppIOInfo;

// 構造体：インデックス情報
typedef struct {
	uint8 u8EnableCheck;				// 有効チェック（レコード有効時は0xAA）
	uint32 u32RemoteDevMap;				// リモートデバイス領域マップ
	uint8 u8EventLogCnt;				// イベント履歴数
	uint8 u8Filler[2];					// 余白
} tsAppIOIndexInfo;

// 構造体：インデックス情報バッファ
typedef struct {
	// リモートデバイス情報数
	uint8  u8RemoteDevCnt;
	// リモートデバイス情報インデックスリスト
	uint8  u8RemoteDevIdx[MAX_REMOTE_DEV_CNT];
	// リモートデバイスIDリスト
	uint32 u32RemoteDevID[MAX_REMOTE_DEV_CNT];
} tsAppIOIndexCache;

// 構造体：イベント履歴
typedef struct {
	uint8 u8SeqNo;						// シーケンス番号
	uint16 u16MsgCd;					// メッセージコード
	uint8 u8Command;					// コマンド
	uint8 u8StatusMap;					// ステータスマップ
	uint16 u16Year;						// イベント発生日（年）
	uint8 u8Month;						// イベント発生日（月）
	uint8 u8Day;						// イベント発生日（日）
	uint8 u8Hour;						// イベント発生時刻（時）
	uint8 u8Minute;						// イベント発生時刻（分）
	uint8 u8Second;						// イベント発生時刻（秒）
	uint8 u8Filler[4];					// 余白
} tsAppIOEventLog;

// 構造体：無線送受信メッセージ
typedef struct {
	// 宛先アドレス
	uint32 u32DstAddr;
	// コマンド
	uint8 u8Command;
	// 同期乱数
	uint32 u32SyncVal;
	// 認証ストレッチング回数
	uint8 u8AuthStCnt;
	// 認証トークン
	uint8 u8AuthToken[32];
	// 更新ストレッチング回数
	uint8 u8UpdAuthStCnt;
	// 更新トークン
	uint8 u8UpdAuthToken[32];
	// タイムスタンプ（年）
	uint16 u16Year;
	// タイムスタンプ（月）
	uint8 u8Month;
	// タイムスタンプ（日）
	uint8 u8Day;
	// タイムスタンプ（時）
	uint8 u8Hour;
	// タイムスタンプ（分）
	uint8 u8Minute;
	// タイムスタンプ（秒）
	uint8 u8Second;
	// ステータスマップ
	uint8 u8StatusMap;
	// CRCチェック
	uint8 u8CRC;
} tsWirelessMsg;

// 構造体：送受信データ
typedef struct {
	// 送信元/送信先アドレス
	uint32 u32Addr;
	// 送受信メッセージ
	tsWirelessMsg sMsg;
} tsRxTxInfo;

// 構造体：無線通信情報
typedef struct {
	// 宛先アドレス（自デバイス）
	uint32 u32DstAddr;
	// 受信有効フラグ
	bool_t bRxEnabled;
	// 受信バッファ開始インデックス
	uint8 u8RxFromIdx;
	// 受信バッファ終了インデックス
	uint8 u8RxToIdx;
	// 受信バッファバッファリングサイズ
	uint8 u8RxSize;
	// 受信バッファ
	tsRxTxInfo sRxBuffer[RX_BUFFER_SIZE];
	// 送信データフレームシーケンス
	uint8 u8TxSeq;
	// 送信バッファ開始インデックス
	uint8 u8TxFromIdx;
	// 送信バッファ終了インデックス
	uint8 u8TxToIdx;
	// 送信バッファバッファリングサイズ
	uint8 u8TxSize;
	// 送信バッファ
	tsRxTxInfo sTxBuffer[TX_BUFFER_SIZE];
} tsWirelessInfo;

// 構造体：画面制御情報
typedef struct {
	// LCD制御
	ST7032i_state sLCDstate;
	// 出力バッファ
	char cLCDBuff[LCD_BUFF_ROW_SIZE][LCD_BUFF_COL_SIZE + 1];
} tsLCDInfo;

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/
/** 入出力情報 */
PUBLIC tsAppIOInfo sAppIO;
/** 構造体：インデックス情報キャッシュ */
PUBLIC tsAppIOIndexCache sAppIOIndexCache;
/** 無線通信情報 */
PUBLIC tsWirelessInfo sWirelessInfo;
/** デバイス情報 */
PUBLIC tsAuthDeviceInfo sDevInfo;
/** イベント履歴情報 */
PUBLIC tsAppIOEventLog sEventLog;
/** I2C EEPROM情報 */
PUBLIC tsEEPROM_status sEEPROM_status;
/** I2C LCD制御情報 */
PUBLIC tsLCDInfo sLCDInfo;

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/

/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/
// 入出力初期処理
PUBLIC void vAppIOInit();
// 5Ｖ電源ON
PUBLIC void v5VPwOn();
// 5Ｖ電源OFF
PUBLIC void v5VPwOff();
// 入力バッファの更新
PUBLIC void vUpdInputBuffer();
// EEPROM初期処理
PUBLIC void vEEPROMInit();
// EEPROM読み込み（デバイス情報）
PUBLIC bool_t bEEPROMReadDevInfo();
// EEPROM書き込み（デバイス情報）
PUBLIC bool_t bEEPROMWriteDevInfo(uint8 u8StatusMapMask);
// EEPROM検索（リモートデバイス情報のインデックス検索）
PUBLIC int iEEPROMIndexOfRemoteInfo(uint32 u32DeviceID);
// EEPROM読み込み（リモートデバイス情報）
PUBLIC int iEEPROMReadRemoteInfo(tsAuthRemoteDevInfo *psRemoteInfo, uint8 u8Idx);
// EEPROM書き込み（リモートデバイス情報）
PUBLIC int iEEPROMWriteRemoteInfo(tsAuthRemoteDevInfo *psRemoteInfo);
// EEPROM書き込み（イベントログ情報）
PUBLIC int iEEPROMWriteLog(uint16 u16MsgCd, uint8 u8Cmd);
// EEPROMトークンマスククリア
PUBLIC void vEEPROMTokenMaskClear();
// 日時更新処理
PUBLIC void vDateTimeUpdate();
// LCD初期処理
PUBLIC void vLCDInit();
// LCDメッセージ書き込み処理
PUBLIC void vLCDSetMessage(char* pcMsg1, char* pcMsg2);
// イベントタスク：LCD描画
PUBLIC void vLCDdrawing();
// 無線通信の初期化処理
PUBLIC void vWirelessInit(tsAuthDeviceInfo* psDevInfo);
// 電文の受信有効化
PUBLIC void vWirelessRxEnabled(uint32 u32DstAddr);
// 電文の受信無効化
PUBLIC void vWirelessRxDisabled();
// 受信電文のバッファリング
PUBLIC bool_t bWirelessRxEnq(tsRxDataApp *psRx);
// 受信電文の取り出し
PUBLIC bool_t bWirelessRxDeq(tsRxTxInfo* psRxInfo);
// 送信電文のバッファリング
PUBLIC bool_t bWirelessTxEnq(uint32 u32SrcAddr, bool_t bEncryption, tsWirelessMsg* psMsg);
// 送信試行
PUBLIC bool_t bWirelessTxTry();
// 送信完了
PUBLIC bool_t bWirelessTxComplete(uint8 u8CbId, uint8 u8Status);
// サーボ設定バッファの更新
PUBLIC uint8 u8UpdServoSetting();
// サーボ位置の更新
PUBLIC void vUpdServoPos();
// サーボ位置の移動判定
PUBLIC bool_t bServoPosChange();
// サーボ制御（ロック）
PUBLIC void vSetServoLock();
// サーボ制御（アンロック）
PUBLIC void vSetServoUnlock();
// サーボ制御（出力オフ）
PUBLIC void vServoControlOff();

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* APP_IO_H_INCLUDED */

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
