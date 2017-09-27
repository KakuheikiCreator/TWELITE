/*******************************************************************************
 *
 * MODULE :Application IO functions source file
 *
 * CREATED:2017/08/29 00:00:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:画面表示やキー入力等の基本的な入出力機能のヘッダファイル
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 *******************************************************************************
 * Copyright (c) 2017, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 ******************************************************************************/

/******************************************************************************/
/***        Include files                                                   ***/
/******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <jendefs.h>
#include <AppHardwareApi.h>

/******************************************************************************/
/***        ToCoNet Definitions                                             ***/
/******************************************************************************/
#include "serial.h"
#include "sprintf.h"

/******************************************************************************/
/***        ToCoNet Include files                                           ***/
/******************************************************************************/
#include "ToCoNet_use_mod.h"
#include "ToCoNet.h"
#include "ToCoNet_mod_prototype.h"
#include "ccitt8.h"

/******************************************************************************/
/***        User Include files                                              ***/
/******************************************************************************/
#include "config.h"
#include "config_default.h"
#include "framework.h"
#include "timer_util.h"
#include "i2c_util.h"
#include "io_util.h"
#include "pwm_util.h"
#include "value_util.h"
#include "st7032i.h"
#include "ds3231.h"
#include "eeprom.h"
#include "app_io.h"

/******************************************************************************/
/***        Macro Definitions                                               ***/
/******************************************************************************/

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/
// EEPROM検索（リモートデバイス情報のインデックス検索）
PRIVATE int iEEPROMIndexOfRemoteInfo(uint32 u32DeviceID);
// EEPROM読み込み（インデックス情報）
PRIVATE bool_t bEEPROMReadIndexInfo();
// EEPROM書き込み（インデックス情報）
PRIVATE bool_t bEEPROMWriteIndexInfo();

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/
/** コンスタント配列：ボタン電圧閾値 */
PRIVATE int const BTN_ADC_VAL[] = {915, 688, 395, 256};
/** コンスタント配列：ボタン番号 */
PRIVATE int const BTN_NO[] = {BTN_NO_0, BTN_NO_1, BTN_NO_2, BTN_NO_3};
/** インデックス情報 */
PRIVATE tsAppIOIndexInfo sIndexInfo;


