/****************************************************************************
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
 ****************************************************************************
 * Copyright (c) 2016, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <jendefs.h>
#include <AppHardwareApi.h>

/****************************************************************************/
/***        ToCoNet Definitions                                           ***/
/****************************************************************************/
#include "serial.h"
#include "sprintf.h"

/****************************************************************************/
/***        ToCoNet Include files                                         ***/
/****************************************************************************/
#include "ToCoNet_use_mod.h"
#include "ToCoNet.h"
#include "ToCoNet_mod_prototype.h"
#include "ccitt8.h"

/****************************************************************************/
/***        User Include files                                            ***/
/****************************************************************************/
#include "config.h"
#include "config_default.h"
#include "framework.h"
#include "timer_util.h"
#include "i2c_util.h"
#include "io_util.h"
#include "value_util.h"
#include "keypad.h"
#include "st7032i.h"
#include "eeprom.h"
#include "melody.h"
#include "app_io.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
// LCDのカラム制御情報更新処理
PRIVATE bool_t bLCDUpdCursorCntr();

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE const char APP_IO_USE_STRING[] =
	"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-/*!#$%&()?_@";
/** 送信データフレームシーケンス */
PRIVATE uint8 u8TxSeq = 0;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vAppIOInit
 *
 * DESCRIPTION:入出力ハードウェアの初期処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vAppIOInit() {
	// 入出力情報初期化
	memset(&sAppIO, 0x00, sizeof(tsAppIO));
	// ローカルEEPROM初期処理
	vLocalEEPROMInit();
	// キーパッドの初期化
	vKeyPadInit();
	// メロディ演奏機能の初期処理
	vMelodyInit();
	// 主I2Cバス選択
	vI2CMainConnect();
	// I2C接続EEPROM初期化処理
	vEEPROMInit();
	// デバイス情報の読み込み
	tsAuthDeviceInfo sDevInfo;
	if (bEEPROMReadDevInfo(&sDevInfo) == FALSE) {
		memset(&sDevInfo, 0x00, sizeof(tsAuthDeviceInfo));
	}
	// 無線通信初期処理
	vWirelessInit(&sDevInfo);
	// LCD初期化処理
	vLCDInit();
}

/****************************************************************************
 *
 * NAME: vKeyPadInit
 *
 * DESCRIPTION:キーパッドの初期化処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vKeyPadInit() {
	tsKEYPAD_status* psKEYPAD_status;
	psKEYPAD_status = &sAppIO.sKEYPAD_status;
	// DIOピンの入出力を設定
	vAHI_DioSetDirection(0xFFFFFFFF, 0x00);
	psKEYPAD_status->u8PinCols[0] = 5;
	psKEYPAD_status->u8PinCols[1] = 18;
	psKEYPAD_status->u8PinCols[2] = 19;
	psKEYPAD_status->u8PinCols[3] = 8;
	psKEYPAD_status->u8PinRows[0] = 9;
	psKEYPAD_status->u8PinRows[1] = 10;
	psKEYPAD_status->u8PinRows[2] = 11;
	psKEYPAD_status->u8PinRows[3] = 13;
	vKEYPAD_deviceSelect(psKEYPAD_status);
	vKEYPAD_configSet();
}

/*****************************************************************************
 *
 * NAME: vUpdateKeyPadBuff
 *
 * DESCRIPTION:キーパッド（４×４）のバッファ更新処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vUpdateKeyPadBuff() {
	vKEYPAD_deviceSelect(&sAppIO.sKEYPAD_status);
	bKEYPAD_updateBuffer();
}

/*****************************************************************************
 *
 * NAME: u8ReadKeyPad
 *
 * DESCRIPTION:キーパッド（４×４）の読み込み
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      uint8 入力された文字を返却、入力無しの場合にはNULLを返す
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint8 u8ReadKeyPad() {
	vKEYPAD_deviceSelect(&sAppIO.sKEYPAD_status);
	return u8KEYPAD_readLast();
}

/*****************************************************************************
 *
 * NAME: vI2CMainConnect
 *
 * DESCRIPTION:主I2C接続判定
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vI2CMainConnect() {
	// USB接続
	vAHI_DioSetDirection(0x00, I2C_BUS_01 | I2C_BUS_02);
	vAHI_DioSetOutput(I2C_BUS_01, I2C_BUS_02);
}

/*****************************************************************************
 *
 * NAME: vI2CSubConnect
 *
 * DESCRIPTION:副I2C接続判定
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vI2CSubConnect() {
	// USB接続
	vAHI_DioSetDirection(0x00, I2C_BUS_01 | I2C_BUS_02);
	vAHI_DioSetOutput(I2C_BUS_02, I2C_BUS_01);
}

/****************************************************************************
 *
 * NAME: u8I2CDeviceTypeChk
 *
 * DESCRIPTION:I2C接続デバイスタイプチェック
 *
 * PARAMETERS:      Name            RW  Usage
 * DS3231_datetime* psDatetime      RW  時刻情報
 *
 * RETURNS:
 *   uint8 デバイスタイプマップ
 *
 ****************************************************************************/
PUBLIC uint8 u8I2CDeviceTypeChk(DS3231_datetime* psDatetime) {
	// I2Cアクセスする事で接続状態を判定
	uint8 u8DevType = 0x00;
	// EEPROM
	uint8 u8Buff[1];
	bEEPROM_deviceSelect(&sAppIO.sEEPROM_status);
	if (bEEPROM_readData(0, 1, u8Buff)) {
		u8DevType = I2C_DEVICE_EEPROM;
	}
	// DS3231
	bDS3231_deviceSelect(I2C_ADDR_DS3231);
	if (bDS3231_getDatetime(psDatetime)) {
		u8DevType = u8DevType | I2C_DEVICE_RTC;
	}
	return u8DevType;
}

/****************************************************************************
 *
 * NAME: vLocalEEPROMInit
 *
 * DESCRIPTION:内臓EEPROM初期処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   bool_t  TRUE:初期化成功
 *
 ****************************************************************************/
PUBLIC void vLocalEEPROMInit() {
	// 内臓EEPROMのデバイス初期化
	uint8 u8SegmentSize = 0;
	u16AHI_InitialiseEEP(&u8SegmentSize);
}

