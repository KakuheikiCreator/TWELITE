/****************************************************************************
 *
 * MODULE :I/O Utility functions header file
 *
 * CREATED:2015/05/31 05:32:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:入力ポートの読み込みユーティリティ関数群
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
#ifndef  IO_UTIL_H_INCLUDED
#define  IO_UTIL_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/** Analog Input Buffer Size */
#ifndef IO_UTIL_AI_BUFF_SIZE
	#define IO_UTIL_AI_BUFF_SIZE       (10)
#endif
/** Digital Input Buffer Size */
#ifndef IO_UTIL_DI_BUFF_SIZE
	#define IO_UTIL_DI_BUFF_SIZE       (10)
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// アナログ入力バッファ
typedef struct {
	uint8  u8ChkPin;			// 参照ピン
	uint8  u8LastValIdx;		// 最終インデックス
	uint16 u16ADCBuff[IO_UTIL_DI_BUFF_SIZE];	// アナログ入力バッファ
} IOUtil_tsADCBuffer;

// ロータリーエンコーダーの状態情報
typedef struct {
	volatile int iVal;		// 値（初期値０、右へ１クリック：＋１）
	bool_t bPullUpFlg;		// ピンのプルアップの有無
	uint32 u32APin;			// A相のピン
	uint32 u32BPin;			// B相のピン
	bool_t bBefAPinVal;		// 直前のA相の値
} IOUtil_tsRotEncoderInfo;

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
//==============================================================================
// ADC関係の関数定義
//==============================================================================
// ADC初期処理
PUBLIC bool_t bIOUtil_adcInit(uint8 u8ClockDivRatio);
// アナログ入力バッファの更新処理
PUBLIC void vIOUtil_adcUpdateBuffer(IOUtil_tsADCBuffer *psAIBuffer);
// アナログ入力バッファ読み取り（チャタリング対策）
PUBLIC int iIOUtil_adcReadBuffer(IOUtil_tsADCBuffer *psADCBuffer, uint16 u16Tolerance, uint8 u8RefCnt);

//==============================================================================
// デジタル入出力関係の関数定義
//==============================================================================
// デジタル入力バッファの更新処理
PUBLIC void vIOUtil_diUpdateBuffer();
// デジタル入力バッファ読み取り（チャタリング対策）
PUBLIC uint32 u32IOUtil_diReadBuffer(uint32 u32PinMap, uint8 u8RefCnt);

//==============================================================================
// ロータリーエンコーダー関係の関数定義
//==============================================================================
// ロータリーエンコーダ初期処理
PUBLIC void vIOUtil_rotEncoderInit(IOUtil_tsRotEncoderInfo *psRotEncInfo);
// ロータリーエンコーダチェック処理
PUBLIC bool_t bIOUtil_rotEncoderChk(IOUtil_tsRotEncoderInfo *psRotEncInfo);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
#if defined __cplusplus
}
#endif

#endif  /* IO_UTIL_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