/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/
/*******************************************************************************
 *
 * NAME: vAppIOInit
 *
 * DESCRIPTION:入出力ハードウェアの初期処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vAppIOInit() {
	//==========================================================================
	// デジタル入出力設定
	//==========================================================================
	// DIOピンの入出力を設定
//	vAHI_DioSetDirection(0x00, 0x00);
	// プルアップ設定
//	vAHI_DioSetPullup(0x00, 0x00);
	// ADCの初期化
	bIOUtil_adcInit(E_AHI_AP_CLOCKDIV_500KHZ);
	//==========================================================================
	// 入出力情報の初期化
	//==========================================================================
	// ボタン監視バッファ
	sAppIO.sButtonBuffer.u8ChkPin = PIN_NO_BUTTON_CHK;
	sAppIO.sButtonBuffer.u8LastValIdx = 0;
	//==========================================================================
	// 初回情報取得
	//==========================================================================
	// 初回ADCバッファリング
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < 5; u8Idx++) {
		vIOUtil_adcUpdateBuffer(&sAppIO.sButtonBuffer);
	}
	// ボタン監視値
	sAppIO.iButtonNo =
		iIOUtil_adcReadBuffer(&sAppIO.sButtonBuffer, ADC_CHK_TOLERANCE, ADC_BUFF_CHK_CNT);
	//==========================================================================
	// I2Cデバイス初期化
	//==========================================================================
	// EEPROM初期処理
	vEEPROMInit();
#ifdef DEBUG
	// デバイス情報の読み込み
	if (bEEPROMReadDevInfo() == FALSE) {
		vfPrintf(&sSerStream, "MS:%08d Read DevInfo Err\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// インデックスの読み込み
	if (bEEPROMReadIndexInfo(&sIndexInfo) == FALSE) {
		vfPrintf(&sSerStream, "MS:%08d Read IdxInfo Err\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
#else
	// デバイス情報の読み込み
	bEEPROMReadDevInfo();
	// インデックスの読み込み
	bEEPROMReadIndexInfo(&sIndexInfo);
#endif
	// LCD初期化処理
	vLCDInit();
	// 無線通信初期処理
	vWirelessInit(&sDevInfo);
}

/*******************************************************************************
 *
 * NAME: vUpdInputBuffer
 *
 * DESCRIPTION:入力バッファの更新
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vUpdInputBuffer() {
	// 押しボタン更新
	vIOUtil_adcUpdateBuffer(&sAppIO.sButtonBuffer);
}

/*****************************************************************************
 *
 * NAME: iReadButton
 *
 * DESCRIPTION:ボタンバッファの読み込み
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      int 入力されたボタンの番号を返す、入力無しの場合にはマイナス値を返す。
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC int iReadButton() {
	int iVal = iIOUtil_adcReadBuffer(&sAppIO.sButtonBuffer, ADC_CHK_TOLERANCE, ADC_BUFF_CHK_CNT);
	if (iVal < 0) {
		return -1;
	}
	int iBtnNo = -1;
	int iFrom;
	int iTo;
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < 4; u8Idx++) {
		iFrom = BTN_ADC_VAL[u8Idx] - ADC_CHK_TOLERANCE;
		iTo   = BTN_ADC_VAL[u8Idx] + ADC_CHK_TOLERANCE;
		if (iVal >= iFrom && iVal <= iTo) {
			iBtnNo = BTN_NO[u8Idx];
			break;
		}
	}
	if (sAppIO.iButtonNo == iBtnNo) {
		return -1;
	}
	sAppIO.iButtonNo = iBtnNo;
	return sAppIO.iButtonNo;
}
/*******************************************************************************
 *
 * NAME: vEEPROMInit
 *
 * DESCRIPTION:EEPROM初期化
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEEPROMInit() {
	// EEPROMの初期化
	sEEPROM_status.u8DevAddress  = I2C_ADDR_EEPROM_0;
	sEEPROM_status.b2ByteAddrFlg = TRUE;
	sEEPROM_status.u8PageSize    = EEPROM_PAGE_SIZE_64B;
	sEEPROM_status.u64LastWrite  = u64TimerUtil_readUsec();
}

/*******************************************************************************
 *
 * NAME: bEEPROMReadDevInfo
 *
 * DESCRIPTION:EEPROM読み込み（デバイス情報）
 *
 * PARAMETERS:          Name         RW  Usage
 *
 * RETURNS:
 *   bool_t TRUE:読み込み成功
 *
 ******************************************************************************/
PUBLIC bool_t bEEPROMReadDevInfo() {
	// I2C EEPROM Read
	bEEPROM_deviceSelect(&sEEPROM_status);
	return bEEPROM_readData(TOP_ADDR_DEV, sizeof(tsAuthDeviceInfo), (uint8*)&sDevInfo);
}

/*******************************************************************************
 *
 * NAME: bEEPROMWriteDevInfo
 *
 * DESCRIPTION:EEPROM書き込み（デバイス情報）
 *
 * PARAMETERS:  Name             RW  Usage
 *      uint8   u8StatusMapMask  R   ステータスマップマスク
 *
 * RETURNS:
 *   bool_t TRUE:書き込み成功
 *
 ******************************************************************************/
PUBLIC bool_t bEEPROMWriteDevInfo(uint8 u8StatusMapMask) {
	// 実行判定
	if ((sDevInfo.u8StatusMap & u8StatusMapMask) == u8StatusMapMask) {
		return FALSE;
	}
	// ステータスマップの更新
	sDevInfo.u8StatusMap = sDevInfo.u8StatusMap | u8StatusMapMask;
	// I2C EEPROM Write
	bEEPROM_deviceSelect(&sEEPROM_status);
	return bEEPROM_writeData(TOP_ADDR_DEV, sizeof(tsAuthDeviceInfo), (uint8*)&sDevInfo);
}


/*******************************************************************************
 *
 * NAME: iEEPROMReadRemoteInfo
 *
 * DESCRIPTION:EEPROM読み込み（リモートデバイス情報）
 *
 * PARAMETERS:             Name          RW  Usage
 *   tsAuthRemoteDevInfo*  psRemoteInfo  R   リモートデバイス情報
 *   uint8                 u8Idx         R   探索開始インデックス
 *   bool_t                bForwardFlg   R   順方向探索フラグ
 *
 * RETURNS:
 *   int 読み込みレコードインデックス、エラー時はマイナス値
 *       (-1:対象データなし、-2:読み込みエラー)
 *
 ******************************************************************************/
