/*******************************************************************************
 *
 * MODULE :Application IO functions source file
 *
 * CREATED:2016/05/30 01:32:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:画面表示やキー入力等の基本的な入出力機能のヘッダファイル
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 *******************************************************************************
 * Copyright (c) 2016, Nakanohito
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
// EEPROM読み込み（インデックス情報）
PRIVATE bool_t bEEPROMReadIndexInfo();
// EEPROM書き込み（インデックス情報）
PRIVATE bool_t bEEPROMWriteIndexInfo();
// EEPROM読み込み（リモートデバイス情報の初期処理）
PRIVATE int iEEPROMRemoteInfoInit();

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/
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
	uint32 u32PinMapInput = 0x00;
	u32PinMapInput |= PIN_MAP_TEST_SW;		// Pin Map:テストスイッチ
	u32PinMapInput |= PIN_MAP_IR_SENS;		// Pin Map:人感センサー
	u32PinMapInput |= PIN_MAP_TOUCH_SENS;	// Pin Map:タッチセンサー
	u32PinMapInput |= PIN_MAP_OPEN_SENS;	// Pin Map:開放取り外しセンサー
	u32PinMapInput |= PIN_MAP_SERVO_CHK;	// Pin Map:サーボチェック
	u32PinMapInput |= PIN_MAP_SERVO_SET_A;	// Pin Map:サーボ設定A
	u32PinMapInput |= PIN_MAP_SERVO_SET_B;	// Pin Map:サーボ設定B
	uint32 u32PinMapOutput = 0x00;
	u32PinMapOutput |= PIN_MAP_5V_CNTR;		// Pin Map:5V電源制御
	u32PinMapOutput |= PIN_MAP_SERVO_CNTR;	// Pin Map:サーボ制御
	// DIOピンの入出力を設定
	vAHI_DioSetDirection(u32PinMapInput, u32PinMapOutput);
	// プルアップ設定
	uint32 u32PinMapPullDown = 0x00;
	u32PinMapPullDown |= PIN_MAP_TEST_SW;
	u32PinMapPullDown |= PIN_MAP_OPEN_SENS;
	u32PinMapPullDown |= PIN_MAP_IR_SENS;
	u32PinMapPullDown |= PIN_MAP_TOUCH_SENS;
	vAHI_DioSetPullup(0x00, u32PinMapPullDown);
	// ADCの初期化
	bIOUtil_adcInit(E_AHI_AP_CLOCKDIV_500KHZ);
	//==========================================================================
	// 入出力情報の初期化
	//==========================================================================
	sAppIO.u32DiMap    = 0;			// デジタル入力ピンマップ
	sAppIO.u32DiMapBef = 0;			// デジタル入力ピンマップ（前回分）
	sAppIO.b5VPwSupply = FALSE;		// 5V電源供給有無
	// サーボ監視バッファ
	sAppIO.sServoPosBuffer.u8ChkPin = PIN_NO_SERVO_CHK;
	sAppIO.sServoPosBuffer.u8LastValIdx = 0;
	// サーボ設定Aバッファ
	sAppIO.sServoSetABuffer.u8ChkPin = PIN_NO_SERVO_SET_A;
	sAppIO.sServoSetABuffer.u8LastValIdx = 0;
	// サーボ設定Bバッファ
	sAppIO.sServoSetBBuffer.u8ChkPin = PIN_NO_SERVO_SET_B;
	sAppIO.sServoSetBBuffer.u8LastValIdx = 0;
	// マスタートークンマスク
	memset(sAppIO.u8MstTokenMask, 0x00, APP_AUTH_TOKEN_SIZE);
	// 5V供給停止
	v5VPwOff();
	//==========================================================================
	// 初回情報取得
	//==========================================================================
	// 初回ADCバッファリング
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < 5; u8Idx++) {
		vIOUtil_adcUpdateBuffer(&sAppIO.sServoPosBuffer);
		vIOUtil_adcUpdateBuffer(&sAppIO.sServoSetABuffer);
		vIOUtil_adcUpdateBuffer(&sAppIO.sServoSetBBuffer);
	}
	// サーボ監視値
	sAppIO.iServoPos  = iIOUtil_adcReadBuffer(&sAppIO.sServoPosBuffer, ADC_CHK_TOLERANCE, ADC_BUFF_CHK_CNT);
	// サーボ設定値
	sAppIO.iServoSetA = iIOUtil_adcReadBuffer(&sAppIO.sServoSetABuffer, ADC_CHK_TOLERANCE, ADC_BUFF_CHK_CNT);
	// サーボ設定値
	sAppIO.iServoSetB = iIOUtil_adcReadBuffer(&sAppIO.sServoSetBBuffer, ADC_CHK_TOLERANCE, ADC_BUFF_CHK_CNT);
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
	// リモートデバイス情報のインデックスキャッシュの読み込み
	if (iEEPROMRemoteInfoInit() < 0) {
		vfPrintf(&sSerStream, "MS:%08d Init Remote Info Err\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// 日時温度更新
	vDateTimeUpdate();
	// LCD初期化処理
	vLCDInit();
#else
	// デバイス情報の読み込み
	bEEPROMReadDevInfo();
	// インデックスの読み込み
	bEEPROMReadIndexInfo(&sIndexInfo);
	// リモートデバイス情報の初期処理
	iEEPROMRemoteInfoInit();
	// 日時温度更新
	vDateTimeUpdate();
#endif
	// 無線通信初期処理
	vWirelessInit(&sDevInfo);
}

/*******************************************************************************
 *
 * NAME: v5VPwOn
 *
 * DESCRIPTION:5Ｖ電源ON
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void v5VPwOn() {
	// 実施判定
	if (sAppIO.b5VPwSupply == TRUE) {
		return;
	}
	// 5V電源ON
	vAHI_DioSetDirection(0x00, PIN_MAP_5V_CNTR | PIN_MAP_SERVO_CNTR);
	vAHI_DioSetOutput(PIN_MAP_5V_CNTR, 0x00);
	sAppIO.b5VPwSupply = TRUE;
}

/*******************************************************************************
 *
 * NAME: v5VPwOff
 *
 * DESCRIPTION:5Ｖ電源OFF
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void v5VPwOff() {
	// 実施判定
	if (sAppIO.b5VPwSupply == FALSE) {
		return;
	}
	// 5V電源OFF
	vAHI_DioSetDirection(0x00, PIN_MAP_5V_CNTR | PIN_MAP_SERVO_CNTR);
	vAHI_DioSetOutput(0x00, PIN_MAP_5V_CNTR);
	sAppIO.b5VPwSupply = FALSE;
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
	// デジタル入力バッファ更新
	vIOUtil_diUpdateBuffer();
	// 入力ピンマップを取得
	uint32 u32PinMap = PIN_MAP_TEST_SW | PIN_MAP_IR_SENS| PIN_MAP_TOUCH_SENS | PIN_MAP_OPEN_SENS;
	uint32 u32WkVal = u32IOUtil_diReadBuffer(u32PinMap, DI_BUFF_CHK_CNT);
	if (u32WkVal != 0x80000000) {
		sAppIO.u32DiMapBef = sAppIO.u32DiMap;
		sAppIO.u32DiMap = u32WkVal;
	}
	// サーボ位置更新
	vIOUtil_adcUpdateBuffer(&sAppIO.sServoPosBuffer);
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
	if ((sDevInfo.u8StatusMap | u8StatusMapMask) == sDevInfo.u8StatusMap) {
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
 * NAME: iEEPROMIndexOfRemoteInfo
 *
 * DESCRIPTION:EEPROM検索（リモートデバイス情報のインデックス検索）
 *
 * PARAMETERS:             Name         RW  Usage
 * uint32                  u32DeviceID  R   対象レコードのデバイスID
 *
 * RETURNS:
 *   int 読み込みレコードインデックス、エラー時はマイナス値
 *
 ******************************************************************************/
