/****************************************************************************
 *
 * MODULE :EEPROM Driver functions source file
 *
 * CREATED:2016/05/27 00:37:00
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
#include <jendefs.h>
#include "eeprom.h"
#include "i2c_util.h"
#include "timer_util.h"
#include "framework.h"

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
// デバイス情報
PRIVATE tsEEPROM_status *spEEPROM_status;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/*****************************************************************************
 *
 * NAME: bEEPROM_deviceSelect
 *
 * DESCRIPTION:デバイス選択
 *
 * PARAMETERS:      Name            RW  Usage
 * tsEEPROM_status  *spStatus       R   デバイスステータス情報
 *
 * RETURNS:
 *   bool_t         TRUE:デバイス選択成功
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bEEPROM_deviceSelect(tsEEPROM_status *spStatus) {
	// デバイスステータス情報
	spEEPROM_status = spStatus;
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bEEPROM_readData
 *
 * DESCRIPTION:データ読み込み
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint16         u16Addr         R   メモリアドレス
 *   uint16         u16Len          R   読み込み長
 *   uint8*         pu8Buff         R   読み込みバッファ
 *
 * RETURNS:
 *   bool_t         TRUE:読み込み成功
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bEEPROM_readData(uint16 u16Addr, uint16 u16Len, uint8 *pu8Buff) {
	// 入力チェック
	if (u16Len <= 0) return TRUE;
	// 書き込み開始宣言
	if (bI2C_startWrite(spEEPROM_status->u8DevAddress) == FALSE) {
		bI2C_stopACK();
		return FALSE;
	}
	// 参照開始アドレス（上位バイト）書き込み
	if (spEEPROM_status->b2ByteAddrFlg) {
		if (u8I2C_write((uint8)(u16Addr >> 8)) != I2CUTIL_STS_ACK) {
			bI2C_stopACK();
			return FALSE;
		}
	}
	// 参照開始アドレス（下位バイト）書き込み
	if (u8I2C_write((uint8)u16Addr) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
		return FALSE;
	}
	// 読み込み開始宣言
	if(bI2C_startRead(spEEPROM_status->u8DevAddress) == FALSE) {
		bI2C_stopNACK();
		return FALSE;
	}
	// データの読み込み処理
	if (bI2C_read(pu8Buff, u16Len, FALSE) == FALSE) {
		bI2C_stopNACK();
		return FALSE;
	}
	// I2C通信完了
	if (bI2C_stopNACK() == FALSE) {
		return FALSE;
	}
	// 読み込み完了
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bEEPROM_writeData
 *
 * DESCRIPTION:データ書き込み
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint16         u16Addr         R   メモリアドレス
 *   uint16         u16Len          R   書き込みデータ長
 *   uint8*         pu8Data         R   書き込みデータ
 *
 * RETURNS:
 *   bool_t         TRUE:読み込み成功
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bEEPROM_writeData(uint16 u16Addr, uint16 u16Len, uint8 *pu8Data) {
	// 入力チェック
	if (u16Len <= 0) return TRUE;
	// 書き込みループ
	uint16 u16End;
	uint16 u16Idx = 0;
	while (u16Idx < u16Len)  {
		// 書き込み間隔制御
		u32TimerUtil_waitUntil(spEEPROM_status->u64LastWrite + EEPROM_WRITE_CYCLE);
		// 書き込み開始宣言
		if (bI2C_startWrite(spEEPROM_status->u8DevAddress) == FALSE) {
			bI2C_stopACK();
			return FALSE;
		}
		// 参照開始アドレス（上位バイト）書き込み
		if (spEEPROM_status->b2ByteAddrFlg) {
			if (u8I2C_write((uint8)((u16Addr + u16Idx) >> 8)) != I2CUTIL_STS_ACK) {
				bI2C_stopACK();
				return FALSE;
			}
		}
		// 参照開始アドレス（下位バイト）書き込み
		if (u8I2C_write((uint8)(u16Addr + u16Idx)) != I2CUTIL_STS_ACK) {
			bI2C_stopACK();
			return FALSE;
		}
		// 書き込みサイズの判定
		if ((u16Len - u16Idx) >= spEEPROM_status->u8PageSize) {
			u16End = u16Idx + spEEPROM_status->u8PageSize - 1;
		} else {
			u16End = u16Len - 1;
		}
		// 書き込みデータの送信
		while (u16Idx < u16End) {
			if (u8I2C_write(*(pu8Data + u16Idx)) != I2CUTIL_STS_ACK) {
				bI2C_stopACK();
				return FALSE;
			}
			// インデックスの移動
			u16Idx++;
		}
		// 終端データの送信
		if (u8I2C_writeStop(*(pu8Data + u16Idx)) != I2CUTIL_STS_ACK) {
			bI2C_stopACK();
			return FALSE;
		}
		// インデックスの移動
		u16Idx++;
		// 最終書き込み時刻更新
		spEEPROM_status->u64LastWrite = u64TimerUtil_readUsec();
	}
	// 書き込み完了
	return TRUE;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