PUBLIC int iEEPROMReadRemoteInfo(tsAuthRemoteDevInfo* psRemoteInfo, uint8 u8Idx, bool_t bForwardFlg) {
	// 探索方向判定
	uint8 u8Add = 1;
	if (bForwardFlg == FALSE) {
		u8Add = MAX_REMOTE_DEV_CNT - 1;
	}
	// リモートデバイス情報のインデックス探索
	uint8 u8ChkIdx = u8Idx;
	uint8 u8Cnt;
	for (u8Cnt = 0; u8Cnt < MAX_REMOTE_DEV_CNT; u8Cnt++) {
		if ((sIndexInfo.u32RemoteDevMap & (0x01 << u8ChkIdx)) != 0) {
			break;
		}
		u8ChkIdx = (u8ChkIdx + u8Add) % MAX_REMOTE_DEV_CNT;
	}
	if (u8Cnt >= MAX_REMOTE_DEV_CNT) {
		return -2;
	}
	// I2C EEPROM Read
	uint32 u32Size = sizeof(tsAuthRemoteDevInfo);
	uint32 u32Addr = TOP_ADDR_REMOTE_DEV + u32Size * u8ChkIdx;
	// I2C EEPROM Read
	bEEPROM_deviceSelect(&sEEPROM_status);
	if (!bEEPROM_readData(u32Addr, u32Size, (uint8*)psRemoteInfo)) {
		return -3;
	}
	return u8ChkIdx;
}

/*******************************************************************************
 *
 * NAME: bEEPROMWriteRemoteInfo
 *
 * DESCRIPTION:EEPROM書き込み（リモートデバイス情報）
 *
 * PARAMETERS:             Name         RW  Usage
 * tsAuthRemoteDevInfo*    psDevInfo    R   リモートデバイス情報
 *
 * RETURNS:
 *   int 書き込みレコードインデックス、エラー時はマイナス値
 *
 ******************************************************************************/
PUBLIC int iEEPROMWriteRemoteInfo(tsAuthRemoteDevInfo *psRemoteInfo) {
	// レコードインデックス探索
	int iIdx = iEEPROMIndexOfRemoteInfo(psRemoteInfo->u32DeviceID);
	if (iIdx < 0) {
		// 空き領域の有無判定
		if (sIndexInfo.u32RemoteDevMap == 0xFFFFFFFF) {
			return -1;
		}
		// 空き領域の探索
		for (iIdx = 0; iIdx < MAX_REMOTE_DEV_CNT; iIdx++) {
			if ((sIndexInfo.u32RemoteDevMap & (0x01 << iIdx)) == 0) {
				break;
			}
		}
		// インデックス情報更新
		sIndexInfo.u32RemoteDevMap = sIndexInfo.u32RemoteDevMap | (0x00000001 << iIdx);
		if (!bEEPROMWriteIndexInfo(&sIndexInfo)) {
			return -2;
		}
	}
	// I2C EEPROM Write
	uint32 u32Size = sizeof(tsAuthRemoteDevInfo);
	uint32 u32Addr = TOP_ADDR_REMOTE_DEV + u32Size * iIdx;
	bEEPROM_deviceSelect(&sEEPROM_status);
	if (!bEEPROM_writeData(u32Addr, u32Size, (uint8 *)psRemoteInfo)) {
		return -3;
	}
	return iIdx;
}

/*******************************************************************************
 *
 * NAME: iEEPROMWriteLog
 *
 * DESCRIPTION:EEPROM書き込み（エラーイベントログ情報）
 *
 * PARAMETERS:           Name              RW  Usage
 *   uint16              u16MsgCd          R   メッセージコード
 *   tsWirelessMsg*      psMsg             R   受信メッセージ
 *
 * RETURNS:
 *   int 書き込みレコードインデックス、読み込めない場合はマイナス値
 *
 ******************************************************************************/
