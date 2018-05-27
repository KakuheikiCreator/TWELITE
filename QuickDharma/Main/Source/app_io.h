/*******************************************************************************
 *
 * MODULE :Application IO functions header file
 *
 * CREATED:2018/05/07 21:30:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:センサー等との基本的な入出力機能のヘッダファイル
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 *******************************************************************************
 * Copyright (c) 2018, Nakanohito
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
#include "ds3231.h"
#include "eeprom.h"
#include "adxl345.h"
#include "s11059_02dt.h"

/******************************************************************************/
/***        Macro Definitions                                               ***/
/******************************************************************************/
// DI Check Cnt
#define DI_BUFF_CHK_CNT              (5)

// Pin Map:人感センサー
#define PIN_MAP_IR_SENS              (0x01 << 5)
// Pin Map:ドップラーセンサー
#define PIN_MAP_DOPPLER_SENS         (0x01 << 1)

// I2C Device Type:EEPROM
#define I2C_DEVICE_EEPROM            (0x01)
// I2C Device Type:RTC
#define I2C_DEVICE_RTC               (0x02)
// I2C Device Type:Acceleration Sensor
#define I2C_DEVICE_ACCELERATION      (I2C_ADDR_ADXL345_L)
// I2C Device Type:Color Sensor
#define I2C_DEVICE_COLOR             (I2C_ADDR_S11059_02DT)

// 受信バッファサイズ
#ifndef RX_BUFFER_SIZE
	#define RX_BUFFER_SIZE           (3)
#endif

// 送信バッファサイズ
#ifndef TX_BUFFER_SIZE
	#define TX_BUFFER_SIZE           (3)
#endif

// 受信タイムアウト
#ifndef RX_TIMEOUT
	#define RX_TIMEOUT               (3000)
#endif

// 送信レコードサイズ
#define TX_REC_SIZE                  (90)

// 温度センサー閾値（整数で扱う為に100倍の値）
#ifndef TEMPERATURE_THRESHOLD
	#define TEMPERATURE_THRESHOLD      (5500)
#endif

// 加速度センサー閾値
#ifndef AXES_THRESHOLD
	#define AXES_THRESHOLD             (10)
#endif

// 加速度データバッファサイズ
#ifndef AXES_BUFFER_SIZE
	// ０．２秒間隔で１秒間分を想定
	#define AXES_BUFFER_SIZE           (6)
#endif

// カラーセンサー閾値
#ifndef COLOR_THRESHOLD
	#define COLOR_THRESHOLD            (10)
#endif

// カラーデータバッファサイズ
#ifndef COLOR_BUFFER_SIZE
	// ０．２秒間隔で５秒間分を想定
	#define COLOR_BUFFER_SIZE          (27)
#endif

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/
// 構造体：入出力情報
typedef struct {
	// デジタル入力ピンマップ
	uint32 u32DiMap;
	// デジタル入力ピンマップ（前回分）
	uint32 u32DiMapBef;
	// 現在日時
	DS3231_datetime sDatetime;
	// 現在気温
	int iTemperature;
	// 加速度センサー情報
	tsADXL345_Info sAccelerationInfo;
	// カラーセンサー情報
	tsS11059_02DT_state sColorState;
	// 加速度センサーバッファインデックス
	uint8 u8AxesBuffIdx;
	// 加速度センサーバッファ
	tsADXL345_AxesData sAxesData[AXES_BUFFER_SIZE];
	// カラーセンサーバッファインデックス
	uint8 u8ColorBuffIdx;
	// カラーデータバッファ
	tsS11059_02DT_data sColorBuffer[COLOR_BUFFER_SIZE];
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
	// 同期乱数
	uint32 u32SyncVal;
	// コマンド
	uint8 u8Command;
	// 認証ストレッチング回数
	uint8 u8AuthStCnt;
	// 認証トークン
	uint8 u8AuthToken[32];
	// 更新ストレッチング回数
	uint8 u8UpdAuthStCnt;
	// 更新トークン
	uint8 u8UpdAuthToken[32];
	// FILLER01
	uint8 u8Filler01;
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
	// FILLER02
	uint8 u8Filler02[5];
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

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/

/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/
// 入出力初期処理
PUBLIC void vAppIOInit();
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
// 高温判定
PUBLIC bool_t bHighTemperature();
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
// 加速度センサー初期処理
PUBLIC bool_t bAccelerationInit();
// 加速度データバッファの更新
PUBLIC bool_t bUpdAxesBuffer();
// 加速度検知判定
PUBLIC bool_t bAccelerating();
// カラーセンサー初期処理
PUBLIC bool_t bColorSensInit();
// カラーデータバッファの更新
PUBLIC bool_t bUpdColorBuffer();
// カラーセンサー検知判定
PUBLIC bool_t bColorChange();

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
