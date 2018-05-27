/****************************************************************************
 *
 * MODULE :S11059-02DT Color sensor Driver functions header file
 *
 * CREATED:2018/04/26 01:40:00
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
#ifndef  S11059_02DT_H_INCLUDED
#define  S11059_02DT_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// I2C Address
#define I2C_ADDR_S11059_02DT       (0x2A)
// ゲイン：低利得
#define S11059_02DT_LOW_GAIN       (0x00)
// ゲイン：高利得
#define S11059_02DT_HIGH_GAIN      (0x01)
// 積分モード：固定時間
#define S11059_02DT_FIXED_MODE     (0x00)
// 積分モード：マニュアル時間
#define S11059_02DT_MANUAL_MODE    (0x01)
// 積分時間設定
#define S11059_02DT_UNIT_TIME_0    (0x00)	// 87.5us
#define S11059_02DT_UNIT_TIME_1    (0x01)	// 1.4ms
#define S11059_02DT_UNIT_TIME_2    (0x02)	// 22.4m
#define S11059_02DT_UNIT_TIME_3    (0x03)	// 179.2ms


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/**
 * S11059-02DTのコントロール型
 */
typedef struct {
	uint8 u8Address;			// I2Cアドレス
	bool_t bADCReset;			// ADCリセット
	bool_t bSleepFunc;			// スリープ機能
	bool_t bSleepFuncMonitor;	// スリープ機能モニタ
	bool_t bGainSelection;		// ゲイン選択
	bool_t bIntegralMode;		// 積分モード
	uint8 u8IntegralTime;		// 積分時間設定
	uint16 u16ManualTiming;		// マニュアルタイミング
	uint32 u32Interval;			// 計測間隔
	uint64 u64NextRead;			// 次回読み込み時刻
} tsS11059_02DT_state;

/**
 * S11059-02DTのデータ型
 */
typedef struct {
	uint16 u16DataRed;			// センサーのデータ（Red）
	uint16 u16DataGreen;		// センサーのデータ（Green）
	uint16 u16DataBlue;			// センサーのデータ（Blue）
	uint16 u16DataInfrared;		// センサーのデータ（Infrared）
} tsS11059_02DT_data;

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
PUBLIC bool_t bS11059_02DT_deviceSelect(tsS11059_02DT_state *spState);
/** 計測開始 */
PUBLIC bool_t bS11059_02DT_start();
/** コントロール情報読み込み */
PUBLIC bool_t bS11059_02DT_readState();
/** データ読み込み */
PUBLIC bool_t bS11059_02DT_readData(tsS11059_02DT_data *spData);
/** 計測完了判定 */
PUBLIC bool_t bS11059_02DT_isDataReady();

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* S11059_02DT_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