PUBLIC int iEEPROMWriteLog(uint16 u16MsgCd, tsWirelessMsg* psMsg) {
	// ログ数の判定
	if (sIndexInfo.u8EventLogCnt >= MAX_EVT_LOG_CNT) {
		return -1;
	}
	// インデックス情報更新
	sIndexInfo.u8EventLogCnt++;
	if (bEEPROMWriteIndexInfo(&sIndexInfo) == FALSE) {
		return -2;
	}
	// イベントログ編集
	tsAppIOEventLog sAppIOEventLog;
	memset(&sAppIOEventLog, 0x00, sizeof(tsAppIOEventLog));
	sAppIOEventLog.u8SeqNo   = sIndexInfo.u8EventLogCnt - 1;// シーケンス番号
	sAppIOEventLog.u16MsgCd  = u16MsgCd;					// メッセージコード
	sAppIOEventLog.u8Command = psMsg->u8Command;			// コマンド
	sAppIOEventLog.u8StatusMap = psMsg->u8StatusMap;		// ステータスマップ
	sAppIOEventLog.u16Year   = psMsg->u16Year;				// イベント発生日（年）
	sAppIOEventLog.u8Month   = psMsg->u8Month;				// イベント発生日（月）
	sAppIOEventLog.u8Day     = psMsg->u8Day;				// イベント発生日（日）
	sAppIOEventLog.u8Hour    = psMsg->u8Hour;				// イベント発生時刻（時）
	sAppIOEventLog.u8Minute  = psMsg->u8Minute;				// イベント発生時刻（分）
	sAppIOEventLog.u8Second  = psMsg->u8Second;				// イベント発生時刻（秒）
	// I2C EEPROM Write
	int iIdx = sIndexInfo.u8EventLogCnt - 1;
	uint32 u32Size = sizeof(tsAppIOEventLog);
	uint32 u32Addr = TOP_ADDR_EVENT_LOG + u32Size * iIdx;
	bEEPROM_deviceSelect(&sEEPROM_status);
	if (bEEPROM_writeData(u32Addr, u32Size, (uint8*)&sAppIOEventLog) == FALSE) {
		return -3;
	}
	return iIdx;
}

/*******************************************************************************
 *
 * NAME: vLCDInit
 *
 * DESCRIPTION:LCDの初期化処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vLCDInit() {
	// LCDデバイスのアドレス
	sLCDstate.u8Address = I2C_ADDR_ST7032I;
	// LCD：コントラスト
	sLCDstate.u8Contrast = LCD_CONTRAST;
	// LCD：アイコンメモリ
	memset(sLCDstate.u8IconData, (uint8)0, ST7032i_ICON_DATA_SIZE);
	// LCDの初期化
	bST7032i_deviceSelect(&sLCDstate);
	bST7032i_init();
}

/*******************************************************************************
 *
 * NAME: vEventLCDdrawing
 *
 * DESCRIPTION:LCD描画処理、描画領域に書き込まれた内容をLCDに表示する
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 *
 *****************************************************************************/
