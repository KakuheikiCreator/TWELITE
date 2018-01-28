/*******************************************************************************
 *
 * MODULE :Application IO functions header file
 *
 * CREATED:2017/08/29 00:00:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:画面表示やキー入力等の基本的な入出力機能のヘッダファイル
 *
 * CHANGE HISTORY:
 * 2018/01/20 21:47:00 認証時の通信内容をAES暗号化する事で保護
 *
 * LAST MODIFIED BY:
 *
 *******************************************************************************
 * Copyright (c) 2017, Nakanohito
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
// ADC Check Cnt
#define ADC_BUFF_CHK_CNT             (3)
// ADC Check Tolerance
#define ADC_CHK_TOLERANCE            (30)
// Pin No:ボタンチェック
#define PIN_NO_BUTTON_CHK            (E_AHI_ADC_SRC_ADC_4)

// ボタン番号：ボタン０
#define BTN_NO_0                     (0)
// ボタン番号：ボタン１
#define BTN_NO_1                     (1)
// ボタン番号：ボタン２
#define BTN_NO_2                     (2)
// ボタン番号：ボタン３
#define BTN_NO_3                     (3)

// LCD Row Size
#define LCD_BUFF_ROW_SIZE            (2)
// LCD Column Size
#define LCD_BUFF_COL_SIZE            (8)

// 送信レコードサイズ
#define TX_REC_SIZE                  (90)

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/
// 構造体：入出力情報
typedef struct {
	// ボタン監視値
	int iButtonNo;
	// ボタン監視バッファ
	IOUtil_tsADCBuffer sButtonBuffer;
} tsAppIOInfo;

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

// 構造体：送受信データ
typedef struct {
	// 送信元/送信先アドレス
	uint32 u32Addr;
	// 送受信メッセージ
	tsWirelessMsg sMsg;
} tsRxTxInfo;

// 構造体：無線通信情報
typedef struct {
	// 通信対象アドレス
	uint32 u32TgtAddr;
	// 受信有効フラグ
	bool_t bRxEnabled;
	// 受信メッセージ
	tsWirelessMsg* psWlsRxMsg;
	// 送信データフレームシーケンス
	uint8 u8TxSeq;
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
/** 無線通信情報 */
PUBLIC tsWirelessInfo sWirelessInfo;
/** I2C EEPROM情報 */
PUBLIC tsEEPROM_status sEEPROM_status;
/** デバイス情報 */
PUBLIC tsAuthDeviceInfo sDevInfo;
/** イベント履歴情報 */
PUBLIC tsAppIOEventLog sEventLog;
/** I2C LCD制御情報 */
PUBLIC ST7032i_state sLCDstate;

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
// 入力ボタンの読み込み
PUBLIC int iReadButton();
// EEPROM初期処理
PUBLIC void vEEPROMInit();
// EEPROM読み込み（デバイス情報）
PUBLIC bool_t bEEPROMReadDevInfo();
// EEPROM書き込み（デバイス情報）
PUBLIC bool_t bEEPROMWriteDevInfo(uint8 u8StatusMapMask);
// EEPROM読み込み（リモートデバイス情報）
PUBLIC int iEEPROMReadRemoteInfo(tsAuthRemoteDevInfo *psRemoteInfo, uint8 u8Idx, bool_t bForwardFlg);
// EEPROM書き込み（リモートデバイス情報）
PUBLIC int iEEPROMWriteRemoteInfo(tsAuthRemoteDevInfo *psRemoteInfo);
// EEPROM書き込み（イベントログ情報）
PUBLIC int iEEPROMWriteLog(uint16 u16MsgCd, tsWirelessMsg* psMsg);
// LCD初期処理
PUBLIC void vLCDInit();
// イベントタスク：LCD描画
PUBLIC void vLCDdrawing(char* pcMsg0, char* pcMsg1);
// 無線通信の初期化処理
PUBLIC void vWirelessInit(tsAuthDeviceInfo* psDevInfo);
// 電文の受信有効化
PUBLIC void vWirelessRxEnabled(uint32 u32SrcAddr, tsWirelessMsg* psWlsMsg);
// 電文の受信無効化
PUBLIC void vWirelessRxDisabled();
// 受信処理
PUBLIC bool_t bWirelessRxPkt(tsRxDataApp *psRx);
// メッセージ送信
PUBLIC bool_t bWirelessTx(uint32 u32SrcAddr, bool_t bEncryption, uint8* u8Data, uint8 u8Len);

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