/****************************************************************************
 *
 * NAME: bLocalEEPROMRead
 *
 * DESCRIPTION:内臓EEPROM書き込み
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   bool_t TRUE:書き込み成功
 *
 ****************************************************************************/
PUBLIC bool_t bLocalEEPROMWrite() {
	// レコードを編集
	uint8 u8Contrast[1];
	u8Contrast[0] = sLCDInfo.sLCDstate.u8Contrast;
	// EEPROMへの書き込み
	return (iAHI_WriteDataIntoEEPROMsegment(0, 0, u8Contrast, 1) == 0);
}

/****************************************************************************
 *
 * NAME: vEEPROMInit
 *
 * DESCRIPTION:EEPROM初期化
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vEEPROMInit() {
	// EEPROMの初期化
	sAppIO.sEEPROM_status.u8DevAddress  = I2C_ADDR_EEPROM_0;
	sAppIO.sEEPROM_status.b2ByteAddrFlg = TRUE;
	sAppIO.sEEPROM_status.u8PageSize    = EEPROM_PAGE_SIZE_64B;
	sAppIO.sEEPROM_status.u64LastWrite  = u64TimerUtil_readUsec();
}

/****************************************************************************
 *
 * NAME: bEEPROMReadDevInfo
 *
 * DESCRIPTION:EEPROM読み込み（デバイス情報）
 *
 * PARAMETERS:          Name         RW  Usage
 *   tsAuthDeviceInfo*  psDevInfo    R   デバイス情報
 *
 * RETURNS:
 *   bool_t TRUE:読み込み成功
 *
 ****************************************************************************/
PUBLIC bool_t bEEPROMReadDevInfo(tsAuthDeviceInfo *psDevInfo) {
	// I2C EEPROM Read
	bEEPROM_deviceSelect(&sAppIO.sEEPROM_status);
	return bEEPROM_readData(TOP_ADDR_DEV, sizeof(tsAuthDeviceInfo), (uint8 *)psDevInfo);
}

/****************************************************************************
 *
 * NAME: bEEPROMWriteDevInfo
 *
 * DESCRIPTION:EEPROM書き込み（デバイス情報）
 *
 * PARAMETERS:          Name         RW  Usage
 *   tsAuthDeviceInfo*  psDevInfo    R   デバイス情報
 *
 * RETURNS:
 *   bool_t TRUE:読み込み成功
 *
 ****************************************************************************/
PUBLIC bool_t bEEPROMWriteDevInfo(tsAuthDeviceInfo *psDevInfo) {
	// I2C EEPROM Write
	bEEPROM_deviceSelect(&sAppIO.sEEPROM_status);
	return bEEPROM_writeData(TOP_ADDR_DEV, sizeof(tsAuthDeviceInfo), (uint8 *)psDevInfo);
}

/****************************************************************************
 *
 * NAME: bEEPROMReadIndexInfo
 *
 * DESCRIPTION:EEPROM読み込み（インデックス情報）
 *
 * PARAMETERS:          Name         RW  Usage
 *   tsAppIOIndexInfo*  psIndexInfo  R   インデックス情報
 *
 * RETURNS:
 *   bool_t TRUE:読み込み成功
 *
 ****************************************************************************/
PUBLIC bool_t bEEPROMReadIndexInfo(tsAppIOIndexInfo *psIndexInfo) {
	// I2C EEPROM Read
	bEEPROM_deviceSelect(&sAppIO.sEEPROM_status);
	bool_t bResult = bEEPROM_readData(TOP_ADDR_INDEX, sizeof(tsAppIOIndexInfo), (uint8 *)psIndexInfo);
	// レコード有効チェック
	if (psIndexInfo->u8EnableCheck == 0xAA) {
		return bResult;
	}
	// レコード初期化
	psIndexInfo->u8EnableCheck   = 0xAA;
	psIndexInfo->u32RemoteDevMap = 0x00;
	psIndexInfo->u8EventLogCnt   = 0;
	return bEEPROMWriteIndexInfo(psIndexInfo);
}

/****************************************************************************
 *
 * NAME: bEEPROMWriteIndexInfo
 *
 * DESCRIPTION:EEPROM書き込み（インデックス情報）
 *
 * PARAMETERS:          Name         RW  Usage
 *   tsAppIOIndexInfo*  psIndexInfo  R   インデックス情報
 *
 * RETURNS:
 *   bool_t TRUE:読み込み成功
 *
 ****************************************************************************/
PUBLIC bool_t bEEPROMWriteIndexInfo(tsAppIOIndexInfo *psIndexInfo) {
	// I2C EEPROM Write
	bEEPROM_deviceSelect(&sAppIO.sEEPROM_status);
	return bEEPROM_writeData(TOP_ADDR_INDEX, sizeof(tsAppIOIndexInfo), (uint8 *)psIndexInfo);
}


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
PUBLIC int iEEPROMIndexOfRemoteInfo(uint32 u32DeviceID) {
	// インデックス情報読み込み
	if (!bEEPROMReadIndexInfo(&sIndexInfo)) {
		return -1;
	}
	// リモートデバイス情報の探索
	bEEPROM_deviceSelect(&sAppIO.sEEPROM_status);
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

/****************************************************************************
 *
 * NAME: iEEPROMReadRemoteInfo
 *
 * DESCRIPTION:EEPROM読み込み（リモートデバイス情報）
 *
 * PARAMETERS:             Name         RW  Usage
 * tsAuthRemoteDevInfo*    psDevInfo    R   リモートデバイス情報
 * uint8                   u8Idx        R   探索開始インデックス
 * bool_t                  bForwardFlg  R   順方向探索フラグ
 *
 * RETURNS:
 *   int 読み込みレコードインデックス、エラー時はマイナス値(-1:Index読み込みエラー、-2:対象データなし、-3:読み込みエラー)
 *
 ****************************************************************************/
PUBLIC int iEEPROMReadRemoteInfo(tsAuthRemoteDevInfo *psRemoteInfo, uint8 u8Idx, bool_t bForwardFlg) {
	// インデックス情報読み込み
	if (!bEEPROMReadIndexInfo(&sIndexInfo)) {
		return -1;
	}
	// リモートデバイス情報のインデックス探索
	uint8 u8Add;
	if (bForwardFlg) {
		u8Add = 1;
	} else {
		u8Add = MAX_REMOTE_DEV_CNT - 1;
	}
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
	bEEPROM_deviceSelect(&sAppIO.sEEPROM_status);
	if (!bEEPROM_readData(u32Addr, u32Size, (uint8 *)psRemoteInfo)) {
		return -3;
	}
	return u8ChkIdx;
}

/****************************************************************************
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
 ****************************************************************************/
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
	bEEPROM_deviceSelect(&sAppIO.sEEPROM_status);
	if (!bEEPROM_writeData(u32Addr, u32Size, (uint8 *)psRemoteInfo)) {
		return -3;
	}
	return iIdx;
}