PUBLIC int iEEPROMIndexOfRemoteInfo(uint32 u32DeviceID) {
	// ローカルインデックスキャッシュ探索
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < sAppIOIndexCache.u8RemoteDevCnt; u8Idx++) {
		if (sAppIOIndexCache.u32RemoteDevID[u8Idx] == u32DeviceID) {
			return sAppIOIndexCache.u8RemoteDevIdx[u8Idx];
		}
	}
	// 対象レコード無し
	return -1;
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
 *
 * RETURNS:
 *   int 読み込みレコードインデックス、エラー時はマイナス値
 *       (-1:対象データなし、-2:読み込みエラー)
 *
 ******************************************************************************/
PUBLIC int iEEPROMReadRemoteInfo(tsAuthRemoteDevInfo *psRemoteInfo, uint8 u8Idx) {
	// リモートデバイス情報のインデックス探索
	uint8 u8ChkIdx;
	for (u8ChkIdx = u8Idx; u8ChkIdx < MAX_REMOTE_DEV_CNT; u8ChkIdx++) {
		if ((sIndexInfo.u32RemoteDevMap & (0x01 << u8ChkIdx)) != 0) {
			break;
		}
		u8ChkIdx = (u8ChkIdx + 1) % MAX_REMOTE_DEV_CNT;
	}
	if (u8ChkIdx >= MAX_REMOTE_DEV_CNT) {
		return -1;
	}
	// I2C EEPROM Read
	uint32 u32Size = sizeof(tsAuthRemoteDevInfo);
	uint32 u32Addr = TOP_ADDR_REMOTE_DEV + u32Size * u8ChkIdx;
	bEEPROM_deviceSelect(&sEEPROM_status);
	if (bEEPROM_readData(u32Addr, u32Size, (uint8*)psRemoteInfo) == FALSE) {
		return -2;
	}
	// マスクオフ処理
	vValUtil_masking(psRemoteInfo->u8AuthCode, sAppIO.u8MstTokenMask, APP_AUTH_TOKEN_SIZE);
	vValUtil_masking(psRemoteInfo->u8AuthHash, sAppIO.u8MstTokenMask, APP_AUTH_TOKEN_SIZE);
	vValUtil_masking(psRemoteInfo->u8SyncToken, sAppIO.u8MstTokenMask, APP_AUTH_TOKEN_SIZE);
	psRemoteInfo->u8SndStretching =
			u8ValUtil_masking(psRemoteInfo->u8SndStretching, sAppIO.u8MstTokenMask, APP_AUTH_TOKEN_SIZE);
	psRemoteInfo->u8RcvStretching =
			u8ValUtil_masking(psRemoteInfo->u8RcvStretching, sAppIO.u8MstTokenMask, APP_AUTH_TOKEN_SIZE);
	// 読み込み成功
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
		// ローカルインデックスキャッシュ更新
		sAppIOIndexCache.u8RemoteDevIdx[sAppIOIndexCache.u8RemoteDevCnt] = iIdx;
		sAppIOIndexCache.u32RemoteDevID[sAppIOIndexCache.u8RemoteDevCnt] = psRemoteInfo->u32DeviceID;
		sAppIOIndexCache.u8RemoteDevCnt++;
	}
	// マスキング処理
	vValUtil_masking(psRemoteInfo->u8AuthCode, sAppIO.u8MstTokenMask, APP_AUTH_TOKEN_SIZE);
	vValUtil_masking(psRemoteInfo->u8AuthHash, sAppIO.u8MstTokenMask, APP_AUTH_TOKEN_SIZE);
	vValUtil_masking(psRemoteInfo->u8SyncToken, sAppIO.u8MstTokenMask, APP_AUTH_TOKEN_SIZE);
	psRemoteInfo->u8SndStretching =
			u8ValUtil_masking(psRemoteInfo->u8SndStretching, sAppIO.u8MstTokenMask, APP_AUTH_TOKEN_SIZE);
	psRemoteInfo->u8RcvStretching =
			u8ValUtil_masking(psRemoteInfo->u8RcvStretching, sAppIO.u8MstTokenMask, APP_AUTH_TOKEN_SIZE);
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
 *   uint8               u8Cmd             R   コマンド
 *
 * RETURNS:
 *   int 書き込みレコードインデックス、読み込めない場合はマイナス値
 *
 ******************************************************************************/
