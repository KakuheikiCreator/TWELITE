/****************************************************************************
 *
 * MODULE :Timer Utility functions source file
 *
 * CREATED:2015/03/22 21:52:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:
 *   Timer接続機能を提供するユーティリティ関数群
 *   Timer Utility functions (source file)
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
#include <jendefs.h>
#include <AppHardwareApi.h>

/****************************************************************************/
/***        ToCoNet Include files                                         ***/
/****************************************************************************/
#include "ToCoNet.h"

/****************************************************************************/
/***        User Include files                                            ***/
/****************************************************************************/
#include "timer_util.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/** Tick Timerの待ち時間誤差 */
#define TIMER_UTIL_TICK_WAIT_DIFF    (176)
/** Tick Timerとミリ秒の誤差*/
#define TIMER_UTIL_TICK_MSEC_DIFF    (62516)
/** 最大待ち時間（ミリ秒） */
#ifndef TIMER_UTIL_MAX_WAIT_MS
	#define TIMER_UTIL_MAX_WAIT_MS   (100)
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/**
 * 構造体：タイマー情報
 */
typedef struct {
	// 直前に計測したミリ秒単位の値
	volatile uint64 u64TimerUtil_tickLastMsec;
} TimerUtil_Info;

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
// Constant:Watchdog Timeout List(msec)
PRIVATE const uint32 WATCHDOG_LIMITS[] = {
	8, 16, 24, 40, 72, 136, 264, 520, 1032, 2056, 4104, 8200, 16392
};
// 時刻情報
PRIVATE TimerUtil_Info sTimerUtil_info;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/**
 * Tick Timer初期処理
 */
void vTimerUtil_initTick() {
	// TickTimerの初期化
	vAHI_TickTimerConfigure(E_AHI_TICK_TIMER_DISABLE);	// 無効化
	// 割り込み頻度：250マイクロ秒単位
	// ※これ以上高いレートだとWatchdogタイマー処理に支障が出る様子
	vAHI_TickTimerInterval(250 * TIMER_UTIL_TICK_PER_USEC);
	vAHI_TickTimerIntEnable(TRUE);						// 割り込み：ON
	vAHI_TickTimerConfigure(E_AHI_TICK_TIMER_CONT);		// カウントアップ再開
	// WatchDog初期化
	vTimerUtil_initWatchDog(520);
	// 直前に取得したミリ秒(64bit)の初期化
	sTimerUtil_info.u64TimerUtil_tickLastMsec = u32TickCount_ms;
}

/**
 * Watch Dog Timer 初期化処理
 *
 * システムフリーズを検知する為のタイマーを初期化する
 * 指定時間以内にリセットイベントが起きない場合に、システムをリブートする
 * 実際のタイムアウト時間は下記の計算式で求められる
 *
 *     タイムアウト時間 = (2 ^ (Prescale - 1) + 1) x 8ms
 *
 * ※この為、必ずしも引数の通りにはタイムアウトしないので注意
 *
 * @param uint32 タイムアウト時間（ミリ秒）の概算値
 */
PUBLIC void vTimerUtil_initWatchDog(uint32 u32TimeOutMsec) {
	//-------------------------------------------------------------------------
	// if (prescale == 0) {
	//     Timeout Period = 8ms
	// } else if (1 <= u8Prescale && u8Prescale  <= 12) {
	//     Timeout Period = (2の(Prescale - 1)乗 + 1) x 8ms
	// }
	//-------------------------------------------------------------------------
	// プレスケールの算出
	uint8 prescale;
	for (prescale = 0; WATCHDOG_LIMITS[prescale + 1] < u32TimeOutMsec && prescale < 11; prescale++);
	// WatchdogTimerを停止してからプレケールを指定して再始動
	vAHI_WatchdogStop();
	vAHI_WatchdogStart(prescale);
}

/**
 * Tick Timerを利用したウェイト処理（ミリ秒単位）
 *
 *	@param uint32 usec 待ち時間（ミリ秒単位）
 *	@return uint32 経過マイクロ秒数
 */
PUBLIC uint32 u32TimerUtil_waitTickMSec(uint32 u32msec) {
	// ウェイト処理
	return u32TimerUtil_waitTickUSec(u32msec * 1000);
}

/**
 * Tick Timerを利用したウェイト処理（マイクロ秒単位）
 *
 *	@param uint32 usec 待ち時間（マイクロ秒単位）
 *	@return uint32 経過マイクロ秒数
 */
PUBLIC uint32 u32TimerUtil_waitTickUSec(uint32 u32usec) {
	// 現在Tickを取得
	uint64 u64BeginTick =
		(uint64)u32TickCount_ms * 1000 * TIMER_UTIL_TICK_PER_USEC + (uint64)u32AHI_TickTimerRead();
	// 待ち合わせ時刻を算出
	uint64 u64EndTick = u64BeginTick + u32usec * TIMER_UTIL_TICK_PER_USEC - TIMER_UTIL_TICK_WAIT_DIFF;
	// Tick Timerによる待ち合わせ
	volatile uint64 u64CurrentTick = u64BeginTick;
	while (u64CurrentTick < u64EndTick) {
		u64CurrentTick =
			(uint64)u32TickCount_ms * 1000 * TIMER_UTIL_TICK_PER_USEC + (uint64)u32AHI_TickTimerRead();
	}
	// 経過時間（マイクロ秒）を返却
	return (u64CurrentTick - u64BeginTick + TIMER_UTIL_TICK_WAIT_DIFF) / TIMER_UTIL_TICK_PER_USEC;
}

/**
 * Tick Timerを利用した待ち合わせ処理（指定時間まで）
 *
 *	@param uint32 u32JoinUsec 待ち合わせ時刻（マイクロ秒）
 *	@return uint32 経過時間（マイクロ秒）
 */
PUBLIC uint32 u32TimerUtil_waitUntil(uint64 u64JoinUsec) {
	// ウェイト判定
	uint64 u64BeginUSec = u64TimerUtil_readUsec();
	if (u64JoinUsec <= u64BeginUSec) {
		return 0;
	}
	// 経過時間を返却
	return u32TimerUtil_waitTickUSec(u64JoinUsec - u64BeginUSec);
}

/**
 * TickTimerを利用した現在時刻（マイクロ秒単位）の取得処理
 *
 * @return uint64 経過マイクロ秒数
 */
PUBLIC uint64 u64TimerUtil_readUsec() {
	// 現在時刻取得
	uint32 u32CurrentMsec = u32TickCount_ms;
	// Tick Timerの補正値を取得
	uint32 u32CurrentTick =
		(u32AHI_TickTimerRead() + TIMER_UTIL_TICK_MSEC_DIFF) % TIMER_UTIL_TICK_REFLESH_RATE;
	// 割り込みによるミリ秒のカウントアップ対応
	if (u32CurrentTick < 80) {
		u32CurrentMsec = u32TickCount_ms;
	}
	// 経過ミリ秒を加算
	uint32 u32LastMsec = (uint32)sTimerUtil_info.u64TimerUtil_tickLastMsec;
	sTimerUtil_info.u64TimerUtil_tickLastMsec += (uint64)(u32CurrentMsec - u32LastMsec);
	// 補正されて64bit化されたマイクロ秒を取得
	uint64 u64CurrentUsec = u32CurrentTick / TIMER_UTIL_TICK_PER_USEC;
	return sTimerUtil_info.u64TimerUtil_tickLastMsec * 1000 + u64CurrentUsec;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
