/****************************************************************************
 *
 * MODULE :Debug functions source file
 *
 * CREATED:2015/05/03 17:11:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:各モジュールのデバッグコードを実装
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
/***        ToCoNet Include files                                         ***/
/****************************************************************************/
#include "ToCoNet.h"
#include "ToCoNet_mod_prototype.h"

/****************************************************************************/
/***        User Include files                                            ***/
/****************************************************************************/
#include "debug_check.h"
#include "framework.h"
#include "timer_util.h"
#include "i2c_util.h"
#include "st7032i.h"
#include "sha256.h"
#include "ds3231.h"
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
//PRIVATE uint8 decToBcd(uint8 val);
//PRIVATE uint8 bcdToDec(uint8 val);
//PRIVATE void vi2cReadScan();
// 乱数文字列生成
PRIVATE void randString(uint8 *str, uint32 len);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
// デバッグ主処理
PUBLIC void vDEBUG_main() {
	// Tick Timerの初期化
	vTimerUtil_initTick(40);
	// Watchdog Timerの初期化
	vTimerUtil_initWatchdog(3);
	// デバッグ初期処理
	vDEBUG_init();
	vDEBUG_dispMsg("vDEBUG_main Start\n");
	//=========================================================================
//	uint32 highVal = 4294967295;
//	uint32 nextVal = highVal + 1;
//	vfPrintf(&sSerStream, "vInitHardware Test HIGH:%08X NEXT:%08X\n", highVal, nextVal);
//	uint32 val32 = 0x68;
//	uint8  val8 = (uint8)val32;
//	vfPrintf(&sSerStream, "vInitHardware Test 32:%08X 8:%02X \n", val32, val8);
//	vTimerUtil_waitTickMSec(1);
//	val8 = (uint8)(val32 & 0x000000FF);
//	vfPrintf(&sSerStream, "vInitHardware Test 32:%08X 8:%02X \n", val32, val8);
//	vTimerUtil_waitTickMSec(1);
//	uint8 u8Val = 5;
//	vfPrintf(&sSerStream, "vInitHardware Test ORG:%02X RESULT:%02X \n", u8Val, u8Val | 0x80);
//	vTimerUtil_waitTickMSec(1);
	//=========================================================================
	// I2Cバスの初期化
//	vI2C_init(31);	// 100KHzで動作
	vI2C_init(47);	// 66KHzで動作
	// LCDデバッグ
//	vST7032I_debug();
	// SHA256デバッグコード
//	vSHA256_debug();
	// DS3231デバッグコード
//	vDS3231_debug();
	// Timer Utilデバッグコード
//	vTimerUtil_debug();
	// ランダム関数デバッグ
//	vRandom_debug();
	// アナログポートデバッグ
//	vAnalogue_debug();
	vDEBUG_dispMsg("vDEBUG_main End\n");
}

// デバッグ初期処理
PUBLIC void vDEBUG_init() {
	// デバッグ出力の初期化　シリアルストリームに出力
	ToCoNet_vDebugInit(&sSerStream);
	// デバッグレベルの設定
	ToCoNet_vDebugLevel(0);
}

// デバッグメッセージの表示処理
PUBLIC void vDEBUG_dispMsg(const char* fmt) {
	vfPrintf(&sSerStream, fmt);
	u32TimerUtil_waitTickMSec(5);
}

// アナログポートデバッグ
PUBLIC void vAnalogue_debug() {
	vDEBUG_dispMsg("\n\n\n");
	vDEBUG_dispMsg("==================================================\n");
	vDEBUG_dispMsg("vAnalogue_debug Begin\n");
	vDEBUG_dispMsg("==================================================\n");
	//========================================================================
	// アナログポート初期化
	//========================================================================
	// 初期処理：ADC開始とアナログポート有効化
	bIOUtil_adcInit();

	//========================================================================
	// アナログポート読込
	//========================================================================
	// ADCを有効化
	vAHI_AdcEnable(E_AHI_ADC_CONTINUOUS,	// 連続モード（値の読み出しはサンプリング周期のタイマー割り込み）
					E_AHI_AP_INPUT_RANGE_2,	// 0-2.4V まで
					E_AHI_ADC_SRC_ADC_1);	// 入力ポート
	// サンプリングを開始
	vAHI_AdcStartSample();  // 開始
	u32TimerUtil_waitTickUSec(100);
	uint32 idx;
	for (idx = 0; idx < 10; idx++) {
		// アナログ入力読込（10bit）
		vfPrintf(&sSerStream, "vAnalogue_debug Analogue Read:%d\n", (int32)u16AHI_AdcRead());
		u32TimerUtil_waitTickMSec(200);
	}

	//========================================================================
	// アナログポート終了
	//========================================================================
	// ADCを無効化
	vAHI_AdcDisable();  // 開始

	//========================================================================
	// キーパッド読み込み
	//========================================================================
	uint8 key;
	for (idx = 0; idx < 10; idx++) {
		key = u8IOUtil_readKeypad(E_AHI_ADC_SRC_ADC_1);
		// アナログ入力読込（10bit）
		vfPrintf(&sSerStream, "vAnalogue_debug Button Read:%c\n", key);
		u32TimerUtil_waitTickMSec(200);
	}
}

