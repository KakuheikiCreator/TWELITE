/****************************************************************************
 *
 * MODULE :Debug functions header file
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
 *******************************************************************************
 * Copyright (c) 2015, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 ****************************************************************************/

#ifndef  DEBUG_CHK_H_INCLUDED
#define  DEBUG_CHK_H_INCLUDED

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

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
// デバッグ主処理
PUBLIC void vDEBUG_main();
// デバッグ初期処理
PUBLIC void vDEBUG_init();
// アナログポートデバッグ
PUBLIC void vAnalogue_debug();
// ランダム関数デバッグ
PUBLIC void vRandom_debug();
// Timer Utilデバッグ
PUBLIC void vTimerUtil_debug();
// LCDデバッグ
PUBLIC void vACM1602NI_debug();
// LCDデバッグ
PUBLIC void vST7032I_debug();
// RTCモジュールデバッグ
PUBLIC void vDS3231_debug();
// SHA256デバッグ
PUBLIC void vSHA256_debug();
// デバッグメッセージ表示処理
PUBLIC void vDEBUG_dispMsg(const char* fmt);


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* DEBUG_CHK_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