/****************************************************************************
 *
 * NAME: iEEPROMDeleteRemoteInfo
 *
 * DESCRIPTION:EEPROM削除（リモートデバイス情報）
 *
 * PARAMETERS:             Name         RW  Usage
 * uint32                  u32DeviceID  R   対象レコードのデバイスID
 *
 * RETURNS:
 *   int 削除レコードインデックス、エラー時はマイナス値
 *   （-1:対象データなし、-2:インデックス更新エラー、-3:削除エラー）
 *
 ****************************************************************************/
PUBLIC int iEEPROMDeleteRemoteInfo(uint32 u32DeviceID) {
	// レコードインデックス探索
	int iIdx = iEEPROMIndexOfRemoteInfo(u32DeviceID);
	if (iIdx < 0) {
		return -1;
	}
	// リモートデバイス情報マップを更新
	sIndexInfo.u32RemoteDevMap = sIndexInfo.u32RemoteDevMap & ~(0x00000001 << iIdx);
	if (!bEEPROMWriteIndexInfo(&sIndexInfo)) {
		return -2;
	}
	// 空レコード編集
	tsAuthRemoteDevInfo sRemoteInfo;
	memset(&sRemoteInfo, '\0', sizeof(tsAuthRemoteDevInfo));
	// I2C EEPROM Write
	uint32 u32Size = sizeof(tsAuthRemoteDevInfo);
	uint32 u32Addr = TOP_ADDR_REMOTE_DEV + u32Size * iIdx;
	bEEPROM_deviceSelect(&sAppIO.sEEPROM_status);
	if (!bEEPROM_writeData(u32Addr, u32Size, (uint8*)&sRemoteInfo)) {
		return -3;
	}
	return iIdx;
}

/****************************************************************************
 *
 * NAME: bEEPROMDeleteAllRemoteInfo
 *
 * DESCRIPTION:EEPROM全クリア（リモートデバイス情報）
 *
 * PARAMETERS:             Name         RW  Usage
 *
 * RETURNS:
 *   bool_t TRUE:削除成功
 *
 ****************************************************************************/
PUBLIC int iEEPROMDeleteAllRemoteInfo() {
	// インデックス情報読み込み
	if (!bEEPROMReadIndexInfo(&sIndexInfo)) {
		return -1;
	}
	// リモートデバイス情報マップをクリア
	sIndexInfo.u32RemoteDevMap = 0;
	if (!bEEPROMWriteIndexInfo(&sIndexInfo)) {
		return -2;
	}
	// 空レコード編集
	tsAuthRemoteDevInfo sRemoteInfo;
	memset(&sRemoteInfo, '\0', sizeof(tsAuthRemoteDevInfo));
	// I2C EEPROM Write
	bEEPROM_deviceSelect(&sAppIO.sEEPROM_status);
	uint16 u16Size = sizeof(tsAuthRemoteDevInfo);
	uint16 u16Addr;
	uint8  u8Idx;
	for (u8Idx = 0; u8Idx < MAX_REMOTE_DEV_CNT; u8Idx++) {
		u16Addr = TOP_ADDR_REMOTE_DEV + u16Size * u8Idx;
		if (!bEEPROM_writeData(u16Addr, u16Size, (uint8*)&sRemoteInfo)) {
			return -3;
		}
	}
	return 1;
}

/****************************************************************************
 *
 * NAME: iEEPROMReadLog
 *
 * DESCRIPTION:EEPROM読み込み（イベントログ情報）
 *
 * PARAMETERS:             Name         RW  Usage
 * tsAppIORemoteDevInfo*   psDevInfo    R   リモートデバイス情報
 * uint8                   u8Idx        R   探索開始インデックス
 * int                     iAdd         R   インデックス増分
 *
 * RETURNS:
 *   int 読み込みレコードインデックス、読み込めない場合はマイナス値
 *
 ****************************************************************************/
PUBLIC int iEEPROMReadLog(tsAppIOEventLog *psAppIOEventLog, uint8 u8Idx) {
	// インデックス情報読み込み
	if (!bEEPROMReadIndexInfo(&sIndexInfo)) {
		return -1;
	}
	// ログの有無判定
	if (sIndexInfo.u8EventLogCnt == 0) {
		return -2;
	}
	// 変数宣言
	uint32 u32ReadIdx = u8Idx % sIndexInfo.u8EventLogCnt;
	uint32 u32Size = sizeof(tsAppIOEventLog);
	uint32 u32Addr = TOP_ADDR_EVENT_LOG + u32Size * u32ReadIdx;
	// I2C EEPROM Read
	bEEPROM_deviceSelect(&sAppIO.sEEPROM_status);
	if (!bEEPROM_readData(u32Addr, u32Size, (uint8 *)psAppIOEventLog)) {
		return -3;
	}
	return u32ReadIdx;
}

/****************************************************************************
 *
 * NAME: iEEPROMWriteLog
 *
 * DESCRIPTION:EEPROM書き込み（イベントログ情報）
 *
 * PARAMETERS:           Name              RW  Usage
 *   uint16              u16MsgCd          R   メッセージコード
 *   tsWirelessMsg*      psWirelessMsg     R   無線メッセージ
 *
 * RETURNS:
 *   int 書き込みレコードインデックス、読み込めない場合はマイナス値
 *
 ****************************************************************************/
