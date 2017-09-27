/****************************************************************************
 *
 * MODULE :PWM Utility functions source file
 *
 * CREATED:2016/04/09 02:37:00
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

#include "pwm_util.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
//==============================================================================
// 音階出力機能関係のリテラル定義
//==============================================================================
// クロックレート（１０００倍値）
#define PWM_UTIL_AUDIO_RATE        (1000000000)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// 構造体：サーボタイプ
typedef struct {
	// 利用タイマー
	uint8 u8Timer;
	// サーボモーター制御パルス周期
	uint16 u16PulseCycle;
	// サーボモーター制御パルス（最小）
	uint16 u16PulseMin;
	// サーボモーター制御パルス（最大）
	uint16 u16PulseMax;
} tsServoType;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/** 音階周波数リスト（１オクターブの各音階の1000倍の周波数値） */
PRIVATE const uint32 SCALE_FREQUENCY_LIST[13] =
	{0, 65405, 69295, 73415, 77780, 82405, 87305, 92495, 97995, 103825, 110000, 116540, 123470};
/** サーボ情報 */
PRIVATE tsServoType sServoType;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/*****************************************************************************
 *
 * NAME:vPWMUtil_pwmInit
 *
 * DESCRIPTION:PWM初期処理
 *
 * PARAMETERS:       Name           RW  Usage
 *   uint8           u8Timer        R   利用タイマー
 *   uint8           u8Prescale     R   クロック周波数の分割数（周波数=16MHz / (2 ^ u8Prescale)）
 *   bool_t          bPinLocFlg     R   ピン再配置フラグ（タイマー１～４で共通）
 *   bool_t          bPinLocFlgEx   R   ピン再配置フラグ（タイマー１～４で共通、タイマー２・３用）
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vPWMUtil_pwmInit(uint8 u8Timer, uint8 u8Prescale, bool_t bLocFlg, bool_t bLocFlgEx) {
	// DO0とDO1の有効化判定
	if (bLocFlgEx) {
		bAHI_DoEnableOutputs(bLocFlgEx);
	}
	// タイマーを有効化（割り込み無効、PWM出力）
	vAHI_TimerEnable(u8Timer, u8Prescale, FALSE, FALSE, TRUE);
	// クロックソース設定（内部クロック16MHz）
	vAHI_TimerClockSelect(u8Timer, FALSE, TRUE);
	// タイマー出力設定（内部クロック16MHz）
	vAHI_TimerConfigureOutputs(u8Timer, FALSE, TRUE);
	// 出力ピン設定
	vAHI_TimerSetLocation(u8Timer, bLocFlg, bLocFlgEx);
	// タイマー出力ピン有効化
	vAHI_TimerDIOControl(u8Timer, TRUE);
}

/*****************************************************************************
 *
 * NAME:vPWMUtil_servoSelect
 *
 * DESCRIPTION:サーボモーターの初期化処理
 *
 * PARAMETERS:           Name           RW  Usage
 *   uint8               u8Timer        R   PWM利用タイマー
 *   tePwmUtil_ServoType sType          R   サーボタイプ
 *   bool_t              bPinLocFlg     R   ピン再配置フラグ
 *   bool_t              bPinLocFlgEx   R   ピン再配置フラグ（タイマー２・３用）
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vPWMUtil_servoSelect(uint8 u8Timer, tePwmUtil_ServoType sType, bool_t bLocFlg, bool_t bLocFlgEx) {
	// タイマー設定
	sServoType.u8Timer = u8Timer;
	// サーボタイプ判定
	switch (sType) {
	case E_PWM_UTIL_SERVO_DEFAULT:
		sServoType.u16PulseCycle = 20000;
		sServoType.u16PulseMin   = 500;
		sServoType.u16PulseMax   = 2400;
		break;
	case E_PWM_UTIL_SERVO_FTB_KND:
		sServoType.u16PulseCycle = 15000;
		sServoType.u16PulseMin   = 1020;
		sServoType.u16PulseMax   = 2020;
		break;
	case E_PWM_UTIL_SERVO_SANWA:
		sServoType.u16PulseCycle = 20000;
		sServoType.u16PulseMin   = 900;
		sServoType.u16PulseMax   = 2100;
		break;
	case E_PWM_UTIL_SERVO_JR:
		sServoType.u16PulseCycle = 20000;
		sServoType.u16PulseMin   = 1000;
		sServoType.u16PulseMax   = 2000;
		break;
	}
	// PWMを初期化、1MHz
	vPWMUtil_pwmInit(u8Timer, 4, bLocFlg, bLocFlgEx);
}

/*****************************************************************************
 *
 * NAME:bPWMUtil_servoAngle
 *
 * DESCRIPTION:サーボモーターへの角度設定
 *
 * PARAMETERS:       Name           RW  Usage
 *   uint8           u8Angle        R   設定角度
 *
 * RETURNS:
 *   TRUE  : 設定完了
 *   FALSE : 角度誤り
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bPWMUtil_servoAngle(uint8 u8Angle) {
	// パルス幅の入力チェック
	if (u8Angle > 180) {
		return FALSE;
	}
	// パルス幅に変換
	uint32 u32PulseMaxWidth = sServoType.u16PulseMax - sServoType.u16PulseMin;
	uint16 u16PulseWidth = sServoType.u16PulseCycle - sServoType.u16PulseMin
						 - (((uint32)u8Angle * u32PulseMaxWidth) / 180);
	// サーボモーター角度設定
	vAHI_TimerStartRepeat(sServoType.u8Timer, u16PulseWidth, sServoType.u16PulseCycle);
	return TRUE;
}

/*****************************************************************************
 *
 * NAME:bPWMUtil_servoAngleSec
 *
 * DESCRIPTION:サーボモーターへの角度設定処理（秒単位）
 *
 * PARAMETERS:       Name           RW  Usage
 *   uint32          u32AngleSec    R   設定角度（秒:0～648000程度）
 *
 * RETURNS:
 *   TRUE  : 設定完了
 *   FALSE : 角度誤り
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bPWMUtil_servoAngleSec(uint32 u32AngleSec) {
	// パルス幅の入力チェック
	if (u32AngleSec > 648000) {
		return FALSE;
	}
	// パルス幅に変換
	uint32 u32PulseMaxWidth = sServoType.u16PulseMax - sServoType.u16PulseMin;
	uint16 u16PulseWidth = sServoType.u16PulseCycle - sServoType.u16PulseMin
						 - ((u32PulseMaxWidth * u32AngleSec) / 648000);
	vAHI_TimerStartRepeat(sServoType.u8Timer, u16PulseWidth, sServoType.u16PulseCycle);
	// 設定完了
	return TRUE;
}

/*****************************************************************************
 *
 * NAME:vPWMUtil_audioInit
 *
 * DESCRIPTION:PWM機能を利用したオーディオ機能の初期処理
 *
 * PARAMETERS:       Name           RW  Usage
 *   uint8           u8Timer        R   利用タイマー
 *   bool_t          bPinLocFlg     R   ピン再配置フラグ
 *   bool_t          bPinLocFlgEx   R   ピン再配置フラグ（タイマー２・３用）
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vPWMUtil_scaleInit(uint8 u8Timer, bool_t bLocFlg, bool_t bLocFlgEx) {
	// PWMを初期化、16MHz
	vPWMUtil_pwmInit(u8Timer, 4, bLocFlg, bLocFlgEx);
}

/*****************************************************************************
 *
 * NAME:bPWMUtil_scaleOutput
 *
 * DESCRIPTION:音階とオクターブ数指定による音声出力処理、PWMの機能を利用
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint8          u8Timer         R   利用タイマー
 *   uint8          u8Scale         R   音階値
 *   uint8          u8Octave        R   オクターブ値（1～）
 *
 * RETURNS:
 *   TRUE  : 出力成功
 *   FALSE : パラメータ誤り
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bPWMUtil_scaleOutput(uint8 u8Timer, uint8 u8Scale, uint8 u8Octave) {
	// 音階値チェック
	if (u8Scale > 11) {
		return FALSE;
	}
	// オクターブ値チェック
	if (u8Octave == 0 || u8Octave > 6) {
		return FALSE;
	}
	// 無音判定
	if (u8Scale == PWM_UTL_SCALE_NONE) {
		// 定常波出力（0Hz）
		vAHI_TimerStartRepeat(u8Timer, 0, 0);
		return TRUE;
	}
	// オクターブ毎の乗数を算出
	uint8 u8multiplier = 1;
	uint8 u8cnt;
	for (u8cnt = 1; u8cnt < u8Octave; u8cnt++) {
		u8multiplier *= 2;
	}
	// 周波数を算出
	uint32 u32Frequency = SCALE_FREQUENCY_LIST[u8Scale] * u8multiplier;
	// 無音判定
	uint16 u16PulseRate = (uint16)(PWM_UTIL_AUDIO_RATE / u32Frequency);
	// 定常波出力
	vAHI_TimerStartRepeat(u8Timer, u16PulseRate / 2, u16PulseRate);
	// 出力成功
	return TRUE;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
