/****************************************************************************
 *
 * MODULE :ST7032I LCD Driver functions source file
 *
 * CREATED:2015/05/30 10:07:00
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
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <string.h>

#include <jendefs.h>
#include <AppHardwareApi.h>

#include "st7032i.h"
#include "i2c_util.h"
#include "timer_util.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// NULL定義
#ifndef NULL
#define NULL (0)
#endif

// Control byte
#define ST7032i_RS_CMD               (0x00)
#define ST7032i_RS_DATA              (0x40)
// Instruction commands
#define ST7032i_CMD_CLEAR_DISP       (0x01)
#define ST7032i_CMD_RETURN_HOME      (0x02)
#define ST7032i_CMD_ENTRY_MODE_DEF   (0x06)
#define ST7032i_CMD_DISP_CNTR_DEF    (0x08)
#define ST7032i_CMD_CURSOR_SHIFT_L   (0x10)
#define ST7032i_CMD_CURSOR_SHIFT_R   (0x14)
#define ST7032i_CMD_OSC_FREQ         (0x14)
#define ST7032i_CMD_DISP_SHIFT_L     (0x18)
#define ST7032i_CMD_DISP_SHIFT_R     (0x1C)
#define ST7032i_CMD_FUNC_SET_DEF     (0x38)
#define ST7032i_CMD_FUNC_SET_EX      (0x39)
#define ST7032i_CMD_SET_CG_ADDR      (0x40)
#define ST7032i_CMD_SET_DD_ADDR      (0x80)
#define ST7032i_CMD_SET_ICON_ADDR    (0x40)
#define ST7032i_CMD_DISP_CNTR_EX     (0x50)
#define ST7032i_CMD_FOLLOWER_CNTR    (0x6C)
#define ST7032i_CMD_CONTRAST_LOW     (0x70)
#define ST7032i_CMD_DATA_WRITE       (0x80)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
// LCD状態
PRIVATE ST7032i_state* stST7032i_sts;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
// コマンド OR データの送信開始
PRIVATE bool_t bST7032i_writeStart();
// コマンドの送信（継続コマンド有り）
PRIVATE bool_t bST7032i_writeCmd(uint8 u8Cmd);
// コマンドの送信（継続コマンド無し）
PRIVATE bool_t bST7032i_writeCmdStop(uint8 u8Cmd);
// データの送信
PRIVATE bool_t bST7032i_writeData(uint8 u8Data);
// アイコンの書き込み
PRIVATE bool_t bST7032i_writeIconData(uint8 u8Addr);
// 画面制御設定（アイコン表示、コントラスト）
PRIVATE bool_t bST7032i_writeControlEx();
// 次回コマンド実行可能時刻（usec）取得
PRIVATE uint32 u64ST7032i_nextExecTime(uint8 u8Cmd);
// エラー終了
PRIVATE bool_t bST7032i_errorEnd();

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/*****************************************************************************
 *
 * NAME: bST7032i_deviceSelect
 *
 * DESCRIPTION:利用するデバイスの選択を行う
 *
 * PARAMETERS:      Name            RW  Usage
 * ST7032i_state*   psState         R   選択するデバイス情報の構造体
 *
 * RETURNS:
 *      TRUE :選択成功
 *      FALSE:選択失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_deviceSelect(ST7032i_state* psState) {
	// 入力チェック
	if (psState == NULL) return FALSE;
	// ステータス設定を行う
	stST7032i_sts = psState;
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bST7032i_init
 *
 * DESCRIPTION:LCDの初期化処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      TRUE :初期化成功
 *      FALSE:初期化失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_init() {
	//=========================================================================
	// Initialize Sequence
	//=========================================================================
	// LCD状態の初期化
	stST7032i_sts->u8Contrast = 40;
	stST7032i_sts->bIconDispFlg = TRUE;
	memset(stST7032i_sts->u8IconData, 0, ST7032i_ICON_DATA_SIZE);
	// コマンド OR データの送信開始
	if (!bST7032i_writeStart()) return FALSE;
	// Function Set Default(IS=1)
	if (!bST7032i_writeCmd(ST7032i_CMD_FUNC_SET_DEF | 0x01)) return FALSE;
	// Internal OSC frequency
	if (!bST7032i_writeCmd(ST7032i_CMD_OSC_FREQ)) return FALSE;
	// Display Contrast Lower set
	if (!bST7032i_writeCmd(ST7032i_CMD_CONTRAST_LOW | 0x08)) return FALSE;
	// Power/ICON Control/Contrast Higher set
	if (!bST7032i_writeCmd(ST7032i_CMD_DISP_CNTR_EX | 0x0E)) return FALSE;
	// Follower Control
	if (!bST7032i_writeCmd(ST7032i_CMD_FOLLOWER_CNTR)) return FALSE;
	// Function Set Default(IS=0)
	if (!bST7032i_writeCmd(ST7032i_CMD_FUNC_SET_DEF)) return FALSE;
	// Display Switch On
	if (!bST7032i_writeCmd(ST7032i_CMD_DISP_CNTR_DEF | 0x04)) return FALSE;
	// Clear Screen
	return bST7032i_writeCmdStop(ST7032i_CMD_CLEAR_DISP);
}

/*****************************************************************************
 *
 * NAME: bST7032i_dispControl
 *
 * DESCRIPTION:画面制御設定（ディスプレイ表示、カーソル表示、アイコン表示）
 *
 * PARAMETERS:      Name            RW  Usage
 *     bool_t       bDispFlg        R   画面表示制御（ON/OFF）
 *     bool_t       bCursorFlg      R   カーソル表示表示制御（ON/OFF）
 *     bool_t       bBlinkFlg       R   カーソル点滅（ON/OFF）
 *     bool_t       bIconFlg        R   アイコン表示（ON/OFF）
 *
 * RETURNS:
 *      TRUE :設定成功
 *      FALSE:設定失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_dispControl(bool_t bDispFlg, bool_t bCursorFlg, bool_t bBlinkFlg, bool_t bIconFlg) {
	//=========================================================================
	// コマンドの編集
	//=========================================================================
	uint8 cntrCmd = ST7032i_CMD_DISP_CNTR_DEF;
	// 画面表示設定
	cntrCmd |= (0x04 * bDispFlg);
	// カーソル表示設定
	cntrCmd |= (0x02 * bCursorFlg);
	// カーソル点滅表示設定
	cntrCmd |= (0x01 * bBlinkFlg);
	// アイコン表示
	stST7032i_sts->bIconDispFlg = bIconFlg;

	//=========================================================================
	// コマンドの送信
	//=========================================================================
	// コマンドの送信開始
	if (!bST7032i_writeStart()) return FALSE;
	// 画面制御設定（ディスプレイON/OFF、カーソル表示、カーソル点滅）
	if (!bST7032i_writeCmd(cntrCmd)) return FALSE;
	// アイコン表示設定
	return bST7032i_writeControlEx();
}

/*****************************************************************************
 *
 * NAME: bST7032i_setContrast
 *
 * DESCRIPTION:コントラスト設定
 *
 * PARAMETERS:      Name            RW  Usage
 *    uint8         u8Contrast      R   画面コントラスト値（0-63）
 *
 * RETURNS:
 *      TRUE :設定成功
 *      FALSE:設定失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_setContrast(uint8 u8Contrast) {
	// コントラスト設定
	stST7032i_sts->u8Contrast = u8Contrast;
	// コマンドの送信開始
	if (!bST7032i_writeStart()) return FALSE;
	// アイコン表示設定
	return bST7032i_writeControlEx();
}

/*****************************************************************************
 *
 * NAME: bST7032i_clearScreen
 *
 * DESCRIPTION:LCDをクリア
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      TRUE :クリア成功
 *      FALSE:クリア失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_clearScreen() {
	// コマンド OR データの送信開始
	if (!bST7032i_writeStart()) return FALSE;
	// Display Clear
	return bST7032i_writeCmdStop(ST7032i_CMD_CLEAR_DISP);
}

/*****************************************************************************
 *
 * NAME: bST7032i_clearICON
 *
 * DESCRIPTION:アイコン表示をクリア
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      TRUE :クリア成功
 *      FALSE:クリア失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_clearICON() {
	//=========================================================================
	// アイコン表示処理
	//=========================================================================
	// アイコンデータのクリア
	memset(stST7032i_sts->u8IconData, 0, ST7032i_ICON_DATA_SIZE);
	// アイコンの出力
	uint8 addr;
	for (addr = 0; addr < ST7032i_ICON_DATA_SIZE; addr++) {
		bST7032i_writeIconData(addr);
	}
	// 実行結果の返却
	return TRUE;

}

/*****************************************************************************
 *
 * NAME: bST7032i_setCursor
 *
 * DESCRIPTION:カーソル移動（行、列）
 *
 * PARAMETERS:      Name            RW  Usage
 *    uint8         u8RowNo         R   移動先行
 *    uint8         u8ColNo         R   移動先列
 *
 * RETURNS:
 *      TRUE :移動成功
 *      FALSE:移動失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_setCursor(uint8 u8RowNo, uint8 u8ColNo) {
	// アドレス計算
	uint8 cmd = ST7032i_CMD_SET_DD_ADDR | (0x40 * u8RowNo + u8ColNo);
	// 書き込み開始処理
	if (!bST7032i_writeStart()) return FALSE;
	// カーソル移動（行、列）
	return bST7032i_writeCmdStop(cmd);
}

/*****************************************************************************
 *
 * NAME: bST7032i_returnHome
 *
 * DESCRIPTION:カーソル移動（先頭）
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      TRUE :移動成功
 *      FALSE:移動失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_returnHome() {
	// コマンド OR データの送信開始
	if (!bST7032i_writeStart()) return FALSE;
	// 画面制御設定（ディスプレイON/OFF、カーソル表示、カーソル点滅）
	return bST7032i_writeCmdStop(ST7032i_CMD_RETURN_HOME);
}

/*****************************************************************************
 *
 * NAME: bST7032i_cursorShiftL
 *
 * DESCRIPTION:カーソル移動（左）
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      TRUE :移動成功
 *      FALSE:移動失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_cursorShiftL() {
	// コマンド OR データの送信開始
	if (!bST7032i_writeStart()) return FALSE;
	// カーソルシフト（左）
	return bST7032i_writeCmdStop(ST7032i_CMD_CURSOR_SHIFT_L);
}

/*****************************************************************************
 *
 * NAME: bST7032i_cursorShiftR
 *
 * DESCRIPTION:カーソル移動（右）
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      TRUE :移動成功
 *      FALSE:移動失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_cursorShiftR() {
// コマンド OR データの送信開始
	if (!bST7032i_writeStart()) return FALSE;
	// カーソルシフト（右）
	return bST7032i_writeCmdStop(ST7032i_CMD_CURSOR_SHIFT_R);
}

/*****************************************************************************
 *
 * NAME: bST7032i_charRegist
 *
 * DESCRIPTION:外字登録処理
 *
 * PARAMETERS:      Name            RW  Usage
 * uint8            u8Ch            R   Character Code
 * uint8*           pu8CgDataList   R   CGData 8 Byte
 *
 * RETURNS:
 *      TRUE :表示成功
 *      FALSE:表示失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_charRegist(uint8 u8Ch, uint8 *pu8CgDataList) {
	//=========================================================================
	// 書き込み処理
	//=========================================================================
	// 書き込み開始処理
	if (!bST7032i_writeStart()) return FALSE;
	// CGアドレスの書き込み
	if (!bST7032i_writeCmdStop(ST7032i_CMD_SET_CG_ADDR | (u8Ch << 3))) return FALSE;
	// 書き込み開始処理
	if (!bST7032i_writeStart()) return FALSE;
	// 制御バイトの送信：データ
	if (u8I2C_write(ST7032i_RS_DATA) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
		return FALSE;
	}
	// データの書き込み
	uint8 idx;
	for (idx = 0; idx < 7; idx++) {
		if (u8I2C_write(pu8CgDataList[idx]) != I2CUTIL_STS_ACK) {
			bI2C_stopACK();
			return FALSE;
		}
	}
	// 終端バイトの書き込み
	if (u8I2C_writeStop(pu8CgDataList[7]) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
		return FALSE;
	}
	// 次回コマンド実行可能時刻を更新
	stST7032i_sts->u64nextExec = u64ST7032i_nextExecTime(ST7032i_CMD_DATA_WRITE);
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bST7032i_writeChar
 *
 * DESCRIPTION:文字表示
 *
 * PARAMETERS:      Name            RW  Usage
 *      char        cCh             R   表示文字
 *
 * RETURNS:
 *      TRUE :表示成功
 *      FALSE:表示失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_writeChar(char cCh) {
	// コマンド OR データの送信開始
	if (!bST7032i_writeStart()) return FALSE;
	// 文字データの書き込み処理
	return bST7032i_writeData(cCh);
}

/*****************************************************************************
 *
 * NAME: bST7032i_writeString
 *
 * DESCRIPTION:文字列表示
 *
 * PARAMETERS:      Name            RW  Usage
 *       char*      pcStr           R   表示文字列
 *
 * RETURNS:
 *      TRUE :表示成功
 *      FALSE:表示失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_writeString(char *pcStr) {
	//=========================================================================
	// 書き込み処理
	//=========================================================================
	// 入力チェック
	int length = strlen(pcStr);
	if (length <= 0) return TRUE;
	// コマンド OR データの送信開始
	if (!bST7032i_writeStart()) return FALSE;
	// 制御バイトの送信：データ
	if (u8I2C_write(ST7032i_RS_DATA) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
		return FALSE;
	}
	// データの書き込み
	while(length > 1) {
		if (u8I2C_write(*pcStr) != I2CUTIL_STS_ACK) {
			bI2C_stopACK();
			return FALSE;
		}
		pcStr++;
		length--;
	}
	// 終端バイトの書き込み
	if (u8I2C_writeStop(*pcStr) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
		return FALSE;
	}
	// 次回コマンド実行可能時刻を更新
	stST7032i_sts->u64nextExec = u64ST7032i_nextExecTime(ST7032i_CMD_DATA_WRITE);
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bST7032i_writeIcon
 *
 * DESCRIPTION:アイコン表示制御
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       u8IconNo        R   表示対象アイコンNo（1-80）
 *
 * RETURNS:
 *      TRUE :表示成功
 *      FALSE:表示失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bST7032i_writeIcon(uint8 u8IconNo) {
	//=========================================================================
	// 入力チェック
	//=========================================================================
	// 個数チェック
	if (u8IconNo == 0 || u8IconNo > 80) return FALSE;

	//=========================================================================
	// 書き込みデータの編集
	//=========================================================================
	// アドレス編集
	uint8 addr = (u8IconNo - 1) / 5;
	// データ変換表
	uint8 convList[5] = {0x10, 0x08, 0x04, 0x02, 0x01};
	// データ編集
	stST7032i_sts->u8IconData[addr] |= convList[(u8IconNo - 1) % 5];

	//=========================================================================
	// アイコン表示処理
	//=========================================================================
	// アイコンデータ書き込み
	return bST7032i_writeIconData(addr);
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/*****************************************************************************
 *
 * NAME: bST7032i_writeStart
 *
 * DESCRIPTION:コマンド OR データの送信開始
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      TRUE :開始成功
 *      FALSE:開始失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE bool_t bST7032i_writeStart() {
	// 書き込み開始処理
	if (!bI2C_startWrite(I2C_ADDR_ST7032I)) {
		return bST7032i_errorEnd();
	}
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bST7032i_writeCmd
 *
 * DESCRIPTION:コマンドの書き込み（継続コマンド有り）
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       u8Cmd           R   コマンド
 *
 * RETURNS:
 *      TRUE :コマンド送信成功
 *      FALSE:コマンド送信失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE bool_t bST7032i_writeCmd(uint8 u8Cmd) {
	// コマンド実行可能まで待つ
	u32TimerUtil_waitUntil(stST7032i_sts->u64nextExec);
	// 制御バイトの送信：コマンド
	if (u8I2C_write(ST7032i_RS_CMD) != I2CUTIL_STS_ACK) {
		return bST7032i_errorEnd();
	}
	// コマンドの送信
	if (u8I2C_write(u8Cmd) != I2CUTIL_STS_ACK) {
		return bST7032i_errorEnd();
	}
	// 次回コマンド実行可能時刻を更新
	stST7032i_sts->u64nextExec = u64ST7032i_nextExecTime(u8Cmd);
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bST7032i_writeCmdStop
 *
 * DESCRIPTION:コマンドの書き込み（継続コマンド無し）
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       u8Cmd           R   コマンド
 *
 * RETURNS:
 *      TRUE :コマンド送信成功
 *      FALSE:コマンド送信失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE bool_t bST7032i_writeCmdStop(uint8 u8Cmd) {
	// コマンド実行可能まで待つ
	u32TimerUtil_waitUntil(stST7032i_sts->u64nextExec);
	// 制御バイトの送信：コマンド
	if (u8I2C_write(ST7032i_RS_CMD) != I2CUTIL_STS_ACK) {
		return bST7032i_errorEnd();
	}
	// コマンドの送信
	if (u8I2C_writeStop(u8Cmd) != I2CUTIL_STS_ACK) {
		return bST7032i_errorEnd();
	}
	// 次回コマンド実行可能時刻を更新
	stST7032i_sts->u64nextExec = u64ST7032i_nextExecTime(u8Cmd);
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bST7032i_writeData
 *
 * DESCRIPTION:データの書き込み
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       u8Data          R   データ
 *
 * RETURNS:
 *      TRUE :データ送信成功
 *      FALSE:データ送信失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE bool_t bST7032i_writeData(uint8 u8Data) {
	//=========================================================================
	// 書き込み処理
	//=========================================================================
	// コマンド実行可能まで待つ
	u32TimerUtil_waitUntil(stST7032i_sts->u64nextExec);
	// 制御バイトの送信：データ
	if (u8I2C_write(ST7032i_RS_DATA) != I2CUTIL_STS_ACK) {
		return bST7032i_errorEnd();
	}
	// データの送信
	if (u8I2C_writeStop(u8Data) != I2CUTIL_STS_ACK) {
		return bST7032i_errorEnd();
	}
	// 次回コマンド実行可能時刻を更新
	stST7032i_sts->u64nextExec = u64ST7032i_nextExecTime(ST7032i_CMD_DATA_WRITE);
	// 正常終了
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bST7032i_writeIconData
 *
 * DESCRIPTION:アイコンの書き込み
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       u8Addr          R   アイコンアドレス
 *
 * RETURNS:
 *      TRUE :書き込み成功
 *      FALSE:書き込み失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE bool_t bST7032i_writeIconData(uint8 u8Addr) {
	// 有効アドレス
	uint8 validAdd = u8Addr & 0x0F;
	// 書き込み開始処理
	if (!bST7032i_writeStart()) return FALSE;
	// コマンドの送信（IS=1）
	if (!bST7032i_writeCmd(ST7032i_CMD_FUNC_SET_EX)) return FALSE;
	// コマンドの送信（アイコンアドレス）
	if (!bST7032i_writeCmd(ST7032i_CMD_SET_ICON_ADDR | validAdd)) return FALSE;
	// コマンドの送信（IS=0）
	if (!bST7032i_writeCmd(ST7032i_CMD_FUNC_SET_DEF)) return FALSE;
	// 書き込み開始処理
	if (!bST7032i_writeStart()) return FALSE;
	// データの送信
	return bST7032i_writeData(stST7032i_sts->u8IconData[validAdd]);
}

/*****************************************************************************
 *
 * NAME: bST7032i_writeControlEx
 *
 * DESCRIPTION:画面制御設定（アイコン表示、コントラスト）
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      TRUE :設定成功
 *      FALSE:設定失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE bool_t bST7032i_writeControlEx() {
	//=========================================================================
	// コマンド編集
	//=========================================================================
	// コマンド：アイコン表示・コントラスト上位桁
	uint8 cmdDisp = ST7032i_CMD_DISP_CNTR_EX;
	// アイコン表示
	cmdDisp |= 0x08 * stST7032i_sts->bIconDispFlg;
	// booster circuit
	cmdDisp |= 0x04;
	// コントラスト上位桁
	cmdDisp |= (stST7032i_sts->u8Contrast >> 4) & 0x03;
	// コマンド：コントラスト下位桁
	uint8 cmdContrast = ST7032i_CMD_CONTRAST_LOW | (stST7032i_sts->u8Contrast & 0x0f);

	//=========================================================================
	// コマンド書き込み
	//=========================================================================
	// コマンドの送信（IS=1）
	if (!bST7032i_writeCmd(ST7032i_CMD_FUNC_SET_EX)) return FALSE;
	// コマンドの送信（アイコン表示・コントラスト上位桁）
	if (!bST7032i_writeCmd(cmdDisp)) return FALSE;
	// コマンドの送信（コントラスト下位桁）
	if (!bST7032i_writeCmd(cmdContrast)) return FALSE;
	// コマンドの送信（IS=0）
	return bST7032i_writeCmdStop(ST7032i_CMD_FUNC_SET_DEF);
}

/*****************************************************************************
 *
 * NAME: u64ST7032i_nextExecTime
 *
 * DESCRIPTION:次回のコマンド実行可能時刻を取得する
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       u8Cmd           R   コマンド
 *
 * RETURNS:
 *      uint64 処理完了時間（マイクロ秒）
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE uint32 u64ST7032i_nextExecTime(uint8 u8Cmd) {
	// デバイスの処理待ち
	uint32 nextTick;
	switch(u8Cmd) {
		case ST7032i_CMD_CLEAR_DISP:
		case ST7032i_CMD_RETURN_HOME:
			// 1.08ミリ秒待つ
			nextTick = 1080 * TIMER_UTIL_TICK_PER_USEC;
			break;
		default:
			// 27マイクロ秒待つ
			nextTick = 27 * TIMER_UTIL_TICK_PER_USEC;
	}
	return u64TimerUtil_readUsec() + nextTick;
}

/*****************************************************************************
 *
 * NAME: bST7032i_errorEnd
 *
 * DESCRIPTION:エラー終了処理、I2Cの通信を完了してFALSEを返す
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      FALSE:設定失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE bool_t bST7032i_errorEnd() {
	bI2C_stopACK();
	return FALSE;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
