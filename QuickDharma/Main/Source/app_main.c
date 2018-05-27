/****************************************************************************
 *
 * MODULE :Application Main source file
 *
 * CREATED:2018/05/22 21:22:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:アプリケーション特有のイベント処理を実装
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 ****************************************************************************
 * Copyright (c) 2018, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "sprintf.h"

#include <jendefs.h>
#include <AppHardwareApi.h>

/****************************************************************************/
/***        ToCoNet Definitions                                           ***/
/****************************************************************************/
// Select Modules (define befor include "ToCoNet.h")

#ifdef DEBUG
// DEBUG options
#include "serial.h"
#endif

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
#include "i2c_util.h"
#include "timer_util.h"
#include "app_main.h"
#include "app_io.h"
#include "app_event.h"
#include "Version.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
// シリアルポート初期処理
PRIVATE void vSerialInit(uint32 u32Baud, tsUartOpt *psUartOpt);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vInitHardware
 *
 * DESCRIPTION:ハードウェアの初期設定処理
 *
 * PARAMETERS:      Name            RW  Usage
 *                  f_warm_start    R   Warm Start判定フラグ
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vInitHardware(bool_t bWarmStart) {
	// Serial Initialize
	// シリアル通信の初期設定、"UART_BAUD"は通信速度115200bps
	vSerialInit(UART_BAUD, NULL);
	// 電圧低下時のリセット設定
	vAHI_BrownOutConfigure(1,		// 0:1.95V 1:2.0V 2:2.1V 3:2.2V 4:2.3V 5:2.4V 6:2.7V 7:3.0V
							TRUE,	// リセット有効化
							TRUE,	// アプリ側からの検出機能 ON/OFF
							TRUE,	// 電圧状態のステータスビットのオン設定
							TRUE);	// 電圧状態のステータスビットのオフ設定
	// vfPrintf関数の初期化処理、バッファ128Byte
	SPRINTF_vInit128();
	// ToCoNet関係のシステム情報設定
	sToCoNet_AppContext.u32AppId  = APP_ID;			// アプリケーションID
	sToCoNet_AppContext.u32ChMask = CHANNEL_MASK;	// チャンネルマスク
	sToCoNet_AppContext.u8Channel = CHANNEL;		// 利用チャンネル(11-25,26)
//	sToCoNet_AppContext.u8TxPower = 3;				// モジュール出力
//	sToCoNet_AppContext.u8TxMacRetry = 3;			// MAC再送回数（JN516x では変更できない）
	sToCoNet_AppContext.bRxOnIdle = TRUE;			// アイドル時の受信回路動作有無
	sToCoNet_AppContext.u8RandMode = 0;				// ハードウェア乱数生成
	ToCoNet_vRfConfig();
	// 受信バッファの初期化
	memset(&sWirelessInfo, 0x00, sizeof(sWirelessInfo));
	// デバッグ設定
	ToCoNet_vDebugInit(&sSerStream);
	ToCoNet_vDebugLevel(0);
	vAHI_SetStackOverflow(TRUE, 0x04008000);
	// Cold Start判定
	if (bWarmStart == FALSE) {
		// Tick Timerの初期化（4ミリ秒ごとに割り込み処理）
		vTimerUtil_initTick();
		// I2C接続の初期化
//		vI2C_init(47);	// 66KHzで動作
		vI2C_init(31);	// 100KHzで動作
	}
}

