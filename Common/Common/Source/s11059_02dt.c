/****************************************************************************
 *
 * MODULE :S11059-02DT Color sensor Driver functions source file
 *
 * CREATED:2018/04/29 02:23:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:S11059-02DT Color sensor draiver
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 ****************************************************************************
 * Copyright (c) 2018, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include "s11059_02dt.h"
#include "i2c_util.h"
#include "timer_util.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
// 計測間隔算出
PRIVATE uint32 u32S11059_02DT_intervalTime();

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
// 制御情報
PRIVATE tsS11059_02DT_state *spS11059_02DT_state = NULL;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/*****************************************************************************
 *
 * NAME: bS11059_02DT_deviceSelect
 *
 * DESCRIPTION:デバイス選択
 *
 * PARAMETERS:         Name          RW  Usage
 * S11059_02DT_state   *spStatus     R   デバイスステータス情報
 *
 * RETURNS:
 *   bool_t         TRUE:デバイス選択成功
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bS11059_02DT_deviceSelect(tsS11059_02DT_state *spState) {
	// 積分時間チェック
	if (spState->u8IntegralTime > S11059_02DT_UNIT_TIME_3) {
		return FALSE;
	}
	// デバイスステータス情報
	spS11059_02DT_state = spState;
	// 正常
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bS11059_02DT_start
 *
 * DESCRIPTION:計測開始
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   bool_t         TRUE:計測開始
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bS11059_02DT_start() {
	// ゲイン選択
	uint8 u8Ctr = (spS11059_02DT_state->bGainSelection << 3);
	// 積分モード
	u8Ctr = u8Ctr | (spS11059_02DT_state->bIntegralMode << 2);
	// 積分時間設定
	u8Ctr = u8Ctr | spS11059_02DT_state->u8IntegralTime;
	// マニュアルタイミング
	uint16 u16time = spS11059_02DT_state->u16ManualTiming;
	// 書き込み開始宣言
	if (bI2C_startWrite(spS11059_02DT_state->u8Address) != TRUE) {
		bI2C_stopACK();
		return FALSE;
	}
	// アドレス書き込み（コントロール）
	if (u8I2C_write(0x00) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
		return FALSE;
	}
	// 制御レジスタ書き込み（リセット）
	if (u8I2C_writeStop(0x84) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
		return FALSE;
	}
	// 書き込み開始宣言
	if (bI2C_startWrite(spS11059_02DT_state->u8Address) != TRUE) {
		bI2C_stopACK();
		return FALSE;
	}
	// アドレス書き込み（コントロール）
	if (u8I2C_write(0x00) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
		return FALSE;
	}
	// 固定時間判定
	if (spS11059_02DT_state->bIntegralMode == S11059_02DT_FIXED_MODE){
		// 制御レジスタ書き込み
		if (u8I2C_writeStop(u8Ctr) != I2CUTIL_STS_ACK) {
			bI2C_stopACK();
			return FALSE;
		}
	} else {
		// 制御レジスタ書き込み
		if (u8I2C_write(u8Ctr) != I2CUTIL_STS_ACK) {
			bI2C_stopACK();
			return FALSE;
		}
		// マニュアルタイミング書き込み
		if (u8I2C_write((uint8)(u16time >> 8)) != I2CUTIL_STS_ACK) {
			bI2C_stopACK();
			return FALSE;
		}
		if (u8I2C_writeStop((uint8)u16time) != I2CUTIL_STS_ACK) {
			bI2C_stopACK();
			return FALSE;
		}
	}
	// 計測間隔
	spS11059_02DT_state->u32Interval = u32S11059_02DT_intervalTime();
	// 次回計測完了時刻
	spS11059_02DT_state->u64NextRead = u64TimerUtil_readUsec() + (uint64)spS11059_02DT_state->u32Interval;
	// 正常終了
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bS11059_02DT_readState
 *
 * DESCRIPTION:コントロール情報読み込み
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   bool_t         TRUE:読み込み成功
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bS11059_02DT_readState() {
	// 書き込み開始宣言
	if (bI2C_startWrite(spS11059_02DT_state->u8Address) != TRUE) {
		bI2C_stopACK();
		return FALSE;
	}
	// アドレス書き込み（コントロール）
	if (u8I2C_write(0x00) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
		return FALSE;
	}
	// 読み込み開始宣言
	if(bI2C_startRead(spS11059_02DT_state->u8Address) != TRUE) {
		bI2C_stopACK();
		return FALSE;
	}
	// コントロールとマニュアルタイミングの読み込み処理
	uint8 u8Buff[3];
	if (bI2C_read(u8Buff, 3, FALSE) != TRUE) {
		bI2C_stopACK();
		return FALSE;
	}
	// ADCリセット
	spS11059_02DT_state->bADCReset = (u8Buff[0] >> 7) & 0x01;
	// スリープ待機
	spS11059_02DT_state->bSleepFunc = (u8Buff[0] >> 6) & 0x01;
	// スリープ機能モニタ
	spS11059_02DT_state->bSleepFuncMonitor = (u8Buff[0] >> 5) & 0x01;
	// ゲイン選択
	spS11059_02DT_state->bGainSelection = (u8Buff[0] >> 3) & 0x01;
	// 積分モード
	spS11059_02DT_state->bIntegralMode = (u8Buff[0] >> 2) & 0x01;
	// 積分時間設定
	spS11059_02DT_state->u8IntegralTime = u8Buff[0] & 0x03;
	// マニュアルタイミング
	spS11059_02DT_state->u16ManualTiming = ((uint16)u8Buff[1] << 8) + u8Buff[2];
	// 読み込み完了
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bS11059_02DT_readData
 *
 * DESCRIPTION:データ読み込み
 *
 * PARAMETERS:         Name         RW  Usage
 * S11059_02DT_data*   spData       W   データ書き込み領域
 *
 * RETURNS:
 *   bool_t         TRUE:読み込み成功
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bS11059_02DT_readData(tsS11059_02DT_data *spData) {
	// 経過時間判定
	if (spS11059_02DT_state->u64NextRead > u64TimerUtil_readUsec()) {
		return FALSE;
	}
	// 書き込み開始宣言
	if (bI2C_startWrite(spS11059_02DT_state->u8Address) != TRUE) {
		bI2C_stopACK();
		return FALSE;
	}
	// アドレス書き込み（データ）
	if (u8I2C_write(0x03) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
		return FALSE;
	}
	// 読み込み開始宣言
	if(bI2C_startRead(spS11059_02DT_state->u8Address) != TRUE) {
		bI2C_stopACK();
		return FALSE;
	}
	// データ読み込み処理
	if (bI2C_read((uint8*)spData, 8, FALSE) != TRUE) {
		bI2C_stopACK();
		return FALSE;
	}
	// 固定時間判定
	if (spS11059_02DT_state->bIntegralMode == S11059_02DT_FIXED_MODE){
		// 次回計測完了時刻を更新
		uint64 u64Interval = (uint64)spS11059_02DT_state->u32Interval;
		uint64 u64NextRead = spS11059_02DT_state->u64NextRead;
		uint64 u64Now = u64TimerUtil_readUsec();
		uint64 u64Elapsed = u64Now - u64NextRead;
		u64Elapsed = u64Elapsed - (u64Elapsed % u64Interval) + u64Interval;
		spS11059_02DT_state->u64NextRead = u64NextRead + u64Elapsed;
	}
	// 読み込み完了
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bS11059_02DT_isDataReady
 *
 * DESCRIPTION:計測完了判定
 *
 * PARAMETERS:         Name         RW  Usage
 *
 * RETURNS:
 *   bool_t    TRUE:計測データ準備完了
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bS11059_02DT_isDataReady() {
	// 経過時間判定
	return spS11059_02DT_state->u64NextRead <= u64TimerUtil_readUsec();
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/*****************************************************************************
 *
 * NAME: u32S11059_02DT_intervalTime
 *
 * DESCRIPTION:計測間隔算出
 *
 * PARAMETERS:         Name         RW  Usage
 *
 * RETURNS:
 *   uint32            計測間隔時間（マイクロ秒単位）
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE uint32 u32S11059_02DT_intervalTime() {
	// 単位時間
	uint32 u32UnitTime;
	switch (spS11059_02DT_state->u8IntegralTime) {
	case S11059_02DT_UNIT_TIME_0:
		u32UnitTime = 350;		// 87.5us * 4
		break;
	case S11059_02DT_UNIT_TIME_1:
		u32UnitTime = 5600;		// 1.4ms * 4
		break;
	case S11059_02DT_UNIT_TIME_2:
		u32UnitTime = 89600;	// 22.4ms * 4
		break;
	default:
		u32UnitTime = 716800;	// 179.2ms * 4
	}
	// 積分モード判定
	if (spS11059_02DT_state->bIntegralMode == S11059_02DT_FIXED_MODE) {
		return u32UnitTime;
	}
	// 計測間隔算出
	return ((uint32)spS11059_02DT_state->u16ManualTiming * u32UnitTime * 2);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
