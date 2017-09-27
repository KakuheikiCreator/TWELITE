/****************************************************************************
 *
 * MODULE :Keypad Driver functions header file
 *
 * CREATED:2016/09/18 04:20:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:Microchip EEPROM driver
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
#ifndef  KEYPAD_H_INCLUDED
#define  KEYPAD_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// KeyPad Setting
#define KEYPAD_BUFF_SIZE       (16)     // バッファサイズ
#define KEYPAD_CHK_CNT         (4)      // チャタリングチェック回数
#define KEYPAD_COL_CNT         (4)      // 列数
#define KEYPAD_ROW_CNT         (4)      // 行数

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/**
 * KEYPAD用のステータス情報
 */
typedef struct {
	uint8 u8CheckRowNo;						// チェック行番号
	uint32 u32PinMapCols;					// 列ピンマップ
	uint32 u32PinMapRows;					// 行ピンマップ
	uint8 u8PinCols[KEYPAD_COL_CNT];		// 列ピン配列
	uint8 u8PinRows[KEYPAD_ROW_CNT];		// 行ピン配列
	uint32 u32BeforeVal;					// 前回キー値
	uint8 u8KeyChkCnt;						// 同一キー回数
	uint8 u8BuffIdx;						// キーバッファインデックス
	uint8 u8BuffSize;						// キーバッファサイズ
	uint8 u8KeyBuffer[KEYPAD_BUFF_SIZE];	// キーバッファ
} tsKEYPAD_status;

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
/** デバイス選択 */
PUBLIC void vKEYPAD_deviceSelect(tsKEYPAD_status *spStatus);
/** デバイスセッティング */
PUBLIC void vKEYPAD_configSet();
/** バッファ更新 */
PUBLIC bool_t bKEYPAD_updateBuffer();
/** バッファサイズ取得 */
PUBLIC uint8 u8KEYPAD_bufferSize();
/** バッファクリア */
PUBLIC void vKEYPAD_clearBuffer();
/** キー読み込み */
PUBLIC uint8 u8KEYPAD_readKey();
/** 最終バッファキー読み込み */
PUBLIC uint8 u8KEYPAD_readLast();

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* KEYPAD_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