PUBLIC int iEEPROMWriteLog(uint16 u16MsgCd, uint8 u8Cmd) {
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
	sAppIOEventLog.u8SeqNo     = sIndexInfo.u8EventLogCnt - 1;	// シーケンス番号
	sAppIOEventLog.u16MsgCd    = u16MsgCd;						// メッセージコード
	sAppIOEventLog.u8Command   = u8Cmd;							// コマンド
	sAppIOEventLog.u8StatusMap = sDevInfo.u8StatusMap;			// ステータスマップ
	sAppIOEventLog.u16Year     = sAppIO.sDatetime.u16Year;		// イベント発生日（年）
	sAppIOEventLog.u8Month     = sAppIO.sDatetime.u8Month;		// イベント発生日（月）
	sAppIOEventLog.u8Day       = sAppIO.sDatetime.u8Day;		// イベント発生日（日）
	sAppIOEventLog.u8Hour      = sAppIO.sDatetime.u8Hour;		// イベント発生時刻（時）
	sAppIOEventLog.u8Minute    = sAppIO.sDatetime.u8Minutes;	// イベント発生時刻（分）
	sAppIOEventLog.u8Second    = sAppIO.sDatetime.u8Seconds;	// イベント発生時刻（秒）
	// I2C EEPROM Write
	uint32 u32Size = sizeof(tsAppIOEventLog);
	uint32 u32Addr = TOP_ADDR_EVENT_LOG + u32Size * sAppIOEventLog.u8SeqNo;
	bEEPROM_deviceSelect(&sEEPROM_status);
	if (bEEPROM_writeData(u32Addr, u32Size, (uint8*)&sAppIOEventLog) == FALSE) {
		return -3;
	}
	return sAppIOEventLog.u8SeqNo;
}

