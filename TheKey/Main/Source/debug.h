/****************************************************************************
 *
 * MODULE :Debug functions header file
 *
 * CREATED:2017/02/19 00:00:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:各モジュールのデバッグコードを実装
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 ****************************************************************************
 * Copyright (c) 2017, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 ****************************************************************************/

#ifndef  DEBUG_H_INCLUDED
#define  DEBUG_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// 構造体：トレース情報
typedef struct {
	// トレースタイミング（ミリ秒）
	uint32 u32TickCountMS;
	// トレースポイント番号
	uint8 iTracePointNo;
	// イベントID
	teEvent eEvent;
	// デバイスID
	uint32 u32DeviceID;
} tsTraceInfo;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/** BCD形式に変換 */
PUBLIC uint8 decToBcd(uint8 val);
/** BCD形式→10進数 */
PUBLIC uint8 bcdToDec(uint8 val);
/** I2CデバイスをReadスキャンする */
PUBLIC void vi2cReadScan();
/** 乱数文字列生成 */
PUBLIC void randString(uint8 *str, uint32 len);
// トレース情報追加
PUBLIC void vAddTraceInfo(uint8 pointNo, teEvent eEvent, uint32 u32DeviceID);


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* DEBUG_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