// ランダム関数デバッグ
PUBLIC void vRandom_debug() {
	vfPrintf(&sSerStream, "\n\n\n");
	u32TimerUtil_waitTickMSec(1);
	vfPrintf(&sSerStream, "==================================================\n");
	u32TimerUtil_waitTickMSec(1);
	vfPrintf(&sSerStream, "vRandom_debug Begin\n");
	u32TimerUtil_waitTickMSec(1);
	vfPrintf(&sSerStream, "==================================================\n");
	u32TimerUtil_waitTickMSec(1);
	//========================================================================
	// 単一乱数生成
	//========================================================================
	// 単一乱数生成、割り込み処理無効
	vAHI_StartRandomNumberGenerator(E_AHI_RND_SINGLE_SHOT, E_AHI_INTS_DISABLED);
	// 連続実行
	uint16 randVal = 0;
	uint32 cnt     = 0;
	uint32 idx;
	for (idx = 0; idx < 10000; idx++) {
		// 乱数の生成状況判定
		if(bAHI_RndNumPoll()) {
			// 乱数の取得
			randVal = u16AHI_ReadRandomNumber();
			cnt++;
		}
	}
	vfPrintf(&sSerStream, "vRandom_debug Single Gen Cnt:%d %05d\n", cnt, randVal);
	u32TimerUtil_waitTickMSec(1);
	//========================================================================
	// 継続的に乱数を生成
	//========================================================================
	// 継続的に乱数生成、割り込み処理無効
	vAHI_StartRandomNumberGenerator(E_AHI_RND_CONTINUOUS, E_AHI_INTS_DISABLED);
	// 連続実行
	cnt = 0;
	for (idx = 0; idx < 10000; idx++) {
		// 乱数の生成状況判定
		if(bAHI_RndNumPoll()) {
			// 乱数の取得
			randVal = u16AHI_ReadRandomNumber();
			cnt++;
		}
	}
	// 乱数生成器停止
	vAHI_StopRandomNumberGenerator();
	//========================================================================
	// 乱数の生成頻度計測
	//========================================================================
	// Tick Timer 初期化
	vTimerUtil_initTick(40);
	// Watchdog Timerの初期化
	vTimerUtil_initWatchdog(3);
	// 継続的に乱数生成、割り込み処理無効
	vAHI_StartRandomNumberGenerator(E_AHI_RND_CONTINUOUS, E_AHI_INTS_DISABLED);
	// 連続実行
	uint16 befVal;
	uint32 current, before, err, total;
	err   = 0;
	total = 0;
	cnt = 0;
	befVal = u16AHI_ReadRandomNumber();
	before = u32AHI_TickTimerRead();
	while (cnt < 1000) {
		// 乱数の生成状況判定
		if(bAHI_RndNumPoll()) {
			// 乱数の取得
			randVal = u16AHI_ReadRandomNumber();
			current = u32AHI_TickTimerRead();
			if (randVal == befVal) err++;
			total  += current - before;
			befVal = randVal;
			before  = current;
			cnt++;
		}
	}
	// 乱数生成器停止
	vAHI_StopRandomNumberGenerator();
	// 結果表示
	vfPrintf(&sSerStream, "vRandom_debug Multi  Gen Cnt:%d Err:%d Avg:%d usec\n",
			cnt, err, total / cnt / 16);
	u32TimerUtil_waitTickMSec(1);
}

