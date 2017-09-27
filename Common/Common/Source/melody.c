/****************************************************************************
 *
 * MODULE :Melody functions source file
 *
 * CREATED:2016/04/09 23:35:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:音声に関する基本的なユーティリティ関数群
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
#include <AppHardwareApi.h>

/****************************************************************************/
/***        ToCoNet Include files                                         ***/
/****************************************************************************/
#include "ToCoNet.h"

/****************************************************************************/
/***        User Include files                                            ***/
/****************************************************************************/
#include "melody.h"
#include "pwm_util.h"
#include "timer_util.h"
#include "framework.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/** プレイヤー実行オフセット */
#ifndef MELODY_PLAYER_EXEC_OFFSET
	#define MELODY_PLAYER_EXEC_OFFSET  (30)
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// プレイヤー状態
typedef struct {
	teFwkEvent ePlayerEvt;		// プレイヤータスクイベント
	int    iPlayerEvtIdx;		// プレイヤーイベントインデックス
	uint8  u8Timer;				// PWM利用タイマー
	uint32 u32NextExec;			// 次回実行時刻
	tsMelody_Score *spScore;	// 演奏楽曲
	uint8  u8ScoreIdx;			// スコアインデックス
	bool_t bRepeatFlg;			// 演奏反復フラグ
} tsMelody_PlayerInfo;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
/** メロディ演奏タスク情報初期化 */
PRIVATE void vMelody_playerInit(teFwkEvent ePlayerEvt, uint8 u8Timer);
/** 演奏楽譜の初期化処理（運命） */
PRIVATE void vMelody_initDestiny();
/** 演奏楽譜の初期化処理（アイネ・クライネ・ナハトムジーク） */
PRIVATE void vMelody_initK525();

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/** プレイヤー情報 */
PRIVATE tsMelody_PlayerInfo sMelody_PlayerInfo;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/*****************************************************************************
 *
 * NAME:vMelody_init
 *
 * DESCRIPTION:メロディ関数群の初期処理
 *
 * PARAMETERS:       Name           RW  Usage
 *   teFwkEvent      ePlayerEvt     R   プレイヤー処理タスクのイベント
 *   uint8           u8Timer        R   利用タイマー
 *   bool_t          bPinLocFlg     R   ピン再配置フラグ
 *   bool_t          bPinLocFlgEx   R   ピン再配置フラグ（タイマー２・３用）
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vMelody_init(teFwkEvent ePlayerEvt, uint8 u8Timer, bool_t bLocFlg, bool_t bLocFlgEx) {
	//=========================================================================
	// PWM機能の初期化
	//=========================================================================
	// PWMを初期化、16MHz
	vPWMUtil_scaleInit(u8Timer, bLocFlg, bLocFlgEx);
	//=========================================================================
	// 演奏タスクの初期化
	//=========================================================================
	vMelody_playerInit(ePlayerEvt, u8Timer);
	//=========================================================================
	// サンプル楽曲の初期化
	//=========================================================================
	// 楽曲初期化：運命
	vMelody_initDestiny();
	// 楽曲初期化：アイネ・クライネ・ナハトムジーク
	vMelody_initK525();
}

/*****************************************************************************
 *
 * NAME:vMelody_request
 *
 * DESCRIPTION:演奏依頼処理
 *
 * PARAMETERS:       Name           RW  Usage
 *   uint8           u8MelodyNo     R   楽譜ナンバー
 *   bool_t          bRepeatFlg     R   反復演奏フラグ
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vMelody_request(tsMelody_Score *spMelody_Score, bool_t bRepeatFlg) {
	// 既存の演奏タスクをクリア
	if (sMelody_PlayerInfo.iPlayerEvtIdx >= 0) {
		vMelody_stop();
	}
	// 楽曲の情報を設定
	sMelody_PlayerInfo.spScore = spMelody_Score;
	sMelody_PlayerInfo.u8ScoreIdx = 0;
	// 反復演奏フラグを設定
	sMelody_PlayerInfo.bRepeatFlg = bRepeatFlg;
	// 演奏時刻を設定
	sMelody_PlayerInfo.u32NextExec = u32TickCount_ms + 100;
	// 演奏タスク登録
	bRegisterEvtTask(sMelody_PlayerInfo.ePlayerEvt, vMelody_play);
	// 演奏イベント登録
	sMelody_PlayerInfo.iPlayerEvtIdx =
		iEntryScheduleEvt(sMelody_PlayerInfo.ePlayerEvt, 50, MELODY_PLAYER_EXEC_OFFSET, TRUE);
}

/*****************************************************************************
 *
 * NAME:vMelody_play
 *
 * DESCRIPTION:メロディ演奏イベントタスク
 *
 * PARAMETERS:       Name           RW  Usage
 *   uint32          u32EvtTimeMs   R   イベント情報
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vMelody_play(uint32 u32EvtTimeMs) {
	// 楽譜がセットされていない場合には、演奏タスクをクリアして終了
	if (sMelody_PlayerInfo.spScore == NULL) {
		// 演奏停止
		vMelody_stop();
		return;
	}
	// 発音開始判定
	if (sMelody_PlayerInfo.u32NextExec > u32TickCount_ms) {
		return;
	}
	// 演奏終了判定
	tsMelody_Score *sScore = sMelody_PlayerInfo.spScore;
	if (sMelody_PlayerInfo.u8ScoreIdx >= sScore->u8ListSize) {
		if (!sMelody_PlayerInfo.bRepeatFlg) {
			// 演奏停止
			vMelody_stop();
			return;
		}
		sMelody_PlayerInfo.u8ScoreIdx = 0;
	}
	// 発音処理
	uint8 u8Timer  = sMelody_PlayerInfo.u8Timer;
	uint8 u8Scale  = sScore->u8ScaleList[sMelody_PlayerInfo.u8ScoreIdx];
	uint8 u8Octave = sScore->u8OctaveList[sMelody_PlayerInfo.u8ScoreIdx];
	bPWMUtil_scaleOutput(u8Timer, u8Scale, u8Octave);
	// 次回発音開始時刻を更新
	sMelody_PlayerInfo.u32NextExec =
		u32TickCount_ms + sScore->u8TimeList[sMelody_PlayerInfo.u8ScoreIdx] * 50;
	// 音階インデックスを更新
	sMelody_PlayerInfo.u8ScoreIdx++;
}

/*****************************************************************************
 *
 * NAME:vMelody_stop
 *
 * DESCRIPTION:演奏終了処理
 *
 * PARAMETERS:       Name           RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vMelody_stop() {
	// イベントタスク有無判定
	if (sMelody_PlayerInfo.iPlayerEvtIdx < 0) {
		return;
	}
	// イベントタスククリア
	bCancelScheduleEvt(sMelody_PlayerInfo.iPlayerEvtIdx);
	// タイマー情報の初期化
	vMelody_playerInit(sMelody_PlayerInfo.ePlayerEvt, 9);
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/*****************************************************************************
 *
 * NAME:vMelody_playerInit
 *
 * DESCRIPTION:メロディ演奏タスク情報初期化
 *
 * PARAMETERS:       Name           RW  Usage
 *   teFwkEvent      ePlayerEvt     R   プレイヤー処理タスクのイベント
 *   uint8           u8Timer        R   利用タイマー
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE void vMelody_playerInit(teFwkEvent ePlayerEvt, uint8 u8Timer) {
	//=========================================================================
	// 演奏タスクの初期化
	//=========================================================================
	// 演奏タスク
	sMelody_PlayerInfo.ePlayerEvt  = ePlayerEvt;
	// 演奏タスクイベント
	sMelody_PlayerInfo.iPlayerEvtIdx = -1;
	// 利用タイマー
	sMelody_PlayerInfo.u8Timer     = u8Timer;
	// 次回実行時刻
	sMelody_PlayerInfo.u32NextExec = 0;
	// 楽譜
	sMelody_PlayerInfo.spScore     = NULL;
	// 音階インデックス
	sMelody_PlayerInfo.u8ScoreIdx  = 0;
	// 演奏反復フラグ
	sMelody_PlayerInfo.bRepeatFlg  = FALSE;
}

/*****************************************************************************
 *
 * NAME:vMelody_initDestiny
 *
 * DESCRIPTION:演奏楽譜の初期化処理（運命）
 *
 * PARAMETERS:       Name           RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE void vMelody_initDestiny() {
	// 楽譜サイズ
	sMelody_ScoreDestiny.u8ListSize = 16;
	// 音階リスト設定
	sMelody_ScoreDestiny.u8ScaleList[0]  = PWM_UTL_SCALE_G;
	sMelody_ScoreDestiny.u8ScaleList[1]  = PWM_UTL_SCALE_NONE;
	sMelody_ScoreDestiny.u8ScaleList[2]  = PWM_UTL_SCALE_G;
	sMelody_ScoreDestiny.u8ScaleList[3]  = PWM_UTL_SCALE_NONE;
	sMelody_ScoreDestiny.u8ScaleList[4]  = PWM_UTL_SCALE_G;
	sMelody_ScoreDestiny.u8ScaleList[5]  = PWM_UTL_SCALE_NONE;
	sMelody_ScoreDestiny.u8ScaleList[6]  = PWM_UTL_SCALE_DS;
	sMelody_ScoreDestiny.u8ScaleList[7]  = PWM_UTL_SCALE_NONE;
	sMelody_ScoreDestiny.u8ScaleList[8]  = PWM_UTL_SCALE_F;
	sMelody_ScoreDestiny.u8ScaleList[9]  = PWM_UTL_SCALE_NONE;
	sMelody_ScoreDestiny.u8ScaleList[10] = PWM_UTL_SCALE_F;
	sMelody_ScoreDestiny.u8ScaleList[11] = PWM_UTL_SCALE_NONE;
	sMelody_ScoreDestiny.u8ScaleList[12] = PWM_UTL_SCALE_F;
	sMelody_ScoreDestiny.u8ScaleList[13] = PWM_UTL_SCALE_NONE;
	sMelody_ScoreDestiny.u8ScaleList[14] = PWM_UTL_SCALE_D;
	sMelody_ScoreDestiny.u8ScaleList[15] = PWM_UTL_SCALE_NONE;
	// オクターブリスト設定
	sMelody_ScoreDestiny.u8OctaveList[0]  = 4;
	sMelody_ScoreDestiny.u8OctaveList[1]  = 4;
	sMelody_ScoreDestiny.u8OctaveList[2]  = 4;
	sMelody_ScoreDestiny.u8OctaveList[3]  = 4;
	sMelody_ScoreDestiny.u8OctaveList[4]  = 4;
	sMelody_ScoreDestiny.u8OctaveList[5]  = 4;
	sMelody_ScoreDestiny.u8OctaveList[6]  = 4;
	sMelody_ScoreDestiny.u8OctaveList[7]  = 4;
	sMelody_ScoreDestiny.u8OctaveList[8]  = 4;
	sMelody_ScoreDestiny.u8OctaveList[9]  = 4;
	sMelody_ScoreDestiny.u8OctaveList[10] = 4;
	sMelody_ScoreDestiny.u8OctaveList[11] = 4;
	sMelody_ScoreDestiny.u8OctaveList[12] = 4;
	sMelody_ScoreDestiny.u8OctaveList[13] = 4;
	sMelody_ScoreDestiny.u8OctaveList[14] = 4;
	sMelody_ScoreDestiny.u8OctaveList[15] = 4;
	// 発音時間設定
	sMelody_ScoreDestiny.u8TimeList[0]  = 2;
	sMelody_ScoreDestiny.u8TimeList[1]  = 1;
	sMelody_ScoreDestiny.u8TimeList[2]  = 2;
	sMelody_ScoreDestiny.u8TimeList[3]  = 1;
	sMelody_ScoreDestiny.u8TimeList[4]  = 2;
	sMelody_ScoreDestiny.u8TimeList[5]  = 1;
	sMelody_ScoreDestiny.u8TimeList[6]  = 12;
	sMelody_ScoreDestiny.u8TimeList[7]  = 10;
	sMelody_ScoreDestiny.u8TimeList[8]  = 2;
	sMelody_ScoreDestiny.u8TimeList[9]  = 1;
	sMelody_ScoreDestiny.u8TimeList[10] = 2;
	sMelody_ScoreDestiny.u8TimeList[11] = 1;
	sMelody_ScoreDestiny.u8TimeList[12] = 2;
	sMelody_ScoreDestiny.u8TimeList[13] = 1;
	sMelody_ScoreDestiny.u8TimeList[14] = 12;
	sMelody_ScoreDestiny.u8TimeList[15] = 10;
}

/*****************************************************************************
 *
 * NAME:vMelody_initDestiny
 *
 * DESCRIPTION:演奏楽譜の初期化処理（アイネ・クライネ・ナハトムジーク）
 *
 * PARAMETERS:       Name           RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE void vMelody_initK525() {
	// 楽譜サイズ
	sMelody_ScoreK525.u8ListSize = 12;
	// 音階リスト設定
	sMelody_ScoreK525.u8ScaleList[0]  = PWM_UTL_SCALE_C;
	sMelody_ScoreK525.u8ScaleList[1]  = PWM_UTL_SCALE_G;
	sMelody_ScoreK525.u8ScaleList[2]  = PWM_UTL_SCALE_NONE;
	sMelody_ScoreK525.u8ScaleList[3]  = PWM_UTL_SCALE_C;
	sMelody_ScoreK525.u8ScaleList[4]  = PWM_UTL_SCALE_G;
	sMelody_ScoreK525.u8ScaleList[5]  = PWM_UTL_SCALE_NONE;
	sMelody_ScoreK525.u8ScaleList[6]  = PWM_UTL_SCALE_C;
	sMelody_ScoreK525.u8ScaleList[7]  = PWM_UTL_SCALE_G;
	sMelody_ScoreK525.u8ScaleList[8]  = PWM_UTL_SCALE_C;
	sMelody_ScoreK525.u8ScaleList[9]  = PWM_UTL_SCALE_E;
	sMelody_ScoreK525.u8ScaleList[10] = PWM_UTL_SCALE_G;
	sMelody_ScoreK525.u8ScaleList[11] = PWM_UTL_SCALE_NONE;
	// オクターブリスト設定
	sMelody_ScoreK525.u8OctaveList[0]  = 4;
	sMelody_ScoreK525.u8OctaveList[1]  = 3;
	sMelody_ScoreK525.u8OctaveList[2]  = 3;
	sMelody_ScoreK525.u8OctaveList[3]  = 4;
	sMelody_ScoreK525.u8OctaveList[4]  = 3;
	sMelody_ScoreK525.u8OctaveList[5]  = 3;
	sMelody_ScoreK525.u8OctaveList[6]  = 4;
	sMelody_ScoreK525.u8OctaveList[7]  = 3;
	sMelody_ScoreK525.u8OctaveList[8]  = 4;
	sMelody_ScoreK525.u8OctaveList[9]  = 4;
	sMelody_ScoreK525.u8OctaveList[10] = 4;
	sMelody_ScoreK525.u8OctaveList[11] = 4;
	// 発音時間設定
	sMelody_ScoreK525.u8TimeList[0]  = 6;
	sMelody_ScoreK525.u8TimeList[1]  = 3;
	sMelody_ScoreK525.u8TimeList[2]  = 3;
	sMelody_ScoreK525.u8TimeList[3]  = 6;
	sMelody_ScoreK525.u8TimeList[4]  = 3;
	sMelody_ScoreK525.u8TimeList[5]  = 3;
	sMelody_ScoreK525.u8TimeList[6]  = 3;
	sMelody_ScoreK525.u8TimeList[7]  = 3;
	sMelody_ScoreK525.u8TimeList[8]  = 3;
	sMelody_ScoreK525.u8TimeList[9]  = 3;
	sMelody_ScoreK525.u8TimeList[10] = 6;
	sMelody_ScoreK525.u8TimeList[11] = 9;
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