/****************************************************************************
 * NAME:cbToCoNet_vNwkEvent
 *
 * DESCRIPTION:送受信以外のネットワークイベント処理
 *             システム動作の継続が不可能となった場合（PANIC処理）もここに通知される
 *
 * PARAMETERS:      Name            RW  Usage
 *   teEvent        eEvent          R   イベント種別
 *   uint32         u32Arg          R   イベント内容情報の構造体へのポインタ
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
void cbToCoNet_vNwkEvent(teEvent eEvent, uint32 u32Arg) {
	tsToCoNet_NbScan_Result *pNwkScn;
	tsToCoNet_NbScan_Entitiy *pEnt;
	uint8 *pu8Result;
	switch(eEvent) {
	case E_EVENT_TOCONET_ENERGY_SCAN_COMPLETE:
		pu8Result = (uint8*)u32Arg;
#ifdef DEBUG
		vfPrintf(&sSerStream, "MS:%08d ENERGY_SCAN_COMPLETE Level:%d\n", u32TickCount_ms, *pu8Result);
		SERIAL_vFlush(sSerStream.u8Device);
#endif
		break;
	case E_EVENT_TOCONET_NWK_SCAN_COMPLETE:
#ifdef DEBUG
		vfPrintf(&sSerStream, "MS:%08d NWK SCAN COMPLETE!!!\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
#endif
		pNwkScn = (tsToCoNet_NbScan_Result*)u32Arg;
		if (pNwkScn->u8scanMode & TOCONET_NBSCAN_NORMAL_MASK) {
			// 全チャンネルスキャン
			uint8 idx;
			for (idx = 0; pNwkScn->u8found; idx++) {
				pEnt = &pNwkScn->sScanResult[pNwkScn->u8IdxLqiSort[idx]];
#ifdef DEBUG
				if (pEnt->bFound){
					vfPrintf(&sSerStream, "MS:%08d NWK SCAN COMPLETE Ch:%d Ad32:%08X Ad16:%04X Lqi:%d\n",
							u32TickCount_ms, pEnt->u8ch, pEnt->u32addr, pEnt->u16addr, pEnt->u8lqi);
					SERIAL_vFlush(sSerStream.u8Device);
				}
#endif
			}
		}
		break;
	case E_EVENT_TOCONET_PANIC:
		// パニック時の処理を記述
#ifdef DEBUG
		if (u32Arg) {
			tsPanicEventInfo *pInfo = (tsPanicEventInfo*)u32Arg;
			vfPrintf(&sSerStream, "PANIC!!! CD:%03d Reason:%d MSG:%s\n"
					, pInfo->u8ReasonCode, pInfo->u32ReasonInfo, pInfo->strReason);
			SERIAL_vFlush(sSerStream.u8Device);
		}
#endif
		break;
	default:
		break;
	}
}

/****************************************************************************
 *
 * NAME: cbvMcEvTxHandler
 *
 * DESCRIPTION:送信完了コールバック関数、再送処理も含めて完了してから呼び出される
 *
 * PARAMETERS:      Name            RW  Usage
 *     uint8        u8CbId          R   送信パケットのコールバックID
 *     uint8        u8Status        R   送信ステータス、(u8Status & 0x01)が1:成功、0:失敗
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PUBLIC void cbToCoNet_vTxEvent(uint8 u8CbId, uint8 u8Status) {
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d cbToCoNet_vTxEvent\n", u32TickCount_ms);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	// 受信待ち有効化
	if (bWirelessTxComplete(u8CbId, u8Status) == TRUE) {
		vWirelessRxEnabled(sDevInfo.u32DeviceID);
	}
}

/****************************************************************************
 *
 * NAME: cbvMcRxHandler
 *
 * DESCRIPTION:パケット受信後の処理、引数は受信パケット情報の構造体へのポインタ
 *
 *             pRx->u8Cmd:パケット種別（0～7）
 *             pRx->u8Len:ペイロード長（auDataのバイト数）
 *             pRx->u8Seq:送信時に設定したシーケンス番号
 *             pRx->u32SrcAddr:送信元アドレス（ショートアドレスモード時：0～0xFFFF、0xFFFFはブロードキャスト）
 *             pRx->u32DstAddr:送信先アドレス（ショートアドレスモード時：0～0xFFFF、0xFFFFはブロードキャスト）
 *             pRx->auData:送信されてきたデータ
 *             pRx->u8Lqi:受信感度（0～255）
 *             pRx->u32Tick:受信時のミリ秒カウンタ
 *             pRx->u8Hops:中継ホップ数
 *             pRx->u8RouteXOR:中継機アドレスのXOR
 *             pRx->u8Lqi:最初の中継機が受信した際の受信感度（0～255）
 *             pRx->bSecurePkt:暗号化の有無（TRUE or FALSE）
 *
 * PARAMETERS:      Name            RW  Usage
 * 	   tsRxDataApp  *psRx           R   リクエストデータ
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void cbToCoNet_vRxEvent(tsRxDataApp *psRx) {
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d cbToCoNet_vRxEvent\n", u32TickCount_ms);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	if (bWirelessRxEnq(psRx)) {
		// タスク登録：受信パケットチェック処理
		iEntrySeqEvt(E_EVENT_RX_PKT_CHK);
	}
}

/****************************************************************************
 *
 * NAME:vEventStartup
 *
 * DESCRIPTION:イベント処理：アプリケーション開始
 *
 * PARAMETERS:      Name            RW  Usage
 *   tsEvent*       psEv            R   イベント情報の構造体へのポインタ
 *   teEvent        eEvent          R   イベント種別
 *   uint32         u32EvArg        R   イベント引数
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vEventStartup(tsEvent *psEv, teEvent eEvent, uint32 u32EvArg) {
#ifdef DEBUG
	// RAMの保持判定（Cold Start）
	if (!(u32EvArg & EVARG_START_UP_WAKEUP_RAMHOLD_MASK)) {
		vfPrintf(&sSerStream, "\nMS:%08d Boot!!!\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// リセットおよび電源ONの判定
	if (!(u32EvArg & EVARG_START_UP_WAKEUP_MASK)) {
		vfPrintf(&sSerStream, "MS:%08d *** Terminal %d.%02d-%d ***\n",
				u32TickCount_ms, VERSION_MAIN, VERSION_SUB, VERSION_VAR);
		vfPrintf(&sSerStream, "MS:%08d *** %08X ***\n", u32TickCount_ms, ToCoNet_u32GetSerial());
		SERIAL_vFlush(sSerStream.u8Device);
	}
#endif
	// 外部入出力ドライバや制御タスクの初期処理
	vAppIOInit();
	// イベント処理の登録
	vEvent_RegistTask();
	// RUNNING 状態へ遷移
	ToCoNet_Event_SetState(psEv, E_STATE_RUNNING);
}

/*****************************************************************************
 *
 * NAME: u8EventSysCtrl
 *
 * DESCRIPTION:システムコントロールの割り込み処理
 *
 * PARAMETERS:      Name            RW  Usage
 *     uint32       u32DeviceId     R   デバイスID
 *     uint32       u32ItemBitmap   R   ビットマップ
 *
 * RETURNS:
 *                  FALSE -  interrupt is not handled, escalated to further
 *                           event call (cbToCoNet_vHwEvent).
 *                  TRUE  -  interrupt is handled, no further call.
 *
 * NOTES:
 *
 *****************************************************************************/