// Timer Utilデバッグ
PUBLIC void vTimerUtil_debug() {
	//=========================================================================
	// 精度チェック
	//=========================================================================
	DS3231_datetime dt;
	bDS3231_getDatetime(&dt);
	vfPrintf(&sSerStream, "vTimerUtil_debug Check Start:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
//	uint8 endMin = dt.u8Minutes + 2;
//	uint32 idx = 0;
	uint32 current = u32AHI_TickTimerRead();
//	while (dt.u8Minutes < endMin) {
		u32TimerUtil_waitTickMSec(120000);
//		bDS3231_getDatetime(&dt);
//		idx++;
//	}
	uint32 tickCnt = u32AHI_TickTimerRead() - current;
	bDS3231_getDatetime(&dt);
	vfPrintf(&sSerStream, "vTimerUtil_debug Check End  :%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(1);
//	vfPrintf(&sSerStream, "vTimerUtil_debug Check End Index:%d\n", idx);
//	vTimerUtil_waitTickMSec(1);
	vfPrintf(&sSerStream, "vTimerUtil_debug Check Tick:%d\n", (tickCnt / 120));
	u32TimerUtil_waitTickMSec(1);
	//=========================================================================
	// 精度チェック その２
	//=========================================================================
	//	uint32 nowMs, befTime, aftTime;
	//	uint64 totalTime;
	//	uint64 befUSec, aftUSec;
	//	uint64 waitUSec;
	//	uint32 endMSec;
	//	totalTime = 0;
	//	nowMs = u32TickCount_ms;
	//	befTime = u32AHI_TickTimerRead();
	//	aftTime = u32AHI_TickTimerRead();
	//	while (aftTime >= befTime) {
	//		befTime = aftTime;
	//		aftTime = u32AHI_TickTimerRead();
	//	}
	//	totalTime = 0xFFFFFFFF00000000ULL;
	//	totalTime = u64TimerUtil_readCurrentUsec();
	//	totalTime = (0x00000000FFFFFFFFULL << 32);
	//	totalTime = totalTime | 0xF0F0F0F0;
	//	totalTime = befTime;
	//	nowMs = u32TickCount_ms;
	//	while (u32TickCount_ms == nowMs) {
	//		totalTime += u32TimerUtil_joinTick(totalTime + 250);
	//	}
	//	nowMs = u32TickCount_ms;
	//	while (u32TickCount_ms == nowMs) {
	//		aftTime = u32AHI_TickTimerRead();
	//	}
	//	totalTime = u32TimerUtil_joinTick(befTime + 8192);
	//	totalTime += u32TimerUtil_joinTick(totalTime + 8192);
	//	totalTime += u32TimerUtil_joinTick(totalTime + 16384);
	//	totalTime += u32TimerUtil_joinTick(totalTime + 16384);
	//	u32TimerUtil_waitTickUSec(1000);
	//	u32TimerUtil_waitTickUSec(1000);
	//	u32TimerUtil_waitTickUSec(1000);
	//	u32TimerUtil_waitTickUSec(500);
	//	u32TimerUtil_waitTickUSec(500);
	//	u32TimerUtil_waitTickUSec(500);
	//	u32TimerUtil_waitTickUSec(500);
	//	u32TimerUtil_waitTickUSec(500);
	//	u32TimerUtil_waitTickUSec(500);
	//	u32TimerUtil_waitTickUSec(500);
	//	aftTime = u32AHI_TickTimerRead();
	//	vfPrintf(&sSerStream, "MS:%08d vProcessEvCore Bef TICK:%08d Rate:%d\n",
	//			nowMs, befTime, u8AHI_GetSystemClkRate());
	//	vfPrintf(&sSerStream, "MS:%08d vProcessEvCore Aft TICK:%08d Total:%d %d\n",
	//			u32TickCount_ms, aftTime, (uint32)(totalTime >> 32), (uint32)totalTime);
	//	vfPrintf(&sSerStream, "MS:%08d vProcessEvCore Aft TICK:%08d Total:%d %d\n",
	//			u32TickCount_ms, aftTime, (uint8)((uint8)10-(uint8)250), (uint16)65000 + (uint8)100);
	//	waitUSec = 0;
	//	endMSec  = 100;
	//	befUSec  = u64TimerUtil_readCurrentUsec();
	//	waitUSec += u32TimerUtil_waitTickUSec(endMSec);
	//	waitUSec += u32TimerUtil_waitTickUSec(endMSec);
	//	waitUSec += u32TimerUtil_waitTickUSec(endMSec);
	//	waitUSec += u32TimerUtil_waitTickUSec(endMSec);
	//	waitUSec += u32TimerUtil_waitTickUSec(endMSec);
	//	waitUSec += u32TimerUtil_waitTickUSec(endMSec);
	//	waitUSec += u32TimerUtil_waitTickUSec(endMSec);
	//	waitUSec += u32TimerUtil_waitTickUSec(endMSec);
	//	waitUSec += u32TimerUtil_waitTickUSec(endMSec);
	//	waitUSec += u32TimerUtil_waitTickUSec(endMSec);
	//	waitUSec = u32TimerUtil_waitTickUSec(30);
	//	aftUSec  = u64TimerUtil_readCurrentUsec();
	//	vfPrintf(&sSerStream, "MS:%08d vProcessEvCore Before:%08X%08X\n",
	//			u32TickCount_ms, (uint32)(befUSec >> 32), (uint32)befUSec);
	//	vfPrintf(&sSerStream, "MS:%08d vProcessEvCore After :%08X%08X\n",
	//			u32TickCount_ms, (uint32)(aftUSec >> 32), (uint32)aftUSec);
	//	vfPrintf(&sSerStream, "MS:%08d vProcessEvCore Wait  :%08d/%08d\n",
	//			u32TickCount_ms, (uint32)waitUSec, (uint32)(aftUSec - befUSec - 10));
	//	vSerialFlush();
}

// LCDデバッグ
PUBLIC void vST7032I_debug() {
	vDEBUG_dispMsg("vST7032I_debug Start\n");
	// デバイスの選択
	ST7032i_state state;
	state.u8Address = I2C_ADDR_ST7032I;
	bool_t result = bST7032I_devSelect(&state);
	// LCDの初期化
	result = bST7032I_init();
	// カーソル点滅表示
	bST7032I_dispControl(TRUE, TRUE, TRUE, TRUE);
//	bST7032I_dispControl(TRUE, FALSE, FALSE, TRUE);
	vDEBUG_dispMsg("vST7032I_debug No.1\n");
	// LCDへの文字列書き込み
	result = bST7032I_setCursor(0, 0);
	vDEBUG_dispMsg("vST7032I_debug No.2\n");
	result = bST7032I_writeChar('X');
	vDEBUG_dispMsg("vST7032I_debug No.3\n");
	result = bST7032I_setCursor(1, 2);
	vDEBUG_dispMsg("vST7032I_debug No.4\n");
	result = bST7032I_writeString("Test String");
	if (result) {
		vDEBUG_dispMsg("vST7032I_debug No.5 OK!\n");
	} else {
		vDEBUG_dispMsg("vST7032I_debug No.5 NG!\n");
	}
	// アイコン表示
	uint8 iconConv[13] = {
		1, 11, 21, 31, 36, 37, 46, 56, 66, 67, 68, 69, 76
	};
	int i;
	for (i = 0; i < 13; i++) {
		if (!bST7032I_dispIcon(iconConv[i])) {
			vDEBUG_dispMsg("vST7032I_debug ICON NG!\n");
		}
		u32TimerUtil_waitTickMSec(50);
	}
	// コントラスト設定
	for (i = 0; i < 64; i++) {
		// コントラスト設定
		bST7032I_setContrast(i);
		u32TimerUtil_waitTickMSec(3);
	}
	for (i = 63; i >= 0; i--) {
		// コントラスト設定
		bST7032I_setContrast(i);
		u32TimerUtil_waitTickMSec(3);
	}
	// ディスプレイクリア
	if (!bST7032I_clearScreen()) {
		vDEBUG_dispMsg("vST7032I_debug Clear Screen NG!\n");
	}
	// アイコンクリア
	if (!bST7032I_clearICON()) {
		vDEBUG_dispMsg("vST7032I_debug Clear ICON NG!\n");
	}
	// カーソル移動：先頭へ
//	bST7032I_setCursor(0, 5);
//	bST7032I_returnHome();
//	bST7032I_cursorShiftR();
//	bST7032I_cursorShiftR();
//	bST7032I_cursorShiftR();
//	result = bST7032I_setCursor(0, 3);
//	vDEBUG_dispMsg("vST7032I_debug No.6 %1x\n", result);
//	bST7032I_writeChar('X');
//	bST7032I_returnHome();
//	bST7032I_clearScreen();
	vDEBUG_dispMsg("vST7032I_debug End\n");
}

// DS3231RTCモジュールデバッグ
PUBLIC void vDS3231_debug() {
	vfPrintf(&sSerStream, "\n");
	u32TimerUtil_waitTickMSec(1);
	vfPrintf(&sSerStream, "==================================================\n");
	u32TimerUtil_waitTickMSec(1);
	vfPrintf(&sSerStream, "vDS3231_debug Test\n");
	u32TimerUtil_waitTickMSec(1);
	vfPrintf(&sSerStream, "==================================================\n");
	u32TimerUtil_waitTickMSec(1);
	// デバイスの選択
	bDS3231_deviceSelect(I2C_ADDR_DS3231);
	//=========================================================================
//	vfPrintf(&sSerStream, "vDS3231_debug READ TEST No.1\n");
//	vTimerUtil_waitTickMSec(1);
	// コントロール初期化
	bool_t result[12];
//	bI2C_startWrite(104);
//	result[0] = u8I2C_write(0x0E);
//	result[1] = u8I2C_write(0x1C);
//	bI2C_stopACK();
	//=========================================================================
//	vfPrintf(&sSerStream, "vDS3231_debug READ TEST No.2\n");
//	vTimerUtil_waitTickMSec(1);
	// 時刻初期化
//	bI2C_startWrite(104);
//	result[2] = u8I2C_write(0);
//	result[3] = u8I2C_write(decToBcd(30));
//	result[4] = u8I2C_write(decToBcd(20));
//	result[5] = u8I2C_write(decToBcd(4));
//	result[6] = u8I2C_write(decToBcd(4));
//	result[7] = u8I2C_write(decToBcd(6));
//	result[8] = u8I2C_write(decToBcd(5));
//	result[9] = u8I2C_write(decToBcd(15));
//	bI2C_stopACK();
	//=========================================================================
//	vfPrintf(&sSerStream, "vDS3231_debug READ TEST No.3\n");
//	char rsVals[] = "01";
//	// LCDの初期化
//	bACM1602NI_init();
//	// カーソル点滅表示
//	bACM1602NI_dispControl(TRUE, TRUE, FALSE);
//	bACM1602NI_clearScreen();
//	bACM1602NI_setCursor(1, 0);
//	bACM1602NI_writeString("RS:");
//	bACM1602NI_writeChar(rsVals[result[0]]);
//	bACM1602NI_writeString(":");
//	bACM1602NI_writeChar(rsVals[result[1]]);
//	vTimerUtil_waitTickMSec(1000);
	//=========================================================================
//	vfPrintf(&sSerStream, "vDS3231_debug READ Start Write:%02X\n", bI2C_startWrite(0x68));
//	vTimerUtil_waitTickMSec(2);
//	vfPrintf(&sSerStream, "vDS3231_debug READ Write  Addr:%02X\n", u8I2C_write(0));
//	vTimerUtil_waitTickMSec(2);
//	vfPrintf(&sSerStream, "vDS3231_debug READ Start  Read:%02X\n", bI2C_startRead(0x68));
//	vTimerUtil_waitTickMSec(2);
//	uint8 pkt[1];
//	result[11] = bI2C_read(pkt, 1, TRUE);
//	vfPrintf(&sSerStream, "vDS3231_debug Read No.5 %1d %02d \n", result[11], bcdToDec(pkt[0]));
//	vTimerUtil_waitTickMSec(1);
	int idx;
//	for (idx = 0; idx < 18; idx++) {
//		result[11] = bI2C_read(pkt, 1, TRUE);
//		vfPrintf(&sSerStream, "vDS3231_debug No.%03d %1d %02X \n", idx, result[11], pkt[0]);
//		vTimerUtil_waitTickMSec(2);
//	}
//	result[11] = bI2C_read(pkt, 1, FALSE);
//	vfPrintf(&sSerStream, "vDS3231_debug No.%03d %1d %02X \n", idx, result[11], pkt[0]);
//	vTimerUtil_waitTickMSec(2);
//	bI2C_stopACK();
//	vTimerUtil_waitTickMSec(1000);
	//=========================================================================
//	vfPrintf(&sSerStream, "vDS3231_debug READ TEST No.4\n");
//	bI2C_startWrite(0x68);
//	result[10] = u8I2C_write(0);
//	bI2C_startRead(104);
//	for (idx = 0; idx < 12; idx++) {
//		result[11] = bI2C_read(pkt, 1, TRUE);
//		vfPrintf(&sSerStream, "vDS3231_debug No.%02X %d %02d\n", idx, result[11], bcdToDec(pkt[0]));
//		vTimerUtil_waitTickMSec(5);
//	}
//	result[11] = bI2C_read(pkt, 1, FALSE);
//	vfPrintf(&sSerStream, "vDS3231_debug No.%02X %d %02d\n", idx, result[11], bcdToDec(pkt[0]));
//	vTimerUtil_waitTickMSec(5);
//	bI2C_stopACK();
//	vTimerUtil_waitTickMSec(1000);
	//=========================================================================
//	vfPrintf(&sSerStream, "vDS3231_debug READ TEST No.5\n");
//	// アドレス書き込み
//	vAHI_SiMasterWriteSlaveAddr(104, FALSE);    //FALSE=WRITE
//	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT, E_AHI_SI_NO_STOP_BIT,
//							E_AHI_SI_NO_SLAVE_READ, E_AHI_SI_SLAVE_WRITE,
//							E_AHI_SI_SEND_ACK,E_AHI_SI_NO_IRQ_ACK);
//	while (bAHI_SiMasterPollTransferInProgress());
//	if (!bAHI_SiMasterCheckRxNack()) {
//		vAHI_SiMasterWriteData8(0);
//		bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT, E_AHI_SI_NO_STOP_BIT,
//							E_AHI_SI_NO_SLAVE_READ, E_AHI_SI_SLAVE_WRITE,
//							E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK);
//	}
//	while (bAHI_SiMasterPollTransferInProgress());
//	bAHI_SiMasterCheckRxNack();
//	// 読み込み開始
//	vAHI_SiMasterWriteSlaveAddr(104, TRUE);
//	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT, E_AHI_SI_NO_STOP_BIT,
//							E_AHI_SI_NO_SLAVE_READ, E_AHI_SI_SLAVE_WRITE,
//							E_AHI_SI_SEND_ACK,E_AHI_SI_NO_IRQ_ACK);
//	while (bAHI_SiMasterPollTransferInProgress()) u32TimerUtil_waitTickUSec(5);
//	if (!bAHI_SiMasterCheckRxNack()) {
//		for (idx = 0; idx < 18; idx++) {
//			bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT, E_AHI_SI_NO_STOP_BIT,
//								E_AHI_SI_SLAVE_READ, E_AHI_SI_NO_SLAVE_WRITE,
//								E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK);    //READ
//			while (bAHI_SiMasterPollTransferInProgress());
//			vfPrintf(&sSerStream, "vDS3231_debug No.%02X %02d\n", idx, bcdToDec(u8AHI_SiMasterReadData8()));
//			vTimerUtil_waitTickMSec(5);
//		}
//	}
//	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT, E_AHI_SI_NO_STOP_BIT,
//						E_AHI_SI_SLAVE_READ, E_AHI_SI_NO_SLAVE_WRITE,
//						E_AHI_SI_SEND_NACK, E_AHI_SI_NO_IRQ_ACK);    //READ
//	while (bAHI_SiMasterPollTransferInProgress());
//	vfPrintf(&sSerStream, "vDS3231_debug No.%02X %02d\n", 18, bcdToDec(u8AHI_SiMasterReadData8()));
//	vTimerUtil_waitTickMSec(5);
//	bI2C_stopNACK();
	//=========================================================================
	// 現在日時
	//=========================================================================
	vfPrintf(&sSerStream, "vDS3231_debug READ TEST 現在時刻\n");
	u32TimerUtil_waitTickMSec(1);
	DS3231_datetime dt;
	// 日付表示
	bDS3231_getDatetime(&dt);
	vfPrintf(&sSerStream, "vDS3231_debug Bef Date:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(1000);
	bDS3231_getDatetime(&dt);
	vfPrintf(&sSerStream, "vDS3231_debug Aft Date:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(1);
	// 日付更新
	dt.u16Year    = 2015;
	dt.u8Month   = 6;
	dt.u8Day     = 24;
	dt.u8Wday    = 3;
	dt.u8Hour    = 1;
	dt.u8Minutes = 30;
	dt.u8Seconds = 0;
	bDS3231_setDatetime(&dt);
	vDEBUG_dispMsg("vDS3231_debug Time set end!\n");
	// 日付表示
	bDS3231_getDatetime(&dt);
	vfPrintf(&sSerStream, "vDS3231_debug Date:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(1);
	//=========================================================================
	// アラーム１
	//=========================================================================
	// アラーム１日付表示
	bDS3231_getAlarm1(&dt);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 1 Bef:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(5);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 1 Bef:%1X:%1X:%1X:%1X:%1X\n",
			dt.bDayValidFlg, dt.bWdayValidFlg, dt.bHourValidFlg, dt.bMinutesValidFlg, dt.bSecondsValidFlg);
	u32TimerUtil_waitTickMSec(5);
	// 日付更新
	dt.u16Year    = 2015;
	dt.u8Month   = 7;
	dt.u8Day     = 1;
	dt.u8Wday    = 3;
	dt.u8Hour    = 2;
	dt.u8Minutes = 30;
	dt.u8Seconds = 10;
	dt.bDayValidFlg     = TRUE;
	dt.bWdayValidFlg    = TRUE;
	dt.bHourValidFlg    = TRUE;
	dt.bMinutesValidFlg = TRUE;
	dt.bSecondsValidFlg = TRUE;
	bDS3231_setAlarm1(&dt);
	// アラーム１日付表示
	bDS3231_getAlarm1(&dt);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 1 Aft:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(5);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 1 Aft:%1X:%1X:%1X:%1X:%1X\n",
			dt.bDayValidFlg, dt.bWdayValidFlg, dt.bHourValidFlg, dt.bMinutesValidFlg, dt.bSecondsValidFlg);
	u32TimerUtil_waitTickMSec(5);
	// 日付更新
	dt.u16Year    = 2015;
	dt.u8Month   = 1;
	dt.u8Day     = 30;
	dt.u8Wday    = 3;
	dt.u8Hour    = 1;
	dt.u8Minutes = 2;
	dt.u8Seconds = 3;
	dt.bDayValidFlg     = TRUE;
	dt.bWdayValidFlg    = TRUE;
	dt.bHourValidFlg    = FALSE;
	dt.bMinutesValidFlg = TRUE;
	dt.bSecondsValidFlg = FALSE;
	bDS3231_setAlarm1(&dt);
	// アラーム１日付表示
	bDS3231_getAlarm1(&dt);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 1 Aft:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(5);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 1 Aft:%1X:%1X:%1X:%1X:%1X\n",
			dt.bDayValidFlg, dt.bWdayValidFlg, dt.bHourValidFlg, dt.bMinutesValidFlg, dt.bSecondsValidFlg);
	u32TimerUtil_waitTickMSec(5);
	// 日付更新
	dt.u16Year    = 0;
	dt.u8Month   = 0;
	dt.u8Day     = 25;
	dt.u8Wday    = 1;
	dt.u8Hour    = 13;
	dt.u8Minutes = 15;
	dt.u8Seconds = 57;
	dt.bDayValidFlg     = FALSE;
	dt.bWdayValidFlg    = TRUE;
	dt.bHourValidFlg    = TRUE;
	dt.bMinutesValidFlg = FALSE;
	dt.bSecondsValidFlg = TRUE;
	bDS3231_setAlarm1(&dt);
	// アラーム１日付表示
	bDS3231_getAlarm1(&dt);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 1 Aft:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(5);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 1 Aft:%1X:%1X:%1X:%1X:%1X\n",
			dt.bDayValidFlg, dt.bWdayValidFlg, dt.bHourValidFlg, dt.bMinutesValidFlg, dt.bSecondsValidFlg);
	u32TimerUtil_waitTickMSec(5);
	//=========================================================================
	// アラーム２
	//=========================================================================
	// アラーム２日付表示
	bDS3231_getAlarm2(&dt);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 2 Bef:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(5);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 2 Bef:%1X:%1X:%1X:%1X:%1X\n",
			dt.bDayValidFlg, dt.bWdayValidFlg, dt.bHourValidFlg, dt.bMinutesValidFlg, dt.bSecondsValidFlg);
	u32TimerUtil_waitTickMSec(5);
	// 日付更新
	dt.u16Year    = 0;
	dt.u8Month   = 0;
	dt.u8Day     = 7;
	dt.u8Wday    = 4;
	dt.u8Hour    = 6;
	dt.u8Minutes = 5;
	dt.u8Seconds = 9;
	dt.bDayValidFlg     = TRUE;
	dt.bWdayValidFlg    = TRUE;
	dt.bHourValidFlg    = TRUE;
	dt.bMinutesValidFlg = TRUE;
	dt.bSecondsValidFlg = TRUE;
	bDS3231_setAlarm2(&dt);
	// アラーム２日付表示
	bDS3231_getAlarm2(&dt);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 2 Aft:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(5);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 2 Aft:%1X:%1X:%1X:%1X:%1X\n",
			dt.bDayValidFlg, dt.bWdayValidFlg, dt.bHourValidFlg, dt.bMinutesValidFlg, dt.bSecondsValidFlg);
	u32TimerUtil_waitTickMSec(5);
	// 日付更新
	dt.u16Year    = 0;
	dt.u8Month   = 0;
	dt.u8Day     = 25;
	dt.u8Wday    = 1;
	dt.u8Hour    = 13;
	dt.u8Minutes = 20;
	dt.u8Seconds = 10;
	dt.bDayValidFlg     = TRUE;
	dt.bWdayValidFlg    = TRUE;
	dt.bHourValidFlg    = FALSE;
	dt.bMinutesValidFlg = TRUE;
	dt.bSecondsValidFlg = FALSE;
	bDS3231_setAlarm2(&dt);
	// アラーム２日付表示
	bDS3231_getAlarm2(&dt);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 2 Aft:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(5);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 2 Aft:%1X:%1X:%1X:%1X:%1X\n",
			dt.bDayValidFlg, dt.bWdayValidFlg, dt.bHourValidFlg, dt.bMinutesValidFlg, dt.bSecondsValidFlg);
	u32TimerUtil_waitTickMSec(5);
	// 日付更新
	dt.u16Year    = 0;
	dt.u8Month   = 0;
	dt.u8Day     = 25;
	dt.u8Wday    = 2;
	dt.u8Hour    = 13;
	dt.u8Minutes = 15;
	dt.u8Seconds = 30;
	dt.bDayValidFlg     = FALSE;
	dt.bWdayValidFlg    = TRUE;
	dt.bHourValidFlg    = TRUE;
	dt.bMinutesValidFlg = FALSE;
	dt.bSecondsValidFlg = TRUE;
	bDS3231_setAlarm2(&dt);
	// アラーム２日付表示
	bDS3231_getAlarm2(&dt);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 2 Aft:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(1);
	vfPrintf(&sSerStream, "vDS3231_debug Alarm 2 Aft:%1X:%1X:%1X:%1X:%1X\n",
			dt.bDayValidFlg, dt.bWdayValidFlg, dt.bHourValidFlg, dt.bMinutesValidFlg, dt.bSecondsValidFlg);
	u32TimerUtil_waitTickMSec(1);
	//=========================================================================
	// 現在温度
	//=========================================================================
	// 現在温度表示
	float tempVal = fDS3231_getTemperature();
	uint32 tempUp = tempVal;
	uint32 tempLw = (uint32)(tempVal * 100 - tempUp * 100) % 100;
	vfPrintf(&sSerStream, "vDS3231_debug Temperature: %3d.%d ℃ \n", tempUp, tempLw);
	u32TimerUtil_waitTickMSec(1);

	//=========================================================================
	// 制御情報表示
	//=========================================================================
	// 制御情報表示
	DS3231_control control;
	result[0] = bDS3231_getControl(&control);
	vfPrintf(&sSerStream, "vDS3231_debug Control Read Result:%d\n", result[0]);
	u32TimerUtil_waitTickMSec(1);
	vfPrintf(&sSerStream, "vDS3231_debug Control Read:%1d,%1d,%1d,%1d,%1d,%1d,%1d,%1d\n",
			control.bEoscFlg, control.bBbsqwFlg, control.bConvFlg, control.bRs2, control.bRs1,
			control.bIntcnFlg, control.bA2ieFlg, control.bA1ieFlg);
	u32TimerUtil_waitTickMSec(1);

	//=========================================================================
	// 制御情報設定
	//=========================================================================
	// 制御情報設定
	control.bEoscFlg  = TRUE;
	control.bBbsqwFlg = TRUE;
	control.bConvFlg  = TRUE;
	control.bRs2      = FALSE;
	control.bRs1      = FALSE;
	control.bIntcnFlg = FALSE;
	control.bA2ieFlg  = TRUE;
	control.bA1ieFlg  = TRUE;
	result[0] = bDS3231_setControl(&control);
	vfPrintf(&sSerStream, "vDS3231_debug Control Set  Result:%d\n", result[0]);
	u32TimerUtil_waitTickMSec(1);
	result[0] = bDS3231_getControl(&control);
	vfPrintf(&sSerStream, "vDS3231_debug Control Read Result:%d\n", result[0]);
	u32TimerUtil_waitTickMSec(1);
	vfPrintf(&sSerStream, "vDS3231_debug Control Read:%1d,%1d,%1d,%1d,%1d,%1d,%1d,%1d\n",
			control.bEoscFlg, control.bBbsqwFlg, control.bConvFlg, control.bRs2, control.bRs1,
			control.bIntcnFlg, control.bA2ieFlg, control.bA1ieFlg);
	u32TimerUtil_waitTickMSec(1);
	// 制御情報設定
	control.bEoscFlg  = FALSE;
	control.bBbsqwFlg = FALSE;
	control.bConvFlg  = FALSE;
	control.bRs2      = TRUE;
	control.bRs1      = TRUE;
	control.bIntcnFlg = TRUE;
	control.bA2ieFlg  = FALSE;
	control.bA1ieFlg  = FALSE;
	result[0] = bDS3231_setControl(&control);
	vfPrintf(&sSerStream, "vDS3231_debug Control Set  Result:%d\n", result[0]);
	u32TimerUtil_waitTickMSec(1);
	result[0] = bDS3231_getControl(&control);
	vfPrintf(&sSerStream, "vDS3231_debug Control Read Result:%d\n", result[0]);
	u32TimerUtil_waitTickMSec(1);
	vfPrintf(&sSerStream, "vDS3231_debug Control Read:%1d,%1d,%1d,%1d,%1d,%1d,%1d,%1d\n",
			control.bEoscFlg, control.bBbsqwFlg, control.bConvFlg, control.bRs2, control.bRs1,
			control.bIntcnFlg, control.bA2ieFlg, control.bA1ieFlg);
	u32TimerUtil_waitTickMSec(1);

	//=========================================================================
	// ステータス情報
	//=========================================================================
	// ステータス情報表示
	DS3231_status status;
	result[0] = bDS3231_getStatus(&status);
	vfPrintf(&sSerStream, "vDS3231_debug Status Result:%d\n", result[0]);
	u32TimerUtil_waitTickMSec(1);
	vfPrintf(&sSerStream, "vDS3231_debug Status:%1d,%1d,%1d,%1d,%1d\n",
			status.bOsFlg, status.bEn32khzFlg, status.bBusyFlg, status.bAlarm2Flg, status.bAlarm1Flg);
	u32TimerUtil_waitTickMSec(1);

	//=========================================================================
	// 負荷テスト
	//=========================================================================
	result[0] = bDS3231_getDatetime(&dt);
	vfPrintf(&sSerStream, "vDS3231_debug Start:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(1);
	for (idx = 0; idx < 1000; idx++) {
		result[0] = bDS3231_getDatetime(&dt);
//		if (!result[0]) {
//			vfPrintf(&sSerStream, "vDS3231_debug Date Get Error:%05d\n", idx);
//			vTimerUtil_waitTickMSec(1);
//			vfPrintf(&sSerStream, "vDS3231_debug Date:%04d/%02d/%02d %1d %02d:%02d:%02d\n",
//					dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
//			vTimerUtil_waitTickMSec(1);
//		}
	}
	result[0] = bDS3231_getDatetime(&dt);
	vfPrintf(&sSerStream, "vDS3231_debug End  :%04d/%02d/%02d %1d %02d:%02d:%02d\n",
			dt.u16Year, dt.u8Month, dt.u8Day, dt.u8Wday, dt.u8Hour, dt.u8Minutes, dt.u8Seconds);
	u32TimerUtil_waitTickMSec(1);

	//=========================================================================
	// その他
	//=========================================================================
//	vi2cReadScan();
	// 60マイクロ秒待つ
//	u32TimerUtil_waitTickUSec(60);
}

