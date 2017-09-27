/****************************************************************************
 *
 * MODULE :Keypad Driver functions source file
 *
 * CREATED:2016/09/18 07:15:00
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
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <AppHardwareApi.h>
#include "keypad.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/** キー文字マップ */
static const char KEYPAD_KEYMAP[4][5] = {"123A", "456B", "789C", "*0#D"};

// デバイス情報
PRIVATE tsKEYPAD_status *spKEYPAD_status;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/*****************************************************************************
 *
 * NAME: vKEYPAD_deviceSelect
 *
 * DESCRIPTION:デバイス選択
 *
 * PARAMETERS:      Name            RW  Usage
 * tsKEYPAD_status  *spStatus       R   デバイスステータス情報
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vKEYPAD_deviceSelect(tsKEYPAD_status *spStatus) {
	// デバイスステータス情報
	spKEYPAD_status = spStatus;
}

/*****************************************************************************
 *
 * NAME: vKEYPAD_configSet
 *
 * DESCRIPTION:設定処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    bool_t        TRUE:設定成功
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vKEYPAD_configSet() {
	// バッファステータス初期化
	spKEYPAD_status->u8CheckRowNo = 0;
	spKEYPAD_status->u32BeforeVal = 0;
	spKEYPAD_status->u8KeyChkCnt  = 0;
	spKEYPAD_status->u8BuffIdx    = 0;
	spKEYPAD_status->u8BuffSize   = 0;
	// 出力ポートビットマップを編集（1が立っている個所以外は現状維持）
	spKEYPAD_status->u32PinMapCols = 0x00;
	spKEYPAD_status->u32PinMapCols |= 0x01 << spKEYPAD_status->u8PinCols[0];
	spKEYPAD_status->u32PinMapCols |= 0x01 << spKEYPAD_status->u8PinCols[1];
	spKEYPAD_status->u32PinMapCols |= 0x01 << spKEYPAD_status->u8PinCols[2];
	spKEYPAD_status->u32PinMapCols |= 0x01 << spKEYPAD_status->u8PinCols[3];
	// 入力ポートビットマップ編集（1が立っている個所以外は現状維持）
	spKEYPAD_status->u32PinMapRows = 0x00;
	spKEYPAD_status->u32PinMapRows |= 0x01 << spKEYPAD_status->u8PinRows[0];
	spKEYPAD_status->u32PinMapRows |= 0x01 << spKEYPAD_status->u8PinRows[1];
	spKEYPAD_status->u32PinMapRows |= 0x01 << spKEYPAD_status->u8PinRows[2];
	spKEYPAD_status->u32PinMapRows |= 0x01 << spKEYPAD_status->u8PinRows[3];
	// DIOピンの入出力を設定
	vAHI_DioSetDirection(spKEYPAD_status->u32PinMapCols, spKEYPAD_status->u32PinMapRows);
	// プルアップ設定
	vAHI_DioSetPullup(spKEYPAD_status->u32PinMapCols, spKEYPAD_status->u32PinMapRows);
	// DIOピン出力設定
	uint32 u32OnMap =
		spKEYPAD_status->u32PinMapRows & ~(0x01 << spKEYPAD_status->u8PinRows[spKEYPAD_status->u8CheckRowNo]);
	vAHI_DioSetOutput(u32OnMap, spKEYPAD_status->u32PinMapCols);
}

/*****************************************************************************
 *
 * NAME: bKEYPAD_updateBuffer
 *
 * DESCRIPTION:バッファ更新
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    bool_t        TRUE:更新成功
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bKEYPAD_updateBuffer() {
	// キー入力チェック
	uint32 u32DiMap = u32AHI_DioReadInput() & spKEYPAD_status->u32PinMapCols;
	if (u32DiMap == spKEYPAD_status->u32PinMapCols) {
		// 未入力の場合には次の行をアクティブ（L）にする
		spKEYPAD_status->u8CheckRowNo = (spKEYPAD_status->u8CheckRowNo + 1) % KEYPAD_ROW_CNT;
		uint32 u32DiOffMap = 0x01 << spKEYPAD_status->u8PinRows[spKEYPAD_status->u8CheckRowNo];
		uint32 u32DiOnMap  = spKEYPAD_status->u32PinMapRows & ~u32DiOffMap;
		vAHI_DioSetOutput(u32DiOnMap, u32DiOffMap);
		// 入力履歴クリア
		spKEYPAD_status->u32BeforeVal = u32DiMap;
		spKEYPAD_status->u8KeyChkCnt  = 0;
		return FALSE;
	}
	// 同一キー入力判定
	if (u32DiMap != spKEYPAD_status->u32BeforeVal) {
		spKEYPAD_status->u32BeforeVal = u32DiMap;
		spKEYPAD_status->u8KeyChkCnt  = 0;
		return FALSE;
	}
	// 押しっぱなし判定
	if (spKEYPAD_status->u8KeyChkCnt >= KEYPAD_CHK_CNT) {
		return FALSE;
	}
	// 一致回数判定
	spKEYPAD_status->u8KeyChkCnt++;
	if (spKEYPAD_status->u8KeyChkCnt < KEYPAD_CHK_CNT) {
		// 規定回数未満
		return FALSE;
	}
	// チャタリングOK
	uint8 u8ColIdx;
	uint32 u32PinMask;
	for (u8ColIdx = 0; u8ColIdx < KEYPAD_COL_CNT; u8ColIdx++) {
		u32PinMask = 0x01 << spKEYPAD_status->u8PinCols[u8ColIdx];
		if ((u32DiMap & u32PinMask) == 0) break;
	}
	// バッファリング
	uint8 u8BuffIdx;
	if (spKEYPAD_status->u8BuffSize < KEYPAD_BUFF_SIZE) {
		u8BuffIdx = (spKEYPAD_status->u8BuffIdx + spKEYPAD_status->u8BuffSize) % KEYPAD_BUFF_SIZE;
		spKEYPAD_status->u8BuffSize++;
	} else {
		// バッファが一杯の場合
		u8BuffIdx = spKEYPAD_status->u8BuffIdx;
		spKEYPAD_status->u8BuffIdx = (spKEYPAD_status->u8BuffIdx + 1) % KEYPAD_BUFF_SIZE;
	}
	spKEYPAD_status->u8KeyBuffer[u8BuffIdx] =
			KEYPAD_KEYMAP[spKEYPAD_status->u8CheckRowNo][u8ColIdx];
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: u8KEYPAD_bufferSize
 *
 * DESCRIPTION:バッファサイズ取得
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    uint8         バッファサイズ
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint8 u8KEYPAD_bufferSize() {
	return spKEYPAD_status->u8BuffSize;
}

/*****************************************************************************
 *
 * NAME: vKEYPAD_clearBuffer
 *
 * DESCRIPTION:バッファクリア
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   bool_t         TRUE:バッファクリア成功
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vKEYPAD_clearBuffer() {
	spKEYPAD_status->u8BuffSize = 0;
}

/*****************************************************************************
 *
 * NAME: bKEYPAD_readKey
 *
 * DESCRIPTION:バッファ先頭文字の読み込み
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8*      pu8Key          W   読み込みキー
 *
 * RETURNS:
 *   bool_t         TRUE:読み込み成功
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint8 u8KEYPAD_readKey() {
	// バッファ文字の有無判定
	if (spKEYPAD_status->u8BuffSize == 0) {
		return 0x00;
	}
	// バッファ読み込み
	uint8 u8Idx = spKEYPAD_status->u8BuffIdx;
	spKEYPAD_status->u8BuffIdx = (spKEYPAD_status->u8BuffIdx + 1) % KEYPAD_BUFF_SIZE;
	spKEYPAD_status->u8BuffSize--;
	return spKEYPAD_status->u8KeyBuffer[u8Idx];
}

/*****************************************************************************
 *
 * NAME: bKEYPAD_readLast
 *
 * DESCRIPTION:バッファ終端文字の読み込み
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   bool_t         TRUE:読み込み成功
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint8 u8KEYPAD_readLast() {
	// バッファ文字の有無判定
	if (spKEYPAD_status->u8BuffSize == 0) {
		return (uint8)NULL;
	}
	// バッファ読み込み
	uint8 u8Idx = (spKEYPAD_status->u8BuffIdx + spKEYPAD_status->u8BuffSize - 1) % KEYPAD_BUFF_SIZE;
	uint8 u8key = spKEYPAD_status->u8KeyBuffer[u8Idx];
	// バッファクリア
	spKEYPAD_status->u8BuffSize = 0;
	return u8key;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
