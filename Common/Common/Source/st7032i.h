/****************************************************************************
 *
 * MODULE :ST7032I LCD Driver functions header file
 *
 * CREATED:2015/05/30 10:06:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:ST7032I LCD I2C draiver
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
#ifndef  ST7032i_H_INCLUDED
#define  ST7032i_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// Icon data Size
#define ST7032i_ICON_DATA_SIZE       (16)
// Address Read/Write
#define I2C_ADDR_ST7032I             (0x3E)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/**
 * 構造体：LCDの状態情報
 */
typedef struct {
	// I2Cアドレス
	uint8 u8Address;
	// コントラスト
	uint8 u8Contrast;
	// アイコン表示
	bool_t bIconDispFlg;
	// アイコンデータ
	uint8 u8IconData[ST7032i_ICON_DATA_SIZE];
	// 次回コマンド実行可能時刻
	uint32 u64nextExec;
} ST7032i_state;

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
// デバイス選択
PUBLIC bool_t bST7032i_deviceSelect(ST7032i_state* psState);
// 初期化
PUBLIC bool_t bST7032i_init();
// 画面制御設定（ディスプレイON/OFF、カーソル表示、カーソル点滅、アイコン表示）
PUBLIC bool_t bST7032i_dispControl(bool_t bDispFlg, bool_t bCursorFlg, bool_t bBlinkFlg, bool_t bIconFlg);
// コントラスト設定
PUBLIC bool_t bST7032i_setContrast(uint8 u8Contrast);
// スクリーンクリア
PUBLIC bool_t bST7032i_clearScreen();
// アイコンクリア
PUBLIC bool_t bST7032i_clearICON();
// カーソル移動（行、列）
PUBLIC bool_t bST7032i_setCursor(uint8 u8RowNo, uint8 u8ColNo);
// カーソル移動（ホームへ）
PUBLIC bool_t bST7032i_returnHome();
// カーソル移動（左）
PUBLIC bool_t bST7032i_cursorShiftL();
// カーソル移動（右）
PUBLIC bool_t bST7032i_cursorShiftR();
// 外字登録
PUBLIC bool_t bST7032i_charRegist(uint8 cCh, uint8 *pu8CgDataList);
// 文字表示
PUBLIC bool_t bST7032i_writeChar(char cCh);
// 文字列表示
PUBLIC bool_t bST7032i_writeString(char* pcStr);
// アイコン表示制御
PUBLIC bool_t bST7032i_writeIcon(uint8 u8IconNo);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* ST7032i_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