/*******************************************************************************
 *
 * NAME: vEEPROMTokenMaskClear
 *
 * DESCRIPTION:EEPROMトークンマスククリア
 *
 * PARAMETERS:           Name              RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEEPROMTokenMaskClear() {
	memset(sAppIO.u8MstTokenMask, 0x00, APP_AUTH_TOKEN_SIZE);
}

/*******************************************************************************
 *
 * NAME: vDateTimeUpdate
 *
 * DESCRIPTION:日時更新処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vDateTimeUpdate() {
	//===========================================================================
	// 日時・温度取得
	//===========================================================================
	// DS3231を選択
	bDS3231_deviceSelect(I2C_ADDR_DS3231);
#ifndef DEBUG
	// 時刻取得
	if (bDS3231_getDatetime(&sAppIO.sDatetime) == FALSE) {
		return;
	}
	// 現在温度取得
	if (bDS3231_getTemperature(&sAppIO.iTemperature) == FALSE) {
		return;
	}
#else
	// 時刻取得
	if (bDS3231_getDatetime(&sAppIO.sDatetime) == FALSE) {
		vfPrintf(&sSerStream, "MS:%08d DS3231 Get Date Time Error!!!\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
	// 現在温度取得
	if (bDS3231_getTemperature(&sAppIO.iTemperature) == FALSE) {
		vfPrintf(&sSerStream, "MS:%08d DS3231 Get Temperature Error!!!\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
#endif
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
	sLCDInfo.sLCDstate.u8Address = I2C_ADDR_ST7032I;
	// LCD：コントラスト
	sLCDInfo.sLCDstate.u8Contrast = LCD_CONTRAST;
	// LCD：アイコンメモリ
	memset(sLCDInfo.sLCDstate.u8IconData, (uint8)0, ST7032i_ICON_DATA_SIZE);
	// 出力バッファと制御情報
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < LCD_BUFF_ROW_SIZE; u8Idx++) {
		memset(sLCDInfo.cLCDBuff[u8Idx], '\0', LCD_BUFF_COL_SIZE + 1);
	}
	// LCDの初期化
	bST7032i_deviceSelect(&sLCDInfo.sLCDstate);
	bST7032i_init();
}

/*******************************************************************************
 *
 * NAME: vSetLCDMessage
 *
 * DESCRIPTION:LCDメッセージ書き込み処理
 *
 * PARAMETERS:      Name        RW  Usage
 *   char*          pcMsg1      R   書き込みメッセージ（１行目）
 *   char*          pcMsg2      R   書き込みメッセージ（２行目）
 *
 * RETURNS:
 *
 * NOTES:
 *
 *****************************************************************************/