PUBLIC int iEEPROMWriteLog(uint16 u16MsgCd, tsWirelessMsg *psWirelessMsg) {
	// インデックス情報読み込み
	if (!bEEPROMReadIndexInfo(&sIndexInfo)) {
		return -1;
	}
	// ログ数の判定
	if (sIndexInfo.u8EventLogCnt >= MAX_EVT_LOG_CNT) {
		return -2;
	}
	// インデックス情報更新
	sIndexInfo.u8EventLogCnt++;
	if (!bEEPROMWriteIndexInfo(&sIndexInfo)) {
		return -3;
	}
	// イベントログ編集
	tsAppIOEventLog sAppIOEventLog;
	memset(&sAppIOEventLog, 0x00, sizeof(tsAppIOEventLog));
	sAppIOEventLog.u8SeqNo   = sIndexInfo.u8EventLogCnt;		// シーケンス番号
	sAppIOEventLog.u16MsgCd  = u16MsgCd;						// メッセージコード
	sAppIOEventLog.u8Command = psWirelessMsg->u8Command;		// コマンド
	sAppIOEventLog.u8StatusMap = psWirelessMsg->u8StatusMap;	// ステータスマップ
	sAppIOEventLog.u16Year   = psWirelessMsg->u16Year;			// イベント発生日（年）
	sAppIOEventLog.u8Month   = psWirelessMsg->u8Month;			// イベント発生日（月）
	sAppIOEventLog.u8Day     = psWirelessMsg->u8Day;			// イベント発生日（日）
	sAppIOEventLog.u8Hour    = psWirelessMsg->u8Hour;			// イベント発生時刻（時）
	sAppIOEventLog.u8Minute  = psWirelessMsg->u8Minute;			// イベント発生時刻（分）
	sAppIOEventLog.u8Second  = psWirelessMsg->u8Second;			// イベント発生時刻（秒）
	// I2C EEPROM Write
	int iIdx = sIndexInfo.u8EventLogCnt - 1;
	uint32 u32Size = sizeof(tsAppIOEventLog);
	uint32 u32Addr = TOP_ADDR_EVENT_LOG + u32Size * iIdx;
	bEEPROM_deviceSelect(&sAppIO.sEEPROM_status);
	if (!bEEPROM_writeData(u32Addr, u32Size, (uint8*)&sAppIOEventLog)) {
		return -4;
	}
	return iIdx;
}

/****************************************************************************
 *
 * NAME: iEEPROMDeleteAllLogInfo
 *
 * DESCRIPTION:EEPROMクリア（イベントログ情報）
 *
 * PARAMETERS:           Name              RW  Usage
 *
 * RETURNS:
 *   int 1:削除成功、-1:インデックス読み込みエラー、-2:インデックス更新エラー、-3:削除エラー
 *
 ****************************************************************************/
PUBLIC int iEEPROMDeleteAllLogInfo() {
	// インデックス情報読み込み
	if (!bEEPROMReadIndexInfo(&sIndexInfo)) {
		return -1;
	}
	// インデックス情報更新
	sIndexInfo.u8EventLogCnt = 0;
	if (!bEEPROMWriteIndexInfo(&sIndexInfo)) {
		return -2;
	}
	// 空データ編集
	uint32 u32Size = sizeof(tsAppIOEventLog);
	tsAppIOEventLog sEventLog;
	memset(&sEventLog, 0x00, u32Size);
	// I2C EEPROM Write
	bEEPROM_deviceSelect(&sAppIO.sEEPROM_status);
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < MAX_EVT_LOG_CNT; u8Idx++) {
		if (!bEEPROM_writeData(TOP_ADDR_EVENT_LOG + u32Size * u8Idx, u32Size, (uint8 *)&sEventLog)) {
			return -3;
		}
	}
	return 1;
}

/*****************************************************************************
 *
 * NAME: vEventSecond
 *
 * DESCRIPTION:秒間隔イベント処理、日時と温度の情報をRTCモジュールから取得
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 * NOTES:
 *
 *****************************************************************************/
PUBLIC void vEventSecond(uint32 u32EvtTimeMs) {
	// メインデバイス接続
	vI2CMainConnect();
	// DS3231
	bDS3231_deviceSelect(I2C_ADDR_DS3231);
	bDS3231_getDatetime(&sAppIO.sDatetime);
}

/****************************************************************************
 *
 * NAME: vLCDInit
 *
 * DESCRIPTION:LCDの初期化処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vLCDInit() {
	// LCDデバイスのアドレス
	sLCDInfo.sLCDstate.u8Address = I2C_ADDR_ST7032I;
	// LCD：コントラスト
	if (sLCDInfo.sLCDstate.u8Contrast >= 64) {
		sLCDInfo.sLCDstate.u8Contrast = LCD_CONTRAST;
	}
	// LCD：アイコンメモリ
	memset(sLCDInfo.sLCDstate.u8IconData, (uint8)0, ST7032i_ICON_DATA_SIZE);
	// 出力バッファと制御情報
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < LCD_BUFF_ROW_SIZE; u8Idx++) {
		memset(sLCDInfo.cLCDBuff[u8Idx], '\0', LCD_BUFF_COL_SIZE + 1);
		memset(sLCDInfo.u8LCDCntr[u8Idx], (uint8)0, LCD_BUFF_COL_SIZE / 4);
	}
	// 描画位置
	sLCDInfo.u8CurrentDispRow = 0;
	// カーソル位置（行）
	sLCDInfo.u8CursorPosRow = 0;
	// カーソル位置（列）
	sLCDInfo.u8CursorPosCol = 0;
	// LCDの初期化
	bST7032i_deviceSelect(&sLCDInfo.sLCDstate);
	bST7032i_init();
	// 内臓EEPROMからコントラスト情報を読み込み
	uint8 u8Contrast[1];
	if (iAHI_ReadDataFromEEPROMsegment(0, 0, u8Contrast, 1) == 0) {
		// コントラスト設定
		sLCDInfo.sLCDstate.u8Contrast = u8Contrast[0];
		bST7032i_setContrast(sLCDInfo.sLCDstate.u8Contrast);
	}
}

/*****************************************************************************
 *
 * NAME: vEventLCDdrawing
 *
 * DESCRIPTION:LCD描画処理、描画領域に書き込まれた内容をLCDに表示する
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 * NOTES:
 *
 *****************************************************************************/
