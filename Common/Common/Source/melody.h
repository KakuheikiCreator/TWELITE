/****************************************************************************
 *
 * MODULE :Melody functions header file
 *
 * CREATED:2016/04/09 22:36:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:短いメロディ出力の為の関数群
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
#ifndef  MELODY_H_INCLUDED
#define  MELODY_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "app_main.h"
#include "framework.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/** 音階リストサイズ */
#define MELODY_MAX_SCORE_SIZE      (16)
/** メロディ：運命 */
#define MELODY_DESTINY             (0)
/** メロディ：アイネ・クライネ・ナハトムジーク */
#define MELODY_K525                (1)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// 楽譜情報
typedef struct {
	uint8 u8ListSize;							// リストサイズ
	uint8 u8ScaleList[MELODY_MAX_SCORE_SIZE];	// 音階リスト
	uint8 u8OctaveList[MELODY_MAX_SCORE_SIZE];	// オクターブリスト
	uint8 u8TimeList[MELODY_MAX_SCORE_SIZE];	// 発音時間リスト（50ミリ秒単位）
} tsMelody_Score;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/** 演奏楽譜：運命 */
PUBLIC tsMelody_Score sMelody_ScoreDestiny;
/** 演奏楽譜：アイネ・クライネ・ナハトムジーク */
PUBLIC tsMelody_Score sMelody_ScoreK525;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/** メロディ関数群の初期処理 */
PUBLIC void vMelody_init(teFwkEvent ePlayerEvt, uint8 u8PWMTimer, bool_t bLocFlg, bool_t bLocFlgEx);
/** 演奏依頼処理 */
PUBLIC void vMelody_request(tsMelody_Score *spMelody_tsScore, bool_t bRepeat);
/** メロディ演奏イベントタスク */
PUBLIC void vMelody_play(uint32 u32EvtTimeMs);
/** メロディ演奏停止処理 */
PUBLIC void vMelody_stop();

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* MELODY_UTIL_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

