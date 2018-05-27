/*******************************************************************************
 *
 * MODULE :Application IO functions source file
 *
 * CREATED:2018/05/07 21:30:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:画面表示やキー入力等の基本的な入出力機能のヘッダファイル
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 *******************************************************************************
 * Copyright (c) 2018, Nakanohito
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
#include "ds3231.h"
#include "eeprom.h"
#include "adxl345.h"
#include "s11059_02dt.h"
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
	u32PinMapInput |= PIN_MAP_IR_SENS;		// Pin Map:人感センサー
	u32PinMapInput |= PIN_MAP_DOPPLER_SENS;	// Pin Map:ドップラーセンサー
	// DIOピンの入出力を設定
	vAHI_DioSetDirection(u32PinMapInput, 0x00);
	// プルアップ・プルダウン設定
	uint32 u32PinMapPullDown = 0x00;
	u32PinMapPullDown |= PIN_MAP_IR_SENS;
	u32PinMapPullDown |= PIN_MAP_DOPPLER_SENS;
	vAHI_DioSetPullup(0x00, u32PinMapPullDown);

	//==========================================================================
	// 入出力情報の初期化
	//==========================================================================
	sAppIO.u32DiMap    = 0;			// デジタル入力ピンマップ
	sAppIO.u32DiMapBef = 0;			// デジタル入力ピンマップ（前回分）
	// カラーデータバッファを初期化
	memset(sAppIO.sColorBuffer, 0x00, sizeof(tsS11059_02DT_data) * COLOR_BUFFER_SIZE);
	// マスタートークンマスク
	memset(sAppIO.u8MstTokenMask, 0x00, APP_AUTH_TOKEN_SIZE);

	//==========================================================================
	// I2Cデバイス初期化
	//==========================================================================
	// EEPROM初期処理
	vEEPROMInit();
#ifdef DEBUG
	// デバイス情報の読み込み
	if (bEEPROMReadDevInfo() != TRUE) {
		vfPrintf(&sSerStream, "MS:%08d Read DevInfo Err\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// インデックスの読み込み
	if (bEEPROMReadIndexInfo(&sIndexInfo) != TRUE) {
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
	// 加速度センサー初期処理
	if (bAccelerationInit() != TRUE) {
		vfPrintf(&sSerStream, "MS:%08d Init Acceleration Sensor Err\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// カラーセンサー初期処理
	if (bColorSensInit() != TRUE) {
		vfPrintf(&sSerStream, "MS:%08d Init Color Sensor Err\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
#else
	// デバイス情報の読み込み
	bEEPROMReadDevInfo();
	// インデックスの読み込み
	bEEPROMReadIndexInfo(&sIndexInfo);
	// リモートデバイス情報の初期処理
	iEEPROMRemoteInfoInit();
	// 日時温度更新
	vDateTimeUpdate();
	// 加速度センサー初期処理
	bAccelerationInit();
	// カラーセンサー初期処理
	bColorSensInit();
#endif
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
	// デジタル入力バッファ更新
	vIOUtil_diUpdateBuffer();
	// 入力ピンマップを取得
	uint32 u32PinMap = PIN_MAP_IR_SENS| PIN_MAP_DOPPLER_SENS;
	uint32 u32WkVal = u32IOUtil_diReadBuffer(u32PinMap, DI_BUFF_CHK_CNT);
	sAppIO.u32DiMapBef = sAppIO.u32DiMap;
	sAppIO.u32DiMap = u32WkVal;
#ifndef DEBUG
	// 加速度センサー
	bUpdAxesBuffer();
	// カラーセンサー
	bUpdColorBuffer();
#else
	// 加速度センサー
	if (bUpdAxesBuffer() != TRUE) {
//		vfPrintf(&sSerStream, "MS:%08d bUpdAxesBuffer Error!!!\n", u32TickCount_ms);
//		SERIAL_vFlush(sSerStream.u8Device);
	}
	// カラーセンサー
	if (bUpdColorBuffer() == TRUE) {
//		tsS11059_02DT_data* spData = &sAppIO.sColorBuffer[sAppIO.u8ColorBuffIdx];
//		vfPrintf(&sSerStream, "MS:%08d vUpdInputBuffer %04X:%04X:%04X:%04X\n", u32TickCount_ms,
//				spData->u16DataRed, spData->u16DataGreen, spData->u16DataBlue, spData->u16DataInfrared);
//		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "MS:%08d vUpdInputBuffer Error!!!\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
#endif
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
	if (bDS3231_getDatetime(&sAppIO.sDatetime) != TRUE) {
		vfPrintf(&sSerStream, "MS:%08d DS3231 Get Date Time Error!!!\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
	// 現在温度取得
	if (bDS3231_getTemperature(&sAppIO.iTemperature) != TRUE) {
		vfPrintf(&sSerStream, "MS:%08d DS3231 Get Temperature Error!!!\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
#endif
}

/*******************************************************************************
 *
 * NAME: bHighTemperature
 *
 * DESCRIPTION:高温判定
 *
 * PARAMETERS:        Name          RW  Usage
 *
 * RETURNS:
 *   TRUE:加速検知
 *
 ******************************************************************************/