PUBLIC void vEventLCDdrawing(uint32 u32EvtTimeMs) {
	// 終端文字の上書き
	sLCDInfo.cLCDBuff[sLCDInfo.u8CurrentDispRow][LCD_BUFF_COL_SIZE] = '\0';
	sLCDInfo.cLCDBuff[sLCDInfo.u8CurrentDispRow + 1][LCD_BUFF_COL_SIZE] = '\0';
	// LCD描画
#ifdef DEBUG
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
	if (!bST7032i_writeString(sLCDInfo.cLCDBuff[sLCDInfo.u8CurrentDispRow])) {
		vfPrintf(&sSerStream, "3:Err.");
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
	if (!bST7032i_setCursor(1, 0)) {
		vfPrintf(&sSerStream, "4:Err.");
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
	if (!bST7032i_writeString(sLCDInfo.cLCDBuff[sLCDInfo.u8CurrentDispRow + 1])) {
		vfPrintf(&sSerStream, "5:Err.");
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
	// カーソル制御情報設定
	if (!bLCDUpdCursorCntr()) {
		vfPrintf(&sSerStream, "6:Err.");
		SERIAL_vFlush(sSerStream.u8Device);
		return;
	}
#else
	bST7032i_deviceSelect(&sLCDInfo.sLCDstate);
	bST7032i_setCursor(0, 0);
	bST7032i_writeString(sLCDInfo.cLCDBuff[sLCDInfo.u8CurrentDispRow]);
	bST7032i_setCursor(1, 0);
	bST7032i_writeString(sLCDInfo.cLCDBuff[sLCDInfo.u8CurrentDispRow + 1]);
	// カーソル制御情報設定
	bLCDUpdCursorCntr();
#endif
}

/****************************************************************************
 *
 * NAME: u8LCDTypeToByte
 *
 * DESCRIPTION:LCDのカラム制御情報変換
 *
 * PARAMETERS:      Name         RW  Usage
 *   teLCDcntr      eType1       R   カラム属性１
 *   teLCDcntr      eType2       R   カラム属性２
 *   teLCDcntr      eType3       R   カラム属性３
 *   teLCDcntr      eType4       R   カラム属性４
 *
 * RETURNS:
 *   uint8 属性値バイト
 *
 ****************************************************************************/
PUBLIC uint8 u8LCDTypeToByte(teLCDcntr eType1, teLCDcntr eType2, teLCDcntr eType3, teLCDcntr eType4) {
	uint8 u8Cntr = eType1 & 0x03;
	u8Cntr = (u8Cntr << 2) | (eType2 & 0x03);
	u8Cntr = (u8Cntr << 2) | (eType3 & 0x03);
	u8Cntr = (u8Cntr << 2) | (eType4 & 0x03);
	return u8Cntr;
}

/****************************************************************************
 *
 * NAME: bLCDCursorSet
 *
 * DESCRIPTION:LCDのカーソル位置設定
 *
 * PARAMETERS:      Name         RW  Usage
 *       uint8      u8Row        R   行
 *       uint8      u8Col        R   列
 *
 * RETURNS:
 *   bool_t TRUE:設定成功
 *
 ****************************************************************************/
PUBLIC bool_t bLCDCursorSet(uint8 u8Row, uint8 u8Col) {
	if (u8Row >= LCD_BUFF_ROW_SIZE) {
		return FALSE;
	}
	if (u8Col >= LCD_BUFF_COL_SIZE) {
		return FALSE;
	}
	if (u8Row >= (sLCDInfo.u8CurrentDispRow + 2)) {
		sLCDInfo.u8CurrentDispRow = u8Row - 1;
	} else if (u8Row < sLCDInfo.u8CurrentDispRow) {
		sLCDInfo.u8CurrentDispRow = u8Row;
	}
	sLCDInfo.u8CursorPosRow = u8Row;
	sLCDInfo.u8CursorPosCol = u8Col;
	// カーソル移動
	return TRUE;
}

/****************************************************************************
 *
 * NAME: bLCDCursorMove
 *
 * DESCRIPTION:LCDのカーソル移動処理
 *
 * PARAMETERS:      Name         RW  Usage
 * teAppCursor      eAppCursor   R   カーソル移動方向
 *
 * RETURNS:
 *   bool_t TRUE:移動成功
 *
 ****************************************************************************/
PUBLIC bool_t bLCDCursorMove(teAppCursor eAppCursor) {
	// 仮の移動先判定
	int iStartRow = sLCDInfo.u8CursorPosRow;
	int iStartCol = sLCDInfo.u8CursorPosCol;
	bool_t chkDirection = TRUE;
	switch (eAppCursor) {
	case E_APPIO_CURSOR_UP:
		if (iStartRow > 0) {
			iStartRow--;
		} else if (iStartCol > 0) {
			iStartCol--;
		} else {
			return FALSE;
		}
		chkDirection = FALSE;
		break;
	case E_APPIO_CURSOR_DOWN:
		if (iStartRow < (LCD_BUFF_ROW_SIZE - 1)) {
			iStartRow++;
		} else if (iStartCol < (LCD_BUFF_COL_SIZE - 1)) {
			iStartCol++;
		} else {
			return FALSE;
		}
		break;
	case E_APPIO_CURSOR_LEFT:
		if (iStartCol > 0) {
			iStartCol--;
		} else if (iStartRow > 0) {
			iStartRow--;
			iStartCol = LCD_BUFF_COL_SIZE - 1;
		} else {
			return FALSE;
		}
		chkDirection = FALSE;
		break;
	case E_APPIO_CURSOR_RIGHT:
		if (iStartCol < (LCD_BUFF_COL_SIZE - 1)) {
			iStartCol++;
		} else if (iStartRow < (LCD_BUFF_ROW_SIZE - 1)) {
			iStartRow++;
			iStartCol = 0;
		} else {
			return FALSE;
		}
		break;
	}
	// 実際の移動先の探索
	int8 i8ChkRow;
	int8 i8ChkCol;
	if (chkDirection) {
		// 順方向探索
		for (i8ChkRow = iStartRow; i8ChkRow < LCD_BUFF_ROW_SIZE; i8ChkRow++) {
			for (i8ChkCol = iStartCol; i8ChkCol < LCD_BUFF_COL_SIZE; i8ChkCol++) {
				if (eLCDGetCntr(i8ChkRow, i8ChkCol) != LCD_CTR_NONE) {
					return bLCDCursorSet(i8ChkRow, i8ChkCol);
				}
			}
			iStartCol = 0;
		}
	} else {
		// 逆方向探索
		for (i8ChkRow = iStartRow; i8ChkRow >= 0; i8ChkRow--) {
			for (i8ChkCol = iStartCol; i8ChkCol >= 0; i8ChkCol--) {
				if (eLCDGetCntr(i8ChkRow, i8ChkCol) != LCD_CTR_NONE) {
					return bLCDCursorSet(i8ChkRow, i8ChkCol);
				}
			}
			iStartCol = LCD_BUFF_COL_SIZE - 1;
		}
	}
	// 対象カラムなし
	return FALSE;
}

/****************************************************************************
 *
 * NAME: bLCDCursorKeyMove
 *
 * DESCRIPTION:LCDの入力によるカーソル移動処理
 *
 * PARAMETERS:      Name         RW  Usage
 *   uint8          u8Key        R   入力文字
 *
 * RETURNS:
 *   bool_t TRUE:移動成功
 *
 ****************************************************************************/
PUBLIC bool_t bLCDCursorKeyMove(uint8 u8Key) {
	if (u8Key == (uint8)NULL) {
		return FALSE;
	}
	bool_t bResult = FALSE;
	if (u8Key == '2' || u8Key == '3') {
		// カーソル上移動
		bResult = bLCDCursorMove(E_APPIO_CURSOR_UP);
	} else if (u8Key == '8' || u8Key == '9') {
		// カーソル下移動
		bResult = bLCDCursorMove(E_APPIO_CURSOR_DOWN);
	} else if (u8Key == '4' || u8Key == '7') {
		// カーソル左移動
		bResult = bLCDCursorMove(E_APPIO_CURSOR_LEFT);
	} else if (u8Key == 'B' || u8Key == 'C') {
		// カーソル左移動
		bResult = bLCDCursorMove(E_APPIO_CURSOR_RIGHT);
	}
	return bResult;
}

/****************************************************************************
 *
 * NAME: eLCDGetCntr
 *
 * DESCRIPTION:LCDの制御情報取得処理
 *
 * PARAMETERS:      Name         RW  Usage
 *       uint8      u8Row        R   対象行
 *       uint8      u8Col        R   対象列
 *
 * RETURNS:
 *   teLCDcntr 制御情報
 *
 ****************************************************************************/
PUBLIC teLCDcntr eLCDGetCntr(uint8 u8Row, uint8 u8Col) {
	uint8 u8Cntr = sLCDInfo.u8LCDCntr[u8Row % LCD_BUFF_ROW_SIZE][(u8Col % LCD_BUFF_COL_SIZE) / 4];
	return (u8Cntr >> (6 - ((u8Col % 4) * 2))) & 0x03;
}

/****************************************************************************
 *
 * NAME: bLCDWriteNumber
 *
 * DESCRIPTION:LCD描画領域への書き込み処理（数値）
 *
 * PARAMETERS:      Name         RW  Usage
 *   uint8          u8Char       R   書き込み文字
 *   uint8          u8PadChar    R   充填文字
 *
 * RETURNS:
 *   bool_t         TRUE:書き込み成功
 *
 ****************************************************************************/
PUBLIC bool_t bLCDWriteNumber(uint8 u8Char, uint8 u8PadChar) {
	if (u8Char == 'B') {
		return bLCDBackSpace(u8PadChar);
	}
	if (u8Char == 'D') {
		return bLCDDeleteChar(u8PadChar);
	}
	if (u8Char >= '0' && u8Char <= '9') {
		return bLCDInsertChar(u8Char);
	}
	return FALSE;
}

/****************************************************************************
 *
 * NAME: bLCDWriteChar
 *
 * DESCRIPTION:LCD描画領域への書き込み処理（文字）
 *
 * PARAMETERS:      Name         RW  Usage
 *   uint8          u8Char       R   入力文字
 *
 * RETURNS:
 *   bool_t         TRUE:書き込み成功
 *
 ****************************************************************************/
PUBLIC bool_t bLCDWriteChar(uint8 u8Char) {
	if (u8Char == 'B') {
		// バックスペース
		return bLCDBackSpace(' ');
	}
	if (u8Char == 'D') {
		// デリート
		return bLCDDeleteChar(' ');
	}
	if (u8Char == '4' || u8Char == '7') {
		// 左移動
		return bLCDCursorMove(E_APPIO_CURSOR_LEFT);
	}
	if (u8Char == 'C') {
		// 右移動
		return bLCDCursorMove(E_APPIO_CURSOR_RIGHT);
	}
	// 文字選択判定
	if (u8Char != '2' && u8Char != '3' && u8Char != '8' && u8Char != '9') {
		return FALSE;
	}
	// 文字選択
	char cBefChar = sLCDInfo.cLCDBuff[sLCDInfo.u8CursorPosRow][sLCDInfo.u8CursorPosCol];
	uint8 u8Size = strlen(APP_IO_USE_STRING);
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < u8Size; u8Idx++) {
		if (APP_IO_USE_STRING[u8Idx] == cBefChar) {
			break;
		}
	}
	if (u8Char == '2' || u8Char == '3') {
		// 文字選択（次）
		if (cBefChar == ' ') {
			u8Idx = 0;
		} else {
			u8Idx = (u8Idx + 1) % u8Size;
		}
	} else if (u8Char == '8' || u8Char == '9') {
		// 文字選択（前）
		u8Idx = (u8Idx + u8Size - 1) % u8Size;
	}
	sLCDInfo.cLCDBuff[sLCDInfo.u8CursorPosRow][sLCDInfo.u8CursorPosCol] = APP_IO_USE_STRING[u8Idx];
	return TRUE;
}

/****************************************************************************
 *
 * NAME: vLCDBackSpace
 *
 * DESCRIPTION:LCD描画領域の編集処理（BSキー処理）
 *
 * PARAMETERS:      Name         RW  Usage
 *       uint8      u8PadCh      R   充填文字列
 *
 * RETURNS:
 *     bool_t       TRUE:１文字削除成功
 *
 ****************************************************************************/
PUBLIC bool_t bLCDBackSpace(uint8 u8PadCh) {
	// 移動先カラム探索
	int iRowIdx;
	int iColIdx;
	if (sLCDInfo.u8CursorPosCol > 0) {
		iRowIdx = sLCDInfo.u8CursorPosRow;
		iColIdx = sLCDInfo.u8CursorPosCol - 1;
	} else if (sLCDInfo.u8CursorPosRow > 0) {
		iRowIdx = sLCDInfo.u8CursorPosRow - 1;
		iColIdx = LCD_BUFF_COL_SIZE - 1;
	} else {
		return FALSE;
	}
	teLCDcntr eCntr;
	do {
		do {
			eCntr = eLCDGetCntr(iRowIdx, iColIdx);
			if (eCntr == LCD_CTR_INPUT || eCntr == LCD_CTR_BLINK_INPUT) {
				// カラム移動
				sLCDInfo.u8CursorPosRow = iRowIdx;
				sLCDInfo.u8CursorPosCol = iColIdx;
				iRowIdx = 0;
				iColIdx = 0;
				break;
			}
			iColIdx--;
		} while(iColIdx > 0);
		iRowIdx--;
		iColIdx = LCD_BUFF_COL_SIZE - 1;
	} while(iRowIdx > 0);
	// １文字削除
	return bLCDDeleteChar(u8PadCh);
}

/****************************************************************************
 *
 * NAME: bLCDDeleteChar
 *
 * DESCRIPTION:LCD描画領域の編集処理（DELキー処理）
 *
 * PARAMETERS:      Name         RW  Usage
 *       uint8      u8PadCh      R   充填文字列
 *
 * RETURNS:
 *     bool_t       TRUE:１文字削除成功
 *
 ****************************************************************************/
PUBLIC bool_t bLCDDeleteChar(uint8 u8PadCh) {
	teLCDcntr eCntr = eLCDGetCntr(sLCDInfo.u8CursorPosRow, sLCDInfo.u8CursorPosCol);
	if (eCntr == LCD_CTR_NONE || eCntr == LCD_CTR_BLINK) {
		return FALSE;
	}
	sLCDInfo.cLCDBuff[sLCDInfo.u8CursorPosRow][sLCDInfo.u8CursorPosCol] = u8PadCh;
	uint8 u8Idx;
	for (u8Idx = sLCDInfo.u8CursorPosCol + 1; u8Idx < LCD_BUFF_COL_SIZE; u8Idx++) {
		eCntr = eLCDGetCntr(sLCDInfo.u8CursorPosRow, u8Idx);
		if (eCntr == LCD_CTR_NONE || eCntr == LCD_CTR_BLINK) {
			break;
		}
		sLCDInfo.cLCDBuff[sLCDInfo.u8CursorPosRow][u8Idx - 1] =
								sLCDInfo.cLCDBuff[sLCDInfo.u8CursorPosRow][u8Idx];
		sLCDInfo.cLCDBuff[sLCDInfo.u8CursorPosRow][u8Idx] = u8PadCh;
	}
	return TRUE;
}

/****************************************************************************
 *
 * NAME: bLCDInsertChar
 *
 * DESCRIPTION:LCD描画領域の編集処理（文字挿入処理）
 *
 * PARAMETERS:      Name         RW  Usage
 *       uint8      u8InsCh      R   充填文字列
 *
 * RETURNS:
 *     bool_t       TRUE:１文字書き込み成功
 *
 ****************************************************************************/
PUBLIC bool_t bLCDInsertChar(uint8 u8InsCh) {
	teLCDcntr eCntr = eLCDGetCntr(sLCDInfo.u8CursorPosRow, sLCDInfo.u8CursorPosCol);
	if (eCntr == LCD_CTR_NONE || eCntr == LCD_CTR_BLINK) {
		return FALSE;
	}
	// 1列移動
	uint8 u8WkNow = sLCDInfo.cLCDBuff[sLCDInfo.u8CursorPosRow][sLCDInfo.u8CursorPosCol];
	uint8 u8WkPrev;
	uint8 u8Idx;
	for (u8Idx = sLCDInfo.u8CursorPosCol + 1; u8Idx < LCD_BUFF_COL_SIZE; u8Idx++) {
		eCntr = eLCDGetCntr(sLCDInfo.u8CursorPosRow, u8Idx);
		if (eCntr == LCD_CTR_NONE || eCntr == LCD_CTR_BLINK) {
			break;
		}
		u8WkPrev = u8WkNow;
		u8WkNow = sLCDInfo.cLCDBuff[sLCDInfo.u8CursorPosRow][u8Idx];
		sLCDInfo.cLCDBuff[sLCDInfo.u8CursorPosRow][u8Idx] = u8WkPrev;
	}
	// 文字挿入
	sLCDInfo.cLCDBuff[sLCDInfo.u8CursorPosRow][sLCDInfo.u8CursorPosCol] = u8InsCh;
	// カーソル移動
	bLCDCursorMove(E_APPIO_CURSOR_RIGHT);
	return TRUE;
}

/****************************************************************************
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
 ****************************************************************************/
PUBLIC void vWirelessInit(tsAuthDeviceInfo*  psDevInfo) {
	// 無線通信情報を無効状態にする
	vWirelessRxDisabled();
	// 通信暗号化設定
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < 12 && psDevInfo->u8AESPassword[u8Idx] == '\0'; u8Idx++);
	// 暗号化判定
	if (u8Idx < 12) {
		uint8 u8KeyBuff[16];
		memset(u8KeyBuff, '0', 4);
		memcpy(&u8KeyBuff[4], psDevInfo->u8AESPassword, 12);
		ToCoNet_bRegisterAesKey(u8KeyBuff, &sCryptDefs);
	}
}

/****************************************************************************
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
 ****************************************************************************/
PUBLIC void vWirelessRxEnabled(uint32 u32DstAddr) {
	// 送信元アドレス
	sWirelessInfo.u32DstAddr = u32DstAddr;
	// 受信の有効化
	sWirelessInfo.bRxEnabled = TRUE;
}

/****************************************************************************
 *
 * NAME: vWirelessRxDisabled
 *
 * DESCRIPTION:電文の受信無効化メソッド
 *
 * PARAMETERS:          Name        RW  Usage
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vWirelessRxDisabled() {
	// 無線通信情報を初期化
	memset(&sWirelessInfo, 0x00, sizeof(tsWirelessInfo));	// ゼロクリア
	sWirelessInfo.u32DstAddr = 0;							// 送信先アドレス
	sWirelessInfo.bRxEnabled = FALSE;						// 受信無効化
	sWirelessInfo.u8FromIdx  = 0;							// 開始インデックス
	sWirelessInfo.u8ToIdx    = 0;							// 終了インデックス
	sWirelessInfo.u8Size     = 0;							// バッファリングサイズ
}

/****************************************************************************
 *
 * NAME: bWirelessRx
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
 *     TRUE:送信成功
 *
 ****************************************************************************/
PUBLIC bool_t bWirelessRx(tsRxDataApp *psRx) {
	// チェック有効判定
	if (sWirelessInfo.bRxEnabled == FALSE) {
		return FALSE;
	}
	// バッファサイズ判定
	if (sWirelessInfo.u8Size >= RX_BUFFER_SIZE) {
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
	// データ部分の宛先アドレス判定
	uint32 u32ChkAddr;
	memcpy(&u32ChkAddr, psRx->auData, 4);
	if (u32ChkAddr != sWirelessInfo.u32DstAddr) {
		return FALSE;
	}
	// インデックス情報更新
	if (sWirelessInfo.u8Size > 0) {
		sWirelessInfo.u8ToIdx = (sWirelessInfo.u8ToIdx + 1) % RX_BUFFER_SIZE;
	}
	sWirelessInfo.u8Size++;
	// 受信メッセージ情報編集
	tsRxInfo* psRxInfo;
	psRxInfo = &sWirelessInfo.sRxBuffer[sWirelessInfo.u8ToIdx];
	psRxInfo->u32SrcAddr = psRx->u32SrcAddr;
	psRxInfo->bSecurePkt = psRx->bSecurePkt;
	memcpy(&psRxInfo->sMsg, psRx->auData, psRx->u8Len);
	// バッファリング完了
	return TRUE;
}

/****************************************************************************
 *
 * NAME: bWirelessRxFetch
 *
 * DESCRIPTION:受信電文の取り出しメソッド
 *
 * PARAMETERS:        Name          RW  Usage
 *   tsRxInfo*        sRxInfo       R   受信データ情報
 *
 * RETURNS:
 *   TRUE:バッファからのフェッチ正常終了
 *
 ****************************************************************************/
PUBLIC bool_t bWirelessRxFetch(tsRxInfo* psRxInfo) {
	// バッファサイズ判定
	if (sWirelessInfo.u8Size == 0) {
		return FALSE;
	}
	// バッファからの取り出し
	*psRxInfo = sWirelessInfo.sRxBuffer[sWirelessInfo.u8FromIdx];
	// サイズ更新
	sWirelessInfo.u8Size--;
	// インデックス更新
	if (sWirelessInfo.u8Size > 0) {
		sWirelessInfo.u8FromIdx = (sWirelessInfo.u8FromIdx + 1) % RX_BUFFER_SIZE;
	}
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
	sTx.u8Seq = ++u8TxSeq;							// フレームシーケンス
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

/****************************************************************************
 *
 * NAME: vSetRandString
 *
 * DESCRIPTION:乱数文字列生成処理
 *
 * PARAMETERS:      Name            RW  Usage
 *       char       cString[]       R   編集バッファ
 *       uint8      u8Len           R   編集文字数
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vSetRandString(char cString[], uint8 u8Len) {
	vValUtil_setRandString(cString, (char*)APP_IO_USE_STRING, u8Len);
}

/****************************************************************************
 *
 * NAME: vMelodyInit
 *
 * DESCRIPTION:メロディ初期化
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vMelodyInit() {
	// メロディ演奏の初期処理
	vMelody_init(E_EVENT_APP_MELODY_PLAY, E_AHI_TIMER_2, FALSE, FALSE);
}

/****************************************************************************
 *
 * NAME: vEventMelodyOK
 *
 * DESCRIPTION:メロディ演奏（OK）
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント情報
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vEventMelodyOK(uint32 u32EvtTimeMs) {
	// メロディ演奏リクエスト
	vMelody_request(&sMelody_ScoreK525, TRUE);
}

/****************************************************************************
 *
 * NAME: vEventMelodyNG
 *
 * DESCRIPTION:メロディ演奏（NG）
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント情報
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vEventMelodyNG(uint32 u32EvtTimeMs) {
	// メロディ演奏リクエスト
	vMelody_request(MELODY_DESTINY, TRUE);
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: bLCDUpdCursorCntr
 *
 * DESCRIPTION:LCDのカラム制御情報更新処理
 *
 * PARAMETERS:      Name         RW  Usage
 *
 * RETURNS:
 *   bool_t 設定可否
 *
 ****************************************************************************/
PRIVATE bool_t bLCDUpdCursorCntr() {
	// カーソル位置設定
	if (!bST7032i_setCursor(sLCDInfo.u8CurrentDispRow - sLCDInfo.u8CursorPosRow, sLCDInfo.u8CursorPosCol)) {
		return FALSE;
	}
	// 制御情報判定
	bool_t bCursorFlg = TRUE;		// カーソル表示表示制御（ON/OFF）
	bool_t bBlinkFlg  = TRUE;		// カーソル点滅（ON/OFF）
	switch (eLCDGetCntr(sLCDInfo.u8CursorPosRow, sLCDInfo.u8CursorPosCol)) {
		case LCD_CTR_NONE:
			bCursorFlg = FALSE;
			bBlinkFlg  = FALSE;
			break;
		case LCD_CTR_BLINK:
			break;
		case LCD_CTR_INPUT:
			bBlinkFlg  = FALSE;
			break;
		case LCD_CTR_BLINK_INPUT:
			break;
	}
	// 画面制御
	return bST7032i_dispControl(TRUE, bCursorFlg, bBlinkFlg, FALSE);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