// SHA256デバッグ
PUBLIC void vSHA256_debug() {
	// ハッシュコード生成
	vfPrintf(&sSerStream, "vInitHardware Hash generate No.1\n");
	u32TimerUtil_waitTickMSec(5);
	SHA256_state hashState = sSHA256_newState();
	vfPrintf(&sSerStream, "vInitHardware Hash generate No.2\n");
	u32TimerUtil_waitTickMSec(5);
	uint8 nums[] = "1234567890";
	uint8 orgMsg[] = "Test Msg 0";
	uint32 idx;
	for (idx=0; idx < 10; idx++) {
		orgMsg[9] = nums[idx % 10];
		vfPrintf(&sSerStream, "vInitHardware Hash generate Msg:%s %d\n", orgMsg, orgMsg);
		u32TimerUtil_waitTickMSec(1);
		vSHA256_append(&hashState, orgMsg, 10);
	}
	vfPrintf(&sSerStream, "vInitHardware Hash generate No.3\n");
	u32TimerUtil_waitTickMSec(5);
	uint8 orgMsg2[] = "28_Moji___123456789012345678";
	vSHA256_append(&hashState, orgMsg2, 28);
	vfPrintf(&sSerStream, "vInitHardware Hash generate No.4\n");
	u32TimerUtil_waitTickMSec(5);
	uint8 orgMsg3[] = "64_Moji___123456789012345678901234567890123456789012345678901234";
	vSHA256_append(&hashState, orgMsg3, 64);
	vfPrintf(&sSerStream, "vInitHardware Hash generate No.5\n");
	u32TimerUtil_waitTickMSec(5);
	uint8 orgMsg4[] = "70_Moji___123456789012345678901234567890123456789012345678901234567890";
	vSHA256_append(&hashState, orgMsg4, 70);
	vfPrintf(&sSerStream, "vInitHardware Hash generate No.6\n");
	u32TimerUtil_waitTickMSec(5);
	uint8 pHash[32];
	vSHA256_generateHash(&hashState, pHash);
	vfPrintf(&sSerStream, "vInitHardware Hash generate No.7\n");
	u32TimerUtil_waitTickMSec(5);
	pHash[31] = '\0';
	u32TimerUtil_waitTickMSec(5);
	vfPrintf(&sSerStream, "vInitHardware Hash code:%s\n", pHash);
	u32TimerUtil_waitTickMSec(5);
	char msg[16];
	strncpy(msg, (char*)pHash, 15);
	msg[15] = '\0';
	bACM1602NI_setCursor(1, 0);
	bACM1602NI_writeString(msg);
	//=========================================================================
	// テストコード
	//=========================================================================
	vfPrintf(&sSerStream, "\n\n\n");
	u32TimerUtil_waitTickMSec(5);
	vfPrintf(&sSerStream, "==================================================\n");
	u32TimerUtil_waitTickMSec(5);
	vfPrintf(&sSerStream, "vSHA256_debug Test\n");
	u32TimerUtil_waitTickMSec(5);
	vfPrintf(&sSerStream, "==================================================\n");
	u32TimerUtil_waitTickMSec(5);
	uint8 setStr[] = "123456789+";
	uint8 testStr[300];
	for (idx = 0; idx < 300; idx++) {
		testStr[idx] = setStr[idx % 10];
	}
	hashState = sSHA256_newState();
	vSHA256_append(&hashState, testStr, 300);
	vSHA256_generateHash(&hashState, pHash);
	//=========================================================================
	// 負荷テスト
	//=========================================================================
	// Tick Timer 初期化
	vTimerUtil_initTick(40);
	// Watchdog Timerの初期化
	vTimerUtil_initWatchdog(3);
	uint32 randCnt, before;
	uint32 tryCnt = 0;
	uint32 tickTotal = 0;
	// 乱数文字列生成10回×試行回数10＝100回実施
	for (randCnt = 0; randCnt < 10; randCnt++) {
		// 乱数文字列生成
		randString(testStr, 256);
		// 100回試行
		before = u32AHI_TickTimerRead();
		for (idx = 0; idx < 10; idx++) {
			hashState = sSHA256_newState();
			vSHA256_append(&hashState, testStr, 300);
			vSHA256_generateHash(&hashState, pHash);
			tryCnt++;
		}
		tickTotal += u32AHI_TickTimerRead() - before;
	}
	// 結果表示
	vfPrintf(&sSerStream, "vSHA256_debug Avg:%d usec\n", tickTotal / tryCnt / 16);
	u32TimerUtil_waitTickMSec(1);

}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
// BCD形式に変換
//PRIVATE uint8 decToBcd(uint8 val) {
//  return (((val / 10) << 4) + (val % 10));
//}

//PRIVATE uint8 bcdToDec(uint8 val) {
//   return (uint8)((val >> 4) * 10 + (val & 0x0f));
//}

// I2CデバイスをReadスキャンする
//PRIVATE void vi2cReadScan() {
//	bool_t result;
//	uint8 readBuff[4];
//	int idx;
//	for (idx = 0; idx < 256; idx++) {
//		bI2C_startRead((uint8)idx);
//		memset(readBuff, 0x00 ,4);
//		result = bI2C_read(readBuff, 4, TRUE);
//		vfPrintf(&sSerStream, "I2C Read Scan AD:%02X %X", idx, result);
//		u32TimerUtil_waitTickMSec(2);
//		vfPrintf(&sSerStream, " %02X:%02X:%02X:%02X \n", readBuff[0], readBuff[1], readBuff[2], readBuff[3]);
//		u32TimerUtil_waitTickMSec(2);
//		bI2C_stopNACK();
//	}
//}

// 乱数文字列生成
PRIVATE void randString(uint8 *str, uint32 len) {
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


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