PUBLIC void vLCDdrawing(char* pcMsg0, char* pcMsg1) {
	// 終端文字の上書き
	pcMsg0[LCD_BUFF_COL_SIZE] = '\0';
	pcMsg1[LCD_BUFF_COL_SIZE] = '\0';
	// LCD描画
#ifndef DEBUG
	bST7032i_deviceSelect(&sLCDstate);
	bST7032i_setCursor(0, 0);
	bST7032i_writeString(pcMsg0);
	bST7032i_setCursor(1, 0);
	bST7032i_writeString(pcMsg1);
#else
	if (!bST7032i_deviceSelect(&sLCDstate)) {
		vfPrintf(&sSerStream, "1:Err.");
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
	if (!bST7032i_setCursor(0, 0)) {
		vfPrintf(&sSerStream, "2:Err.");
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
	if (!bST7032i_writeString(pcMsg0)) {
		vfPrintf(&sSerStream, "3:Err.");
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
	if (!bST7032i_setCursor(1, 0)) {
		vfPrintf(&sSerStream, "4:Err.");
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
	if (!bST7032i_writeString(pcMsg1)) {
		vfPrintf(&sSerStream, "5:Err.");
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
#endif
}

/*******************************************************************************
 *
 * NAME: vWirelessInit
 *
 * DESCRIPTION:無線通信の初期化処理
 *
 * PARAMETERS:          Name        RW  Usage
 *   tsAuthDeviceInfo*  psDevInfo   R   デバイス情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vWirelessInit(tsAuthDeviceInfo*  psDevInfo) {
	// 無線通信情報を無効状態にする
	memset(&sWirelessInfo, 0x00, sizeof(tsWirelessInfo));
	sWirelessInfo.bRxEnabled = FALSE;	// 受信無効化
	// 通信暗号化設定
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < 12 && psDevInfo->u8AESPassword[u8Idx] == '\0'; u8Idx++);
	// 暗号化判定
	if (u8Idx < 12) {
		uint8 u8KeyBuff[16];
		memset(u8KeyBuff, '0', 4);
		memcpy(&u8KeyBuff[4], psDevInfo->u8AESPassword, 12);
		ToCoNet_bRegisterAesKey(u8KeyBuff, NULL);
	}
}

/*******************************************************************************
 *
 * NAME: vWirelessRxEnabled
 *
 * DESCRIPTION:電文の受信有効化メソッド
 *
 * PARAMETERS:      Name          RW  Usage
 *     uint32       u32TgtAddr    R   対象デバイスのアドレス
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vWirelessRxEnabled(uint32 u32TgtAddr, tsWirelessMsg* psWlsMsg) {
	sWirelessInfo.u32TgtAddr = u32TgtAddr;		// 通信対象アドレス
	sWirelessInfo.bRxEnabled = TRUE;			// 受信の有効化
	sWirelessInfo.psWlsRxMsg = psWlsMsg;		// 受信メッセージ
}

/*******************************************************************************
 *
 * NAME: vWirelessRxDisabled
 *
 * DESCRIPTION:電文の受信無効化メソッド
 *
 * PARAMETERS:          Name        RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vWirelessRxDisabled() {
	// 無線通信情報を初期化
	sWirelessInfo.u32TgtAddr = 0;				// 通信対象アドレス
	sWirelessInfo.bRxEnabled = FALSE;			// 受信有効化
	sWirelessInfo.psWlsRxMsg = 0x00;			// 受信メッセージ
}

/*******************************************************************************
 *
 * NAME: bWirelessRxPkt
 *
 * DESCRIPTION:無線通信によるメッセージ受信処理
 *
 * PARAMETERS:          Name        RW  Usage
 * tsRxDataApp*         psRx        R   受信パケット
 *
 * RETURNS:
 *     TRUE:受信データバッファリング
 *
 ******************************************************************************/
PUBLIC bool_t bWirelessRxPkt(tsRxDataApp *psRx) {
	//==========================================================================
	// 受信パケットチェック
	//==========================================================================
	// チェック有効判定
	if (sWirelessInfo.bRxEnabled == FALSE) {
		return FALSE;
	}
	// 送信元アドレス
	if (psRx->u32SrcAddr != sWirelessInfo.u32TgtAddr) {
		return FALSE;
	}
	// 送信先アドレス判定（ブロードキャスト）
	if (psRx->u32DstAddr != TOCONET_MAC_ADDR_BROADCAST) {
		return FALSE;
	}
	// データ種別判定（ユーザー向けデータ判定）
	if (psRx->u8Cmd != TOCONET_PACKET_CMD_APP_DATA) {
		return FALSE;
	}
	// 受信データ長判定
	if (psRx->u8Len != sizeof(tsWirelessMsg)) {
		return FALSE;
	}
	//==========================================================================
	// 受信メッセージチェック
	//==========================================================================
	// 受信メッセージ読み出し
	memcpy(sWirelessInfo.psWlsRxMsg, psRx->auData, psRx->u8Len);
	tsWirelessMsg* psWlsMsg = sWirelessInfo.psWlsRxMsg;
	// 宛先アドレス
	if (psWlsMsg->u32DstAddr != sDevInfo.u32DeviceID) {
		return FALSE;
	}
	// 受信無効化
	vWirelessRxDisabled();
	//受信完了
	return TRUE;
}

/****************************************************************************
 *
 * NAME: bWirelessTx
 *
 * DESCRIPTION:無線通信によるメッセージ送信処理
 *
 * PARAMETERS:          Name        RW  Usage
 *     uint32           u32SrcAddr  R   送信元アドレス
 *     bool_t           bEncryption R   暗号化有無
 *     uint8*           u8Data      R   送信データ
 *     uint8            u8Len       R   送信データ長
 *
 * RETURNS:
 *     TRUE:送信成功
 *
 ****************************************************************************/
PUBLIC bool_t bWirelessTx(uint32 u32SrcAddr, bool_t bEncryption, uint8* u8Data, uint8 u8Len) {
	// 送信パケット編集
	tsTxDataApp sTx;
	memset(&sTx, 0x00, sizeof(tsTxDataApp));
	sTx.u32SrcAddr = u32SrcAddr;					// 送信元アドレス
	sTx.u32DstAddr = TOCONET_MAC_ADDR_BROADCAST;	// 送信先アドレス（ブロードキャスト）
	sTx.u8Seq = sWirelessInfo.u8TxSeq++;			// フレームシーケンス
	sTx.u8CbId = sTx.u8Seq;							// コールバックID（フレームシーケンス指定）
	sTx.bAckReq = FALSE;							// ACK返信有無
	sTx.u8Retry = 0;								// 再送回数（なし）
	sTx.u16RetryDur = 0;							// 再送間隔（ミリ秒単位）
	sTx.u16DelayMin = 4;							// 遅延送信時間（最小）
	sTx.u16DelayMax = 16;							// 遅延送信時間（最大）
	sTx.bSecurePacket = bEncryption;				// 暗号化設定
	sTx.u8Cmd = TOCONET_PACKET_CMD_APP_DATA;		// パケットタイプ
	sTx.u8Len = u8Len;								// 送信データ長
	memcpy(sTx.auData, u8Data, sTx.u8Len);			// 送信データ
	// 送信処理
	return ToCoNet_bMacTxReq(&sTx);
}

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

/****************************************************************************
 *
 * NAME: iEEPROMReadRemoteInfo
 *
 * DESCRIPTION:EEPROM検索（リモートデバイス情報のインデックス検索）
 *
 * PARAMETERS:             Name         RW  Usage
 * uint32                  u32DeviceID  R   対象レコードのデバイスID
 *
 * RETURNS:
 *   int 読み込みレコードインデックス、エラー時はマイナス値
 *
 ****************************************************************************/
PRIVATE int iEEPROMIndexOfRemoteInfo(uint32 u32DeviceID) {
	// リモートデバイス情報の探索
	bEEPROM_deviceSelect(&sEEPROM_status);
	tsAuthRemoteDevInfo wkInfo;
	uint32 u32Size = sizeof(tsAuthRemoteDevInfo);
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < MAX_REMOTE_DEV_CNT; u8Idx++) {
		// レコード有無判定
		if ((sIndexInfo.u32RemoteDevMap & (0x01 << u8Idx)) == 0) {
			continue;
		}
		// レコード読み込み
		if (!bEEPROM_readData(TOP_ADDR_REMOTE_DEV + u32Size * u8Idx, u32Size, (uint8*)&wkInfo)) {
			return -1;
		}
		// デバイスID判定
		if (wkInfo.u32DeviceID == u32DeviceID) {
			return u8Idx;
		}
	}
	// 対象レコード無し
	return -1;
}

/*******************************************************************************
 *
 * NAME: bEEPROMReadIndexInfo
 *
 * DESCRIPTION:EEPROM読み込み（インデックス情報）
 *
 * PARAMETERS:          Name         RW  Usage
 *
 * RETURNS:
 *   bool_t TRUE:読み込み成功
 *
 ******************************************************************************/
PRIVATE bool_t bEEPROMReadIndexInfo() {
	// I2C EEPROM Read
	bEEPROM_deviceSelect(&sEEPROM_status);
	bool_t bResult = bEEPROM_readData(TOP_ADDR_INDEX, sizeof(tsAppIOIndexInfo), (uint8*)&sIndexInfo);
	// レコード有効チェック
	if (sIndexInfo.u8EnableCheck != 0xAA) {
		// レコード初期化
		memset(&sIndexInfo, 0x00, sizeof(tsAppIOIndexInfo));
		sIndexInfo.u8EnableCheck = 0xAA;
		return bEEPROMWriteIndexInfo();
	}
	// 読み込み成功
	return bResult;
}

/*******************************************************************************
 *
 * NAME: bEEPROMWriteIndexInfo
 *
 * DESCRIPTION:EEPROM書き込み（インデックス情報）
 *
 * PARAMETERS:          Name         RW  Usage
 *
 * RETURNS:
 *   bool_t TRUE:読み込み成功
 *
 ******************************************************************************/
PRIVATE bool_t bEEPROMWriteIndexInfo() {
	// I2C EEPROM Write
	bEEPROM_deviceSelect(&sEEPROM_status);
	return bEEPROM_writeData(TOP_ADDR_INDEX, sizeof(tsAppIOIndexInfo), (uint8 *)&sIndexInfo);
}

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
