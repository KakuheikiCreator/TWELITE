/****************************************************************************
 *
 * MODULE :Debug functions source file
 *
 * CREATED:2015/05/03 17:11:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:アプリケーション特有の基本的な処理を実装
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 ****************************************************************************
 * Copyright (c) 2015, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 ****************************************************************************/
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <jendefs.h>
#include <AppHardwareApi.h>

/****************************************************************************/
/***        ToCoNet Definitions                                           ***/
/****************************************************************************/
// Select Modules (define befor include "ToCoNet.h")
#define ToCoNet_USE_MOD_ENERGYSCAN
#define ToCoNet_USE_MOD_NBSCAN

/****************************************************************************/
/***        ToCoNet Definitions                                           ***/
/****************************************************************************/
#include "ToCoNet.h"
#include "ToCoNet_mod_prototype.h"

/****************************************************************************/
/***        User Include files                                            ***/
/****************************************************************************/
#include "debug.h"
#include "framework.h"
#include "timer_util.h"
#include "i2c_util.h"
#include "st7032i.h"
#include "sha256.h"
#include "io_util.h"

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
// デバッグトレース情報
PRIVATE tsTraceInfo sTraceInfo[100];
PRIVATE int iTraceIdx = 0;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/** デバッグメッセージの表示処理 */
PUBLIC void vDEBUG_dispMsg(const char* fmt) {
	vfPrintf(&sSerStream, fmt);
	u32TimerUtil_waitTickMSec(5);
}

/** BCD形式に変換 */
PUBLIC uint8 decToBcd(uint8 val) {
  return (((val / 10) << 4) + (val % 10));
}

/** BCD形式→10進数 */
PUBLIC uint8 bcdToDec(uint8 val) {
   return (uint8)((val >> 4) * 10 + (val & 0x0f));
}

/** 乱数文字列生成 */
PUBLIC void randString(uint8 *str, uint32 len) {
	// 継続的に乱数生成、割り込み処理無効
	vAHI_StartRandomNumberGenerator(E_AHI_RND_CONTINUOUS, E_AHI_INTS_DISABLED);
	// テスト文字列更新
	uint32 idx;
	for (idx = 0; idx < len; idx++) {
		str[idx] = ((u16AHI_ReadRandomNumber() & 0xff00) >> 8);
		u32TimerUtil_waitTickUSec(256);
	}
	// 乱数生成器停止
	vAHI_StopRandomNumberGenerator();
}

/** I2CデバイスをReadスキャンする */
PUBLIC void vi2cReadScan() {
	bool_t result;
	uint8 readBuff[4];
	int idx;
	for (idx = 0; idx < 256; idx++) {
		bI2C_startRead((uint8)idx);
		memset(readBuff, 0x00 ,4);
		result = bI2C_readStop(readBuff, 4, FALSE);
		vfPrintf(&sSerStream, "I2C Read Scan AD:%02X %X", idx, result);
		u32TimerUtil_waitTickMSec(2);
		vfPrintf(&sSerStream, " %02X:%02X:%02X:%02X \n", readBuff[0], readBuff[1], readBuff[2], readBuff[3]);
		u32TimerUtil_waitTickMSec(2);
//		bI2C_stopNACK();
	}
}

/*****************************************************************************
 *
 * NAME: vAddTraceInfo
 *
 * DESCRIPTION:トレース情報追加
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vAddTraceInfo(uint8 pointNo, teEvent eEvent, uint32 u32DeviceID) {
	tsTraceInfo *info;
	int i, size = sizeof(sTraceInfo) / sizeof(sTraceInfo[0]);
	if (iTraceIdx >= size) {
		for (i=0; i < size; i++) {
			info = &sTraceInfo[i];
			vfPrintf(&sSerStream, "%03d,%08d,%03d,%04X,%04X\n",
					i, info->u32TickCountMS, info->iTracePointNo, info->eEvent, info->u32DeviceID);
			SERIAL_vFlush(sSerStream.u8Device);
		}
		// イベントゼロクリア
		iTraceIdx = 0;
	}
	info = &sTraceInfo[iTraceIdx % size];
	info->u32TickCountMS = u32TickCount_ms;
	info->iTracePointNo = pointNo;
	info->eEvent = eEvent;
	info->u32DeviceID = u32DeviceID;
	iTraceIdx++;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
