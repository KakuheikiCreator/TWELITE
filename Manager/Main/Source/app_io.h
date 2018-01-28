/****************************************************************************
 *
 * MODULE :Application IO functions header file
 *
 * CREATED:2016/05/30 01:32:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:画面表示やキー入力等の基本的な入出力機能のヘッダファイル
 *
 * CHANGE HISTORY:
 * 2018/01/23 05:05:00 通信レコードレイアウトをAES暗号化に合わせて調整
 *
 * LAST MODIFIED BY:
 *
 ****************************************************************************
 * Copyright (c) 2016, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 ****************************************************************************/
#ifndef APP_IO_H_INCLUDED
#define APP_IO_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "ToCoNet.h"
#include "io_util.h"
#include "st7032i.h"
#include "ds3231.h"
#include "eeprom.h"
#include "keypad.h"
#include "app_auth.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// I2C Bus ID:01
#define I2C_BUS_01                 (0x01 << 3)
// I2C Bus ID:02
#define I2C_BUS_02                 (0x01 << 2)

// I2C Device Type:EEPROM
#define I2C_DEVICE_EEPROM          (0x01)
// I2C Device Type:RTC
#define I2C_DEVICE_RTC             (0x02)

// LCD Row Size
#define LCD_BUFF_ROW_SIZE          (7)
// LCD Column Size
#define LCD_BUFF_COL_SIZE          (16)

// 受信バッファサイズ
#ifndef RX_BUFFER_SIZE
	#define RX_BUFFER_SIZE         (5)
#endif

// 受信タイムアウト
#ifndef RX_TIMEOUT_S
	#define RX_TIMEOUT_S           (1000)
#endif

#ifndef RX_TIMEOUT_L
	#define RX_TIMEOUT_L           (6000)
#endif

// 送信レコードサイズ
#define TX_REC_SIZE                (90)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// Cursor
typedef enum {
	E_APPIO_CURSOR_UP,
	E_APPIO_CURSOR_DOWN,
	E_APPIO_CURSOR_LEFT,
	E_APPIO_CURSOR_RIGHT
} teAppCursor;

// Application Event
typedef enum {
	// LCD Column Control:NONE
	LCD_CTR_NONE = (0x00),
	// LCD Column Control:BLINK
	LCD_CTR_BLINK = (0x01),
	// LCD Column Control:INPUT
	LCD_CTR_INPUT = (0x02),
	// LCD Column Control:BLINK_INPUT
	LCD_CTR_BLINK_INPUT = (0x03)
} teLCDcntr;

// 構造体：画面制御情報
typedef struct {
	// LCD制御
	ST7032i_state sLCDstate;
	// 出力バッファ
	char cLCDBuff[LCD_BUFF_ROW_SIZE][LCD_BUFF_COL_SIZE + 1];
	// 制御領域
	uint8 u8LCDCntr[LCD_BUFF_ROW_SIZE][LCD_BUFF_COL_SIZE / 4];
	// 現在表示行
	uint8 u8CurrentDispRow;
	// カーソル位置（ライン）
	uint8 u8CursorPosRow;
	// カーソル位置（カラム）
	uint8 u8CursorPosCol;
} tsLCDInfo;

// 構造体：入出力情報
typedef struct {
	// 主I2Cトークンのデバイスタイプ
	uint8 u8I2CMainType;
	// 副I2Cトークンのデバイスタイプ
	uint8 u8I2CSubType;
	// 現在日時
	DS3231_datetime sDatetime;
	// I2C EEPROM情報
	tsEEPROM_status sEEPROM_status;
	// キーパッド情報
	tsKEYPAD_status sKEYPAD_status;
} tsAppIO;

// 構造体：インデックス情報
typedef struct {
	uint8 u8EnableCheck;				// 有効チェック（レコード有効時は0xAA）
	uint32 u32RemoteDevMap;				// リモートデバイス領域マップ
	uint8 u8EventLogCnt;				// イベント履歴数
	uint8 u8Filler[2];					// 余白
} tsAppIOIndexInfo;

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

// 構造体：受信データ
typedef struct {
	// 送信元アドレス
	uint32 u32SrcAddr;
	// 暗号化フラグ
	bool_t bSecurePkt;
	// 受信メッセージ
	tsWirelessMsg sMsg;
} tsRxInfo;

