/****************************************************************************
 *
 * MODULE :I/O Utility functions source file
 *
 * CREATED:2015/05/31 05:32:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:アナログ入力ポートの読み込みユーティリティ関数群
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

// ユーザーライブラリ
#include "io_util.h"
#include "timer_util.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// デジタル入力バッファ
typedef struct {
	uint8  u8LastValIdx;						// 現在インデックス
	uint32 u32DIBuff[IO_UTIL_DI_BUFF_SIZE];		// デジタル入力バッファ
} IOUtil_tsDIBuffer;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
// Digital Input Buffer
PRIVATE IOUtil_tsDIBuffer sDIBuffer;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/*****************************************************************************
 *
 * NAME:bIOUtil_adcInit
 *
 * DESCRIPTION:アナログポート初期処理
 *
 * PARAMETERS:       Name              RW  Usage
 *   uint8           u8ClockDivRatio   R   クロック分割レート
 *
 * RETURNS:
 *   TRUE :初期化成功
 *   FALSE:初期化失敗（既に初期化済み）
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bIOUtil_adcInit(uint8 u8ClockDivRatio) {
	// ADC を開始する
	vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,		// アナログ電源供給機能：有効
					 E_AHI_AP_INT_DISABLE,			// AD変換後の割り込み：割り込み無効
					 E_AHI_AP_SAMPLE_4,				// サンプリング周波数：4クロック毎
					 u8ClockDivRatio,				// クロック分割レート：16MHz / (250KHz(64)～2MHz(8))
					 E_AHI_AP_INTREF);				// 参照電圧：内部
	// 初期化成功
	return TRUE;
}

/*****************************************************************************
 *
 * NAME:vIOUtil_adcUpdateBuffer
 *
 * DESCRIPTION:アナログ入力バッファの更新処理
 *
 * PARAMETERS:       Name           RW  Usage
 * tsAIBuffer        *psAIBuffer    RW  ピン毎のバッファ情報
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vIOUtil_adcUpdateBuffer(IOUtil_tsADCBuffer *psADCBuffer) {
	// アナログ電源供給機能の起動待ち
	uint32 idx;
	for (idx = 0; !bAHI_APRegulatorEnabled(); idx++) {
		// インターバル
		u32TimerUtil_waitTickUSec(20);
	}
	// ADCを有効化
	vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,	// 単発モード
					E_AHI_AP_INPUT_RANGE_2,	// 0-2.4Vまで
					psADCBuffer->u8ChkPin);	// 入力ピン
	// サンプリングを開始
	vAHI_AdcStartSample();
	// AD変換待ち
	while (bAHI_AdcPoll());
	// バッファ情報を更新
	psADCBuffer->u8LastValIdx = (psADCBuffer->u8LastValIdx + 1) % IO_UTIL_AI_BUFF_SIZE;
	psADCBuffer->u16ADCBuff[psADCBuffer->u8LastValIdx] = u16AHI_AdcRead();
	// ADCを無効化
	vAHI_AdcDisable();
}

/*****************************************************************************
 *
 * NAME:iIOUtil_adcReadBuffer
 *
 * DESCRIPTION:アナログピン読み取り
 *      アナログピンを読み込み、その値の範囲に対応するインデックスを返す。
 *      チャタリング対策として複数回読み込み、指定回数同じ値が取得された場合に
 *      その値に対応したインデックス値を返却する。
 *      有効値が無い場合には-1を返却
 *
 * PARAMETERS:      Name            RW  Usage
 *   tsADCBuffer*   psADCBuffer     R   読み取り情報構造体（参照ピンや閾値リスト等）
 *   uint16         u16Tolerance    R   ±許容値
 *   uint8          u8RefCnt        R   最小参照回数
 *
 * RETURNS:
 *   int  参照対象値の平均値、無効値の場合には-1を返却
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC int iIOUtil_adcReadBuffer(IOUtil_tsADCBuffer *psADCBuffer, uint16 u16Tolerance, uint8 u8RefCnt) {
	// 参照回数の上限値チェック
	if (u8RefCnt == 0 || u8RefCnt > IO_UTIL_AI_BUFF_SIZE) {
		return -1;
	}
	// 基準となる閾値インデックス
	uint8 u8BuffIdx = psADCBuffer->u8LastValIdx;
	uint16 u16TargetVal = psADCBuffer->u16ADCBuff[u8BuffIdx];
	int iAddVal = u16TargetVal;
	int iTargetFrom = (int)u16TargetVal - u16Tolerance;
	int iTargetTo = (int)u16TargetVal + u16Tolerance;
	// 許容値の読み飛ばし
	uint8 u8Cnt;
	uint16 u16CheckVal;
	for (u8Cnt = 1; u8Cnt < u8RefCnt; u8Cnt++) {
		u8BuffIdx = ((psADCBuffer->u8LastValIdx + IO_UTIL_AI_BUFF_SIZE) - u8Cnt) % IO_UTIL_AI_BUFF_SIZE;
		u16CheckVal = psADCBuffer->u16ADCBuff[u8BuffIdx];
		iAddVal = iAddVal + u16CheckVal;
		if (u16CheckVal < iTargetFrom || iTargetTo < u16CheckVal) {
			return -1;
		}
	}
	// ターゲットインデックス
	return (int)(iAddVal / u8RefCnt);
}

/*****************************************************************************
 *
 * NAME:vIOUtil_diUpdateBuffer
 *
 * DESCRIPTION:デジタル入力バッファの更新処理
 *
 * PARAMETERS:       Name           RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vIOUtil_diUpdateBuffer() {
	// 現在インデックスの更新
	sDIBuffer.u8LastValIdx = (sDIBuffer.u8LastValIdx + 1) % IO_UTIL_DI_BUFF_SIZE;
	// DIの読み込み
	sDIBuffer.u32DIBuff[sDIBuffer.u8LastValIdx] = u32AHI_DioReadInput();
}

/*****************************************************************************
 *
 * NAME:u32IOUtil_diReadBuffer
 *
 * DESCRIPTION:デジタル入力バッファの読み込み処理
 *
 * PARAMETERS:       Name           RW  Usage
 *   uint32          u32PinMap      R   ピンマップ
 *   uint8           u8RefCnt       R   最小参照回数
 *
 * RETURNS:
 *   uint32  入力値（元の値）、無効値:0x80000000
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint32 u32IOUtil_diReadBuffer(uint32 u32PinMap, uint8 u8RefCnt) {
	// ピンマップの有効性チェック
	if (u32PinMap >= (E_AHI_DIO19_INT << 1)) {
		return 0x80000000;
	}
	// 参照回数の有効性チェック
	if (u8RefCnt > IO_UTIL_DI_BUFF_SIZE) {
		return 0x80000000;
	}
	// チェック対象値
	uint32 u32ChkVal = sDIBuffer.u32DIBuff[sDIBuffer.u8LastValIdx] & u32PinMap;
	// 直近の参照回数確認
	uint8 u8Cnt, u8ChkIdx;
	for (u8Cnt = 1; u8Cnt < u8RefCnt; u8Cnt++) {
		u8ChkIdx = (sDIBuffer.u8LastValIdx + (IO_UTIL_DI_BUFF_SIZE - u8Cnt)) % IO_UTIL_DI_BUFF_SIZE;
		if ((sDIBuffer.u32DIBuff[u8ChkIdx] & u32PinMap) != u32ChkVal) {
			return 0x80000000;
		}
	}
	// 有効値を返却
	return u32ChkVal;
}

/*****************************************************************************
 *
 * NAME:vIOUtil_rotEncoderInit
 *
 * DESCRIPTION:ロータリーエンコーダ初期処理
 *
 * PARAMETERS:       Name           RW  Usage
 * tsRotEncoderInfo* psRotEncInfo   R   ロータリーエンコーダの情報
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vIOUtil_rotEncoderInit(IOUtil_tsRotEncoderInfo *psRotEncInfo) {
	// 状態値の初期化
	psRotEncInfo->iVal = 0;
	// 入力ポートビットマップを編集、1が立っている個所以外は現状維持
	uint32 u32ReadMapA = 0x01 << psRotEncInfo->u32APin;
	uint32 u32ReadMapB = 0x01 << psRotEncInfo->u32BPin;
	uint32 u32ReadMap = u32ReadMapA | u32ReadMapB;
	// A相とB相が割り当てられたDIOピンを入力に設定
	vAHI_DioSetDirection(u32ReadMap, 0);
	if (psRotEncInfo->bPullUpFlg) {
		// A相とB相が割り当てられたDIOピンをプルアップ
		vAHI_DioSetPullup(u32ReadMap, 0);
	} else {
		// A相とB相が割り当てられたDIOピンをプルアップ停止
		vAHI_DioSetPullup(0, u32ReadMap);
	}
	// ピンの初期値設定
	uint32 u32DiVals = u32AHI_DioReadInput();
	psRotEncInfo->bBefAPinVal = (u32DiVals >> psRotEncInfo->u32APin) & TRUE;
}

/*****************************************************************************
 *
 * NAME:bIOUtil_rotEncoderChk
 *
 * DESCRIPTION:ロータリーエンコーダチェック処理
 *
 * PARAMETERS:       Name           RW  Usage
 * tsRotEncoderInfo* psRotEncInfo   R   ロータリーエンコーダの情報
 *
 * RETURNS:
 *      TRUE :チェック成功
 *      FALSE:チェック失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bIOUtil_rotEncoderChk(IOUtil_tsRotEncoderInfo *psRotEncInfo) {
	// 有効値判定
	uint32 u32BitMask = ((0x01 << psRotEncInfo->u32APin) | (0x01 << psRotEncInfo->u32BPin));
	uint32 u32DIVals = u32IOUtil_diReadBuffer(u32BitMask, 2);
	if (u32DIVals == 0x80000000) {
		// AピンBピンの何れかが無効
		return FALSE;
	}
	// A相とB相の値を取得
	bool_t bCurAPinVal = (u32DIVals >> psRotEncInfo->u32APin) & TRUE;
	bool_t bCurBPinVal = (u32DIVals >> psRotEncInfo->u32BPin) & TRUE;
	// A相の値の変化を判定（A相の値が変化したタイミングでチェックをする為）
	if (bCurAPinVal == psRotEncInfo->bBefAPinVal) {
		return FALSE;
	}
	// ロータリーエンコーダー回転方向判定
	if (bCurAPinVal != bCurBPinVal) {
		// 右回転
		psRotEncInfo->iVal += 1;
	} else {
		// 左回転
		psRotEncInfo->iVal -= 1;
	}
	// 直前の値を更新
	psRotEncInfo->bBefAPinVal = bCurAPinVal;
	// 正常終了
    return TRUE;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