PUBLIC uint8 u8EventSysCtrl(uint32 u32DeviceId, uint32 u32ItemBitmap) {
	// Brownout判定
	if (bAHI_BrownOutStatus()) {
		// マスタートークンマスククリア
		vEEPROMTokenMaskClear();
#ifdef DEBUG
		vfPrintf(&sSerStream, "MS:%08d u8EventSysCtrl Brownout!\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
#endif
	}
	// 正常終了
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: u8EventTickTimer
 *
 * DESCRIPTION:TickTimerの割り込み処理
 *
 * PARAMETERS:      Name            RW  Usage
 *     uint32       u32DeviceId     R   デバイスID
 *     uint32       u32ItemBitmap   R   ビットマップ
 *
 * RETURNS:
 *                  FALSE -  interrupt is not handled, escalated to further
 *                           event call (cbToCoNet_vHwEvent).
 *                  TRUE  -  interrupt is handled, no further call.
 *
 * NOTES:
 *
 *****************************************************************************/
PUBLIC uint8 u8EventTickTimer(uint32 u32DeviceId, uint32 u32ItemBitmap) {
	// 処理成功
	return TRUE;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vSerialInit
 *
 * DESCRIPTION:非同期シリアル通信(UART)の初期化処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32Baud         R   ボーレート（bps）
 *   tsUartOpt*     psUartOpt       R   オプション
 *
 * RETURNS:
 *
 ****************************************************************************/
PRIVATE void vSerialInit(uint32 u32Baud, tsUartOpt *psUartOpt) {
#ifdef DEBUG
	static uint8 au8SerialTxBuffer[255];
	static uint8 au8SerialRxBuffer[255];
	/* Initialise the serial port to be used for debug output */
	sSerPort.pu8SerialRxQueueBuffer = au8SerialRxBuffer;		// 受信キューの設定
	sSerPort.pu8SerialTxQueueBuffer = au8SerialTxBuffer;		// 送信キューの設定
	sSerPort.u32BaudRate = u32Baud;								// 通信速度を設定
	sSerPort.u16AHI_UART_RTS_LOW  = 0xffff;						// 送信要求LOWの初期化
	sSerPort.u16AHI_UART_RTS_HIGH = 0xffff;						// 送信要求HIGHの初期化
	sSerPort.u16SerialRxQueueSize = sizeof(au8SerialRxBuffer);	// 受信キューサイズの初期化
	sSerPort.u16SerialTxQueueSize = sizeof(au8SerialTxBuffer);	// 送信キューサイズの初期化
	sSerPort.u8SerialPort = UART_PORT_SLAVE;					// マスター OR スレーブの設定
	sSerPort.u8RX_FIFO_LEVEL = E_AHI_UART_FIFO_LEVEL_1;			// キューの設定？
	SERIAL_vInitEx(&sSerPort, psUartOpt);						// 拡張型シリアル通信初期化処理
	sSerStream.bPutChar = SERIAL_bTxChar;		// シリアル送信制御？
	sSerStream.u8Device = UART_PORT_SLAVE;		// シリアル通信のマスタ・スレーブ制御？
#endif
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