// 構造体：無線通信情報
typedef struct {
	// 宛先アドレス
	uint32 u32DstAddr;
	// 受信有効フラグ
	bool_t bRxEnabled;
	// 開始インデックス
	uint8 u8FromIdx;
	// 終了インデックス
	uint8 u8ToIdx;
	// バッファリングサイズ
	uint8 u8Size;
	// 受信バッファ
	tsRxInfo sRxBuffer[RX_BUFFER_SIZE];
} tsWirelessInfo;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/** 入出力情報 */
PUBLIC tsAppIO sAppIO;
/** LCD制御情報 */
PUBLIC tsLCDInfo sLCDInfo;
/** 無線通信暗号化設定 */
PUBLIC tsCryptDefs sCryptDefs;
/** インデックス情報 */
PUBLIC tsAppIOIndexInfo sIndexInfo;
/** イベント履歴情報 */
PUBLIC tsAppIOEventLog sEventLog;
/** 無線通信情報 */
PUBLIC tsWirelessInfo sWirelessInfo;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
// 入出力初期処理
PUBLIC void vAppIOInit();
// キーパッドの初期処理
PUBLIC void vKeyPadInit();
// キーパッドバッファのリフレッシュ処理
PUBLIC void vUpdateKeyPadBuff();
// キーパッドの読み込み
PUBLIC uint8 u8ReadKeyPad();
// 主I2C接続判定
PUBLIC void vI2CMainConnect();
// 副I2C接続判定
PUBLIC void vI2CSubConnect();
// I2C接続デバイスタイプチェック
PUBLIC uint8 u8I2CDeviceTypeChk(DS3231_datetime* psDatetime);
// 内臓EEPROM初期処理
PUBLIC void vLocalEEPROMInit();
// 内臓EEPROM書き込み
PUBLIC bool_t bLocalEEPROMWrite();
// EEPROM初期処理
PUBLIC void vEEPROMInit();
// EEPROM読み込み（デバイス情報）
PUBLIC bool_t bEEPROMReadDevInfo(tsAuthDeviceInfo *psDevInfo);
// EEPROM書き込み（デバイス情報）
PUBLIC bool_t bEEPROMWriteDevInfo(tsAuthDeviceInfo *psDevInfo);
// EEPROM読み込み（インデックス情報）
PUBLIC bool_t bEEPROMReadIndexInfo(tsAppIOIndexInfo *psIndexInfo);
// EEPROM書き込み（インデックス情報）
PUBLIC bool_t bEEPROMWriteIndexInfo(tsAppIOIndexInfo *psIndexInfo);
// EEPROM検索（リモートデバイス情報のインデックス検索）
PUBLIC int iEEPROMIndexOfRemoteInfo(uint32 u32DeviceID);
// EEPROM読み込み（リモートデバイス情報）
PUBLIC int iEEPROMReadRemoteInfo(tsAuthRemoteDevInfo *psRemoteInfo, uint8 u8Idx, bool_t bForwardFlg);
// EEPROM書き込み（リモートデバイス情報）
PUBLIC int iEEPROMWriteRemoteInfo(tsAuthRemoteDevInfo *psRemoteInfo);
// EEPROMクリア（リモートデバイス情報）
PUBLIC int iEEPROMDeleteRemoteInfo(uint32 u32DeviceID);
// EEPROM全クリア（リモートデバイス情報）
PUBLIC int iEEPROMDeleteAllRemoteInfo();
// EEPROM読み込み（イベントログ情報）
PUBLIC int iEEPROMReadLog(tsAppIOEventLog *psAppIOEventLog, uint8 u8Idx);
// EEPROM書き込み（イベントログ情報）
PUBLIC int iEEPROMWriteLog(uint16 u16MsgCd, tsWirelessMsg *psWirelessMsg);
// EEPROMクリア（イベントログ情報）
PUBLIC int iEEPROMDeleteAllLogInfo();
// イベントタスク：秒間隔イベント処理
PUBLIC void vEventSecond(uint32 u32EvtTimeMs);
// LCD初期処理
PUBLIC void vLCDInit();
// イベントタスク：LCD描画
PUBLIC void vEventLCDdrawing(uint32 u32EvtTimeMs);
// LCDのカラム制御情報変換
PUBLIC uint8 u8LCDTypeToByte(teLCDcntr eType1, teLCDcntr eType2, teLCDcntr eType3, teLCDcntr eType4);
// LCDのカーソル移動処理
PUBLIC bool_t bLCDCursorSet(uint8 u8Row, uint8 u8Col);
// LCDのカーソル移動処理
PUBLIC bool_t bLCDCursorMove(teAppCursor eAppCursor);
// LCDの入力によるカーソル移動処理
PUBLIC bool_t bLCDCursorKeyMove(uint8 u8Key);
// LCDの制御情報取得処理
PUBLIC teLCDcntr eLCDGetCntr(uint8 u8Row, uint8 u8Col);
// LCD描画領域への書き込み処理（数値）
PUBLIC bool_t bLCDWriteNumber(uint8 u8Char, uint8 u8PadChar);
// LCD描画領域への書き込み処理（文字）
PUBLIC bool_t bLCDWriteChar(uint8 u8Char);
// LCD描画領域の編集処理（BSキー処理）
PUBLIC bool_t bLCDBackSpace(uint8 u8PadCh);
// LCD描画領域の編集処理（DELキー処理）
PUBLIC bool_t bLCDDeleteChar(uint8 u8PadCh);
// LCD描画領域の編集処理（文字挿入処理）
PUBLIC bool_t bLCDInsertChar(uint8 u8InsCh);
// 無線通信の初期化処理
PUBLIC void vWirelessInit(tsAuthDeviceInfo* psDevInfo);
// 電文の受信有効化メソッド
PUBLIC void vWirelessRxEnabled(uint32 u32DstAddr);
// 電文の受信無効化メソッド
PUBLIC void vWirelessRxDisabled();
// 受信電文の取り出しメソッド
PUBLIC bool_t bWirelessRxFetch(tsRxInfo* psRxInfo);
// 電文の受信メソッド
PUBLIC bool_t bWirelessRx(tsRxDataApp *psRx);
// 電文の送信メソッド
PUBLIC bool_t bWirelessTx(uint32 u32SrcAddr, bool_t bEncryption, uint8* u8Data, uint8 u8Len);
// 乱数文字列生成処理
PUBLIC void vSetRandString(char cString[], uint8 u8Len);
// イベントタスク：メロディ初期処理
PUBLIC void vMelodyInit();
// イベントタスク：メロディ演奏（OK）
PUBLIC void vEventMelodyOK(uint32 u32EvtTimeMs);
// イベントタスク：メロディ演奏（NG）
PUBLIC void vEventMelodyNG(uint32 u32EvtTimeMs);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* APP_IO_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