PUBLIC bool_t bHighTemperature() {
	return (sAppIO.iTemperature >= TEMPERATURE_THRESHOLD);
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
	if (psRx->u8Len != TX_REC_SIZE) {
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
	sTx.u8Len = TX_REC_SIZE;						// 送信データ長
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
 * NAME: vAccelerationInit
 *
 * DESCRIPTION:加速度センサー初期処理
 *
 * PARAMETERS:        Name          RW  Usage
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC bool_t bAccelerationInit() {
	// I2Cアドレス設定
	sAppIO.sAccelerationInfo.u8DeviceAddress = I2C_DEVICE_ACCELERATION;
	// カラーバッファインデックス
	sAppIO.u8AxesBuffIdx = AXES_BUFFER_SIZE - 1;
	// デバイス選択
	bADXL345_deviceSelect(&sAppIO.sAccelerationInfo);
	// センサー初期化処理
	return bADXL345_init(0x06);
}

/*******************************************************************************
 *
 * NAME: bUpdAxesBuffer
 *
 * DESCRIPTION:加速度データバッファの更新
 *
 * PARAMETERS:        Name          RW  Usage
 *   TRUE:バッファ更新成功
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC bool_t bUpdAxesBuffer() {
	// レジスタ情報読み込み
	if (bADXL345_readRegister() != TRUE) {
		return FALSE;
	}
	// 加速度データ読み込み
	sAppIO.u8AxesBuffIdx = (sAppIO.u8AxesBuffIdx + 1) % AXES_BUFFER_SIZE;
	sAppIO.sAxesData[sAppIO.u8AxesBuffIdx] = sADXL345_getGData();
#ifdef DEBUG
//	tsADXL345_AxesData sData = sAppIO.sAxesData[sAppIO.u8AxesBuffIdx];
//	vfPrintf(&sSerStream, "MS:%08d bUpdAxesBuffer %05d %05d %05d\n", u32TickCount_ms,
//			sData.i16DataX, sData.i16DataY, sData.i16DataZ);
//	SERIAL_vFlush(sSerStream.u8Device);
#endif
	// 更新完了
	return TRUE;
}

/*******************************************************************************
 *
 * NAME: bAccelerating
 *
 * DESCRIPTION:加速度検知判定
 *
 * PARAMETERS:        Name          RW  Usage
 *
 * RETURNS:
 *   TRUE:加速検知
 *
 ******************************************************************************/
PUBLIC bool_t bAccelerating() {
	// 各軸について直前の移動平均値を算出
	tsADXL345_AxesData* sData;
	int16 i16ValX = 0;
	int16 i16ValY = 0;
	int16 i16ValZ = 0;
	uint8 u8BuffIdx = sAppIO.u8AxesBuffIdx;
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < (AXES_BUFFER_SIZE - 1); u8Idx++) {
		u8BuffIdx = (u8BuffIdx + 1) % AXES_BUFFER_SIZE;
		sData = &sAppIO.sAxesData[u8BuffIdx];
		i16ValX = i16ValX + sData->i16DataX;
		i16ValY = i16ValY + sData->i16DataY;
		i16ValZ = i16ValZ + sData->i16DataZ;
	}
	i16ValX = i16ValX / (AXES_BUFFER_SIZE - 1);
	i16ValY = i16ValY / (AXES_BUFFER_SIZE - 1);
	i16ValZ = i16ValZ / (AXES_BUFFER_SIZE - 1);
	// 変化量を算出
	sData = &sAppIO.sAxesData[sAppIO.u8AxesBuffIdx];
	uint16 u16Change;
	if (sData->i16DataX > i16ValX) {
		u16Change = sData->i16DataX - i16ValX;
	} else {
		u16Change = i16ValX - sData->i16DataX;
	}
	if (sData->i16DataY > i16ValY) {
		u16Change = u16Change + sData->i16DataY - i16ValY;
	} else {
		u16Change = u16Change + i16ValY - sData->i16DataY;
	}
	if (sData->i16DataZ > i16ValZ) {
		u16Change = u16Change + sData->i16DataZ - i16ValZ;
	} else {
		u16Change = u16Change + i16ValZ - sData->i16DataZ;
	}
	// 変化量を判定
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d bAccelerating Chg:%d\n", u32TickCount_ms, u16Change);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	return (u16Change > AXES_THRESHOLD);
}

/*******************************************************************************
 *
 * NAME: vColorSensInit
 *
 * DESCRIPTION:カラーセンサー初期処理
 *
 * PARAMETERS:        Name          RW  Usage
 *
 * RETURNS:
 *   TRUE:初期化成功
 *
 ******************************************************************************/
PUBLIC bool_t bColorSensInit() {
	// カラーバッファインデックス
	sAppIO.u8ColorBuffIdx = COLOR_BUFFER_SIZE - 1;
	// I2Cアドレス設定
	sAppIO.sColorState.u8Address = I2C_DEVICE_COLOR;
	// ゲイン選択
	sAppIO.sColorState.bGainSelection = S11059_02DT_HIGH_GAIN;
	// 積分モード
	sAppIO.sColorState.bIntegralMode = S11059_02DT_FIXED_MODE;
	// 積分時間
	sAppIO.sColorState.u8IntegralTime = S11059_02DT_UNIT_TIME_2;
	// デバイス選択
	if (bS11059_02DT_deviceSelect(&sAppIO.sColorState) != TRUE){
		return FALSE;
	}
	// 計測開始
	return bS11059_02DT_start();
}

/*******************************************************************************
 *
 * NAME: vUpdColorBuffer
 *
 * DESCRIPTION:カラーデータバッファの更新
 *
 * PARAMETERS:        Name          RW  Usage
 *
 * RETURNS:
 *   TRUE:バッファ更新成功
 *
 ******************************************************************************/
PUBLIC bool_t bUpdColorBuffer() {
	// カラーデータ読み込み
	uint8 u8Idx = (sAppIO.u8ColorBuffIdx + 1) % COLOR_BUFFER_SIZE;
	if (bS11059_02DT_readData(&sAppIO.sColorBuffer[u8Idx]) != TRUE) {
		return FALSE;
	}
	// カラーバッファインデックス更新
	sAppIO.u8ColorBuffIdx = u8Idx;
	return TRUE;
}

/*******************************************************************************
 *
 * NAME: bColorChange
 *
 * DESCRIPTION:カラーセンサー検知判定
 *
 * PARAMETERS:        Name          RW  Usage
 *
 * RETURNS:
 *   TRUE:カラーセンサー検知
 *
 ******************************************************************************/
PUBLIC bool_t bColorChange() {
	// ２サイクル前までの計測結果を積算
	uint32 u32DataRed      = 0;		// センサーのデータ（Red）
	uint32 u32DataGreen    = 0;		// センサーのデータ（Green）
	uint32 u32DataBlue     = 0;		// センサーのデータ（Blue）
	uint32 u32DataInfrared = 0;		// センサーのデータ（Infrared）
	uint16 u16BuffIdx = sAppIO.u8ColorBuffIdx;
	uint16 u16Idx;
	for (u16Idx = 0; u16Idx < (COLOR_BUFFER_SIZE - 2); u16Idx++) {
		u16BuffIdx = (u16BuffIdx + 1) % COLOR_BUFFER_SIZE;
		// センサーのデータ（Red）
		u32DataRed = u32DataRed + sAppIO.sColorBuffer[u16BuffIdx].u16DataRed;
		// センサーのデータ（Green）
		u32DataGreen = u32DataGreen + sAppIO.sColorBuffer[u16BuffIdx].u16DataGreen;
		// センサーのデータ（Blue）
		u32DataBlue = u32DataBlue + sAppIO.sColorBuffer[u16BuffIdx].u16DataBlue;
		// センサーのデータ（Infrared）
		u32DataInfrared = u32DataInfrared + sAppIO.sColorBuffer[u16BuffIdx].u16DataInfrared;
	}
	// 平均値を算出（端数切捨て）
	u32DataRed      = u32DataRed / (COLOR_BUFFER_SIZE - 2);			// センサーのデータ（Red）
	u32DataGreen    = u32DataGreen / (COLOR_BUFFER_SIZE - 2);		// センサーのデータ（Green）
	u32DataBlue     = u32DataBlue / (COLOR_BUFFER_SIZE - 2);		// センサーのデータ（Blue）
	u32DataInfrared = u32DataInfrared / (COLOR_BUFFER_SIZE - 2);	// センサーのデータ（Infrared）
	// 色相の変化量を算出
	tsS11059_02DT_data* sData =
			&sAppIO.sColorBuffer[(sAppIO.u8ColorBuffIdx + COLOR_BUFFER_SIZE - 1) % COLOR_BUFFER_SIZE];
	uint32 u32Change;
	// 赤の変化量
	if (u32DataRed > sData->u16DataRed) {
		u32Change = u32DataRed - sData->u16DataRed;
	} else {
		u32Change = sData->u16DataRed - u32DataRed;
	}
	// 緑の変化量
	if (u32DataGreen > sData->u16DataGreen) {
		u32Change = u32Change + u32DataGreen - sData->u16DataGreen;
	} else {
		u32Change = u32Change + sData->u16DataGreen - u32DataGreen;
	}
	// 青の変化量
	if (u32DataBlue > sData->u16DataBlue) {
		u32Change = u32Change + u32DataBlue - sData->u16DataBlue;
	} else {
		u32Change = u32Change + sData->u16DataBlue - u32DataBlue;
	}
	// 赤外線の変化量
	if (u32DataInfrared > sData->u16DataInfrared) {
		u32Change = u32Change + u32DataInfrared - sData->u16DataInfrared;
	} else {
		u32Change = u32Change + sData->u16DataInfrared - u32DataInfrared;
	}
	// 色相の変化量を判定
#ifdef DEBUG
//	vfPrintf(&sSerStream, "MS:%08d bColorChange %05d\n", u32TickCount_ms, u32Change);
//	SERIAL_vFlush(sSerStream.u8Device);
#endif
	return (u32Change > COLOR_THRESHOLD);
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
	int iIdx = 0;
	while(iIdx < MAX_REMOTE_DEV_CNT) {
		// レコード有無判定
		if ((sIndexInfo.u32RemoteDevMap & (0x01 << iIdx)) == 0) {
			iIdx++;
			continue;
		}
		// レコード読み込み
		if (bEEPROM_readData(TOP_ADDR_REMOTE_DEV + u32Size * iIdx, u32Size, (uint8*)&wkInfo) == FALSE) {
			// ウェイト
			u32TimerUtil_waitTickUSec(200);
			// リトライ
			continue;
		}
		// ローカルインデックスキャッシュ更新
		sAppIOIndexCache.u8RemoteDevIdx[sAppIOIndexCache.u8RemoteDevCnt] = (uint8)iIdx;
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
		iIdx++;
	}
	// 最終読み込みインデックス
	return iIdx - 1;
}

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
