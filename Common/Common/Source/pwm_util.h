/****************************************************************************
 *
 * MODULE :PWM Utility functions header file
 *
 * CREATED:2016/04/09 19:29:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:PWM出力に関するユーティリティ関数群
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
#ifndef  PWM_UTIL_H_INCLUDED
#define  PWM_UTIL_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/** 音階値：無音 */
#define PWM_UTL_SCALE_NONE   (0)
/** 音階値：ド */
#define PWM_UTL_SCALE_C      (1)
/** 音階値：ド＃ */
#define PWM_UTL_SCALE_CS     (2)
/** 音階値：レ */
#define PWM_UTL_SCALE_D      (3)
/** 音階値：レ＃ */
#define PWM_UTL_SCALE_DS     (4)
/** 音階値：ミ */
#define PWM_UTL_SCALE_E      (5)
/** 音階値：ファ */
#define PWM_UTL_SCALE_F      (6)
/** 音階値：ファ＃ */
#define PWM_UTL_SCALE_FS     (7)
/** 音階値：ソ */
#define PWM_UTL_SCALE_G      (8)
/** 音階値：ソ＃ */
#define PWM_UTL_SCALE_GS     (9)
/** 音階値：ラ */
#define PWM_UTL_SCALE_A      (10)
/** 音階値：ラ＃ */
#define PWM_UTL_SCALE_AS     (11)
/** 音階値：シ */
#define PWM_UTL_SCALE_B      (12)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// サーボタイプ
typedef enum {
	E_PWM_UTIL_SERVO_DEFAULT,	// デフォルト
	E_PWM_UTIL_SERVO_FTB_KND,	// Futaba製・近藤科学製
	E_PWM_UTIL_SERVO_SANWA,		// SANWA製
	E_PWM_UTIL_SERVO_JR			// JR製
} tePwmUtil_ServoType;

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
/** PWM初期処理 */
PUBLIC void vPWMUtil_pwmInit(uint8 u8Timer, uint8 u8Prescale, bool_t bLocFlg, bool_t bLocFlgEx);

//==============================================================================
// サーボモーター関係の関数定義
//==============================================================================
/** サーボモーターの選択処理 */
PUBLIC void vPWMUtil_servoSelect(uint8 u8Timer, tePwmUtil_ServoType sType, bool_t bLocFlg, bool_t bLocFlgEx);
/** サーボモーターへの角度設定処理 */
PUBLIC bool_t bPWMUtil_servoAngle(uint8 u8Angle);
/** サーボモーターへの角度設定処理（秒単位） */
PUBLIC bool_t bPWMUtil_servoAngleSec(uint32 u32AngleSec);

//==============================================================================
// 音階出力の為の関数定義
//==============================================================================
/** PWM機能を利用した音階出力機能の初期処理 */
PUBLIC void vPWMUtil_scaleInit(uint8 u8Timer, bool_t bLocFlg, bool_t bLocFlgEx);
/** 音階とオクターブ数指定による音声出力処理 */
PUBLIC bool_t bPWMUtil_scaleOutput(uint8 u8Timer, uint8 u8Scale, uint8 u8Octave);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* PWM_UTIL_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