PUBLIC void vLCDSetMessage(char* pcMsg1, char* pcMsg2) {
	// メッセージ書き込み
	sprintf(sLCDInfo.cLCDBuff[0], pcMsg1);
	sprintf(sLCDInfo.cLCDBuff[1], pcMsg2);
	// 終端文字の上書き
	sLCDInfo.cLCDBuff[0][LCD_BUFF_COL_SIZE] = '\0';
	sLCDInfo.cLCDBuff[1][LCD_BUFF_COL_SIZE] = '\0';
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
PUBLIC void vLCDdrawing() {
	// 終端文字の上書き
	sLCDInfo.cLCDBuff[0][LCD_BUFF_COL_SIZE] = '\0';
	sLCDInfo.cLCDBuff[1][LCD_BUFF_COL_SIZE] = '\0';
	// LCD描画
#ifndef DEBUG
	bST7032i_deviceSelect(&sLCDInfo.sLCDstate);
	bST7032i_setCursor(0, 0);
	bST7032i_writeString(sLCDInfo.cLCDBuff[0]);
	bST7032i_setCursor(1, 0);
	bST7032i_writeString(sLCDInfo.cLCDBuff[1]);
#else
	if (!bST7032i_deviceSelect(&sLCDInfo.sLCDstate)) {
		vfPrintf(&sSerStream, "1:Err.");
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
	if (!bST7032i_setCursor(0, 0)) {
		vfPrintf(&sSerStream, "2:Err.");
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
	if (!bST7032i_writeString(sLCDInfo.cLCDBuff[0])) {
		vfPrintf(&sSerStream, "3:Err.");
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
	if (!bST7032i_setCursor(1, 0)) {
		vfPrintf(&sSerStream, "4:Err.");
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
	if (!bST7032i_writeString(sLCDInfo.cLCDBuff[1])) {
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
 * PARAMETERS:          Name        RW  Usage
 *     uint32           u32DstAddr  R   宛先アドレス
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vWirelessRxEnabled(uint32 u32DstAddr) {
	// 宛先アドレス
	sWirelessInfo.u32DstAddr = u32DstAddr;
	// 受信の有効化
	sWirelessInfo.bRxEnabled = TRUE;
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
	sWirelessInfo.u32DstAddr  = 0;				// 送信先アドレス
	sWirelessInfo.bRxEnabled  = FALSE;			// 受信有効化
	sWirelessInfo.u8RxFromIdx = 0;				// 開始インデックス
	sWirelessInfo.u8RxToIdx   = 0;				// 終了インデックス
	sWirelessInfo.u8RxSize    = 0;				// バッファリングサイズ
}

/*******************************************************************************
 *
 * NAME: bWirelessRxEnq
 *
 * DESCRIPTION:無線通信によるメッセージ受信処理
 *
 * PARAMETERS:          Name        RW  Usage
 *     uint32           u32Addr     R   送信先アドレス
 *     bool_t           bEncryption R   暗号化有無
 *     uint8*           u8Data      R   送信データ
 *     uint8            u8Len       R   送信データ長
 *
 * RETURNS:
 *     TRUE:受信データバッファリング
 *
 ******************************************************************************/
PUBLIC bool_t bWirelessRxEnq(tsRxDataApp *psRx) {
	// チェック有効判定
	if (sWirelessInfo.bRxEnabled == FALSE) {
		return FALSE;
	}
	// バッファサイズ判定
	if (sWirelessInfo.u8RxSize >= RX_BUFFER_SIZE) {
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
	// 宛先アドレス
	uint32 u32ChkAddr;
	memcpy(&u32ChkAddr, psRx->auData, 4);
	if (u32ChkAddr != sWirelessInfo.u32DstAddr) {
		return FALSE;
	}
	// インデックス情報更新
	if (sWirelessInfo.u8RxSize > 0) {
		sWirelessInfo.u8RxToIdx = (sWirelessInfo.u8RxToIdx + 1) % RX_BUFFER_SIZE;
	}
	sWirelessInfo.u8RxSize++;
	// 受信メッセージ情報編集
	tsRxTxInfo* psRxInfo = &sWirelessInfo.sRxBuffer[sWirelessInfo.u8RxToIdx];
	psRxInfo->u32Addr = psRx->u32SrcAddr;
	memcpy(&psRxInfo->sMsg, psRx->auData, psRx->u8Len);
	// バッファリング完了
	return TRUE;
}

/*******************************************************************************
 *
 * NAME: bWirelessRxDeq
 *
 * DESCRIPTION:受信電文の取り出しメソッド
 *
 * PARAMETERS:        Name          RW  Usage
 *   tsRxInfo*        sRxInfo       R   受信データ情報
 *
 * RETURNS:
 *   TRUE:バッファからのフェッチ正常終了
 *
 ******************************************************************************/
PUBLIC bool_t bWirelessRxDeq(tsRxTxInfo* psRxInfo) {
	// バッファサイズ判定
	if (sWirelessInfo.u8RxSize == 0) {
		return FALSE;
	}
	// バッファからの取り出し
	*psRxInfo = sWirelessInfo.sRxBuffer[sWirelessInfo.u8RxFromIdx];
	// サイズ更新
	sWirelessInfo.u8RxSize--;
	// インデックス更新
	if (sWirelessInfo.u8RxSize > 0) {
		sWirelessInfo.u8RxFromIdx = (sWirelessInfo.u8RxFromIdx + 1) % RX_BUFFER_SIZE;
	}
	return TRUE;
}

/*******************************************************************************
 *
 * NAME: bWirelessTxEnq
 *
 * DESCRIPTION:無線通信によるメッセージ送信電文バッファリング
 *
 * PARAMETERS:          Name           RW  Usage
 *     uint32           u32SrcAddr     R   送信元アドレス
 *     bool_t           bEncryption    R   暗号化有無（事前にパスワード設定が必要？）
 *     tsWirelessMsg*   psMsg          R   送信データ
 *
 * RETURNS:
 *     TRUE:送信成功
 *
 ******************************************************************************/
PUBLIC bool_t bWirelessTxEnq(uint32 u32SrcAddr, bool_t bEncryption, tsWirelessMsg* psMsg) {
	// バッファサイズ判定
	if (sWirelessInfo.u8TxSize >= TX_BUFFER_SIZE) {
		return FALSE;
	}
	// 送信データエンキュー
	if (sWirelessInfo.u8TxSize > 0) {
		sWirelessInfo.u8TxToIdx = (sWirelessInfo.u8TxToIdx + 1) % TX_BUFFER_SIZE;
	}
	sWirelessInfo.u8TxSize++;
	// バッファ編集
	tsRxTxInfo* pTxInfo;
	pTxInfo = &sWirelessInfo.sTxBuffer[sWirelessInfo.u8TxToIdx];
	pTxInfo->u32Addr    = u32SrcAddr;	// 送信元アドレス
	pTxInfo->sMsg       = *psMsg;		// 電文
	return TRUE;
}

/*******************************************************************************
 *
 * NAME: bWirelessTxTry
 *
 * DESCRIPTION:キューイングされた電文の送信試行
 *
 * PARAMETERS:        Name          RW  Usage
 *
 * RETURNS:
 *   TRUE:送信試行
 *
 ******************************************************************************/
PUBLIC bool_t bWirelessTxTry() {
	// バッファサイズ判定
	if (sWirelessInfo.u8TxSize == 0) {
		return FALSE;
	}
	// バッファ先頭の参照
	tsRxTxInfo* psTxInfo;
	psTxInfo = &sWirelessInfo.sTxBuffer[sWirelessInfo.u8RxFromIdx];
	// 送信パケット編集
	tsTxDataApp sTx;
	memset(&sTx, 0x00, sizeof(tsTxDataApp));
	sTx.u32SrcAddr = psTxInfo->u32Addr;				// 送信元アドレス
	sTx.u32DstAddr = TOCONET_MAC_ADDR_BROADCAST;	// 送信先アドレス（ブロードキャスト）
	sTx.u8Cmd = TOCONET_PACKET_CMD_APP_DATA;		// パケットタイプ
	sTx.u8Seq = sWirelessInfo.u8TxSeq++;			// フレームシーケンス
	sTx.u8Len = (uint8)sizeof(tsWirelessMsg);		// 送信データ長
	sTx.u8CbId = sTx.u8Seq;							// コールバックID（フレームシーケンス指定）
	memcpy(sTx.auData, &psTxInfo->sMsg, sTx.u8Len);	// 送信データ
	sTx.bAckReq = FALSE;							// ACK返信有無
	sTx.bSecurePacket = FALSE;						// 暗号化設定（未実装なのでTRUEにすると暗号化送信のふりをする）
	sTx.u8Retry = 0x81;								// 再送回数（再送しない）
	sTx.u16DelayMin = 0;							// 遅延送信時間（最小）
	sTx.u16DelayMax = 0;							// 遅延送信時間（最大）
	sTx.u16RetryDur = 0;							// 再送間隔（ミリ秒単位）
	// 送信処理
	return ToCoNet_bMacTxReq(&sTx);
}

/*******************************************************************************
 *
 * NAME: bWirelessTxComplete
 *
 * DESCRIPTION:キューイングされた電文の送信完了処理
 *
 * PARAMETERS:        Name          RW  Usage
 *       uint8        u8CbId        R   送信パケットのコールバックID
 *       uint8        u8Status      R   送信ステータス、(u8Status & 0x01)が1:成功、0:失敗
 *
 * RETURNS:
 *   TRUE:送信試行
 *
 ******************************************************************************/
PUBLIC bool_t bWirelessTxComplete(uint8 u8CbId, uint8 u8Status) {
	// 送信完了判定
	if ((u8Status & 0x01) == 0x00) {
		return FALSE;
	}
	// コールバックIDチェック
	if (u8CbId != (sWirelessInfo.u8TxSeq - sWirelessInfo.u8TxSize)) {
		return FALSE;
	}
	// 送信キューの更新
	sWirelessInfo.u8TxSize--;
	if (sWirelessInfo.u8TxSize > 0) {
		sWirelessInfo.u8RxFromIdx = (sWirelessInfo.u8RxFromIdx + 1) % TX_BUFFER_SIZE;
	}
	// 送信完了
	return TRUE;
}

/*******************************************************************************
 *
 * NAME: bvUpdServoSetting
 *
 * DESCRIPTION:サーボ設定の更新
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   uint8 0:値更新なし 1:サーボ設定A更新 2:サーボ設定B更新 3:サーボ設定A&B更新
 *
 ******************************************************************************/
PUBLIC uint8 u8UpdServoSetting() {
	// 返却値
	uint8 u8Result = 0;
	// バッファ更新：サーボ設定A
	vIOUtil_adcUpdateBuffer(&sAppIO.sServoSetABuffer);
	int iWkVal = iIOUtil_adcReadBuffer(&sAppIO.sServoSetABuffer, ADC_CHK_TOLERANCE, ADC_BUFF_CHK_CNT);
	int iFrom = iWkVal - ADC_CHK_TOLERANCE;
	int iTo = iWkVal + ADC_CHK_TOLERANCE;
	if (iWkVal >= 0 && (sAppIO.iServoSetA < iFrom || sAppIO.iServoSetA > iTo)) {
		sAppIO.iServoSetA = iWkVal;
		u8Result = 1;
	}
	// バッファ更新：サーボ設定B
	vIOUtil_adcUpdateBuffer(&sAppIO.sServoSetBBuffer);
	iWkVal = iIOUtil_adcReadBuffer(&sAppIO.sServoSetBBuffer, ADC_CHK_TOLERANCE, ADC_BUFF_CHK_CNT);
	iFrom = iWkVal - ADC_CHK_TOLERANCE;
	iTo = iWkVal + ADC_CHK_TOLERANCE;
	if (iWkVal >= 0 && (sAppIO.iServoSetB < iFrom || sAppIO.iServoSetB > iTo)) {
		sAppIO.iServoSetB = iWkVal;
		u8Result = u8Result + 2;
	}
	// 返却値
	return u8Result;
}

/*******************************************************************************
 *
 * NAME: vUpdServoPos
 *
 * DESCRIPTION:サーボ位置の更新
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vUpdServoPos() {
	// サーボポジション記録
	int iSrvPos;
	do {
		vIOUtil_adcUpdateBuffer(&sAppIO.sServoPosBuffer);
		iSrvPos = iIOUtil_adcReadBuffer(&sAppIO.sServoPosBuffer, ADC_CHK_TOLERANCE, ADC_BUFF_CHK_CNT);
	} while (iSrvPos < 0);
	sAppIO.iServoPos = iSrvPos;
}

/*******************************************************************************
 *
 * NAME: bServoPosChange
 *
 * DESCRIPTION:サーボ位置の移動判定
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   bool_t TRUE:サーボ位置変更
 *
 ******************************************************************************/
PUBLIC bool_t bServoPosChange() {
	int iSrvPos = iIOUtil_adcReadBuffer(&sAppIO.sServoPosBuffer, ADC_CHK_TOLERANCE, ADC_BUFF_CHK_CNT);
	if (iSrvPos < 0) {
		return FALSE;
	}
	int iFrom = sAppIO.iServoPos - ADC_CHK_TOLERANCE;
	int iTo   = sAppIO.iServoPos + ADC_CHK_TOLERANCE;
	return (iSrvPos < iFrom || iSrvPos > iTo);
}

/*******************************************************************************
 *
 * NAME: vSetServoLock
 *
 * DESCRIPTION:サーボ制御（ロック）
 *
 * PARAMETERS:          Name        RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vSetServoLock() {
	// 5V電源ON
	v5VPwOn();
	// サーボ位置を補正
	uint32 u32AngleSec = (uint32)((sAppIO.iServoSetB * 648000) / 1023);
	vPWMUtil_servoSelect(E_AHI_TIMER_4, E_PWM_UTIL_SERVO_DEFAULT, TRUE, FALSE);
	bPWMUtil_servoAngleSec(u32AngleSec);
}

/*******************************************************************************
 *
 * NAME: vSetServoUnlock
 *
 * DESCRIPTION:サーボ制御（アンロック）
 *
 * PARAMETERS:          Name        RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vSetServoUnlock() {
	// 5V電源ON
	v5VPwOn();
	// サーボ位置を補正
	uint32 u32AngleSec = (uint32)((sAppIO.iServoSetA * 648000) / 1023);
	vPWMUtil_servoSelect(E_AHI_TIMER_4, E_PWM_UTIL_SERVO_DEFAULT, TRUE, FALSE);
	bPWMUtil_servoAngleSec(u32AngleSec);
}

/*******************************************************************************
 *
 * NAME: vServoControlOff
 *
 * DESCRIPTION:サーボ制御（出力オフ）
 *
 * PARAMETERS:          Name        RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vServoControlOff() {
	vAHI_TimerDisable(E_AHI_TIMER_4);
}

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

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

/*******************************************************************************
 *
 * NAME: iEEPROMRemoteInfoInit
 *
 * DESCRIPTION:EEPROM読み込み（リモートデバイス情報インデックスのキャッシュ）
 *
 * PARAMETERS:             Name         RW  Usage
 *
 * RETURNS:
 *   int 最終読み込みレコードインデックス、エラー時はマイナス値
 *
 ******************************************************************************/
PRIVATE int iEEPROMRemoteInfoInit() {
	//==========================================================================
	// マスタートークンマスクを生成
	//==========================================================================
#ifndef DEBUG
	vValUtil_setU8RandArray(sAppIO.u8MstTokenMask, APP_AUTH_TOKEN_SIZE);
#endif

	//==========================================================================
	// リモートデバイス情報のキャッシュ編集とマスキング
	//==========================================================================
	// 構造体の初期化
	memset(&sAppIOIndexCache, 0x00, sizeof(tsAppIOIndexCache));
	// リモートデバイス情報の読み込み
	bEEPROM_deviceSelect(&sEEPROM_status);
	tsAuthRemoteDevInfo wkInfo;
	uint32 u32Size = sizeof(tsAuthRemoteDevInfo);
	uint8 u8Idx = 0;
	while(u8Idx < MAX_REMOTE_DEV_CNT) {
		// レコード有無判定
		if ((sIndexInfo.u32RemoteDevMap & (0x01 << u8Idx)) == 0) {
			u8Idx++;
			continue;
		}
		// レコード読み込み
		if (bEEPROM_readData(TOP_ADDR_REMOTE_DEV + u32Size * u8Idx, u32Size, (uint8*)&wkInfo) == FALSE) {
			// ウェイト
			u32TimerUtil_waitTickUSec(200);
			// リトライ
			continue;
		}
		// ローカルインデックスキャッシュ更新
		sAppIOIndexCache.u8RemoteDevIdx[sAppIOIndexCache.u8RemoteDevCnt] = (uint8)u8Idx;
		sAppIOIndexCache.u32RemoteDevID[sAppIOIndexCache.u8RemoteDevCnt] = wkInfo.u32DeviceID;
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d iEEPROMRemoteInfoInit Idx:%02d ID:%010d\n", u32TickCount_ms,
			sAppIOIndexCache.u8RemoteDevIdx[sAppIOIndexCache.u8RemoteDevCnt],
			sAppIOIndexCache.u32RemoteDevID[sAppIOIndexCache.u8RemoteDevCnt]);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
		sAppIOIndexCache.u8RemoteDevCnt++;
		// リモートデバイス情報の書き込み
		iEEPROMWriteRemoteInfo(&wkInfo);
		// インデックス更新
		u8Idx++;
	}
	// 最終読み込みインデックス
	return u8Idx - 1;
}

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
