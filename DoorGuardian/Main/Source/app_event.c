/*******************************************************************************
 *
 * MODULE :Application Event source file
 *
 * CREATED:2017/05/07 23:49:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:イベント処理を実装
 *
 * CHANGE HISTORY:
 * 2018/01/27 23:19:00 認証時の通信データをAES暗号化
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
#include "io_util.h"
#include "pwm_util.h"
#include "aes.h"
#include "sha256.h"
#include "app_event.h"
#include "app_io.h"
#include "app_auth.h"
#include "app_main.h"
#include "value_util.h"

/******************************************************************************/
/***        Macro Definitions                                               ***/
/******************************************************************************/

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/
// 受信メッセージ基本チェック
PRIVATE bool_t bEvt_RxDefaultChk(tsRxTxInfo* psRxInfo);
// 経過時間の算出処理
PRIVATE uint32 u32Evt_getElapsedTime(tsAuthRemoteDevInfo* psRemoteInfo, DS3231_datetime* psDateTime);
// 返信処理
PRIVATE bool_t bEvt_TxResponse(teAppCommand eCommand, bool_t bEncryption);
// 通信トランザクション開始
PRIVATE void vEvt_BeginTxRxTrns(tsAppTxRxTrnsInfo* psTxRxTrnsInfo, tsRxTxInfo* psRxInfo);
// 通信トランザクション終了
PRIVATE void vEvt_EndTxRxTrns(tsAppTxRxTrnsInfo* psTxRxTrnsInfo);
#ifdef DEBUG
// 配列の文字列化
PRIVATE void vConv_ToStr(uint8* pu8Src, char* pcStr, uint8 u8Size);
#endif

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/
/** 送受信トランザクション情報 */
PUBLIC tsAppTxRxTrnsInfo sTxRxTrnsInfo;
/** ハッシュ値生成情報 */
PUBLIC tsAuthHashGenState sHashGenInfo;

/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: vEvent_RegistTask
 *
 * DESCRIPTION:タスクの登録
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
PUBLIC void vEvent_RegistTask() {
	//==========================================================================
	// ハードウェアイベント処理タスク
	//==========================================================================
	// システムコントロールの割り込み処理
	vRegisterHwIntTask(E_AHI_DEVICE_SYSCTRL, u8EventSysCtrl);
	// Tick Timerの割り込み処理
	vRegisterHwIntTask(E_AHI_DEVICE_TICK_TIMER, u8EventTickTimer);

	//==========================================================================
	// ユーザーイベント処理タスク
	//==========================================================================
	bRegisterEvtTask(E_EVENT_INITIALIZE, vEvent_Init);
	bRegisterEvtTask(E_EVENT_UPD_BUFFER, vEvent_UpdBuffer);
	bRegisterEvtTask(E_EVENT_SECOND, vEvent_Second);
	bRegisterEvtTask(E_EVENT_RX_PKT_CHK, vEvent_RxPacketCheck);
	bRegisterEvtTask(E_EVENT_RX_MST_AUTH_00, vEvent_RxMstAuth_00);
	bRegisterEvtTask(E_EVENT_RX_MST_AUTH_01, vEvent_RxMstAuth_01);
	bRegisterEvtTask(E_EVENT_RX_AUTH_00, vEvent_RxAuth_00);
	bRegisterEvtTask(E_EVENT_RX_AUTH_01, vEvent_RxAuth_01);
	bRegisterEvtTask(E_EVENT_RX_AUTH_02, vEvent_RxAuth_02);
	bRegisterEvtTask(E_EVENT_RX_AUTH_03, vEvent_RxAuth_03);
	bRegisterEvtTask(E_EVENT_RX_AUTH_04, vEvent_RxAuth_04);
	bRegisterEvtTask(E_EVENT_TX_DATA, vEvent_TxData);
	bRegisterEvtTask(E_EVENT_SENSOR_CHK, vEvent_SensorCheck);
	bRegisterEvtTask(E_EVENT_SETTING_CHK, vEvent_SettingCheck);
	bRegisterEvtTask(E_EVENT_STS_UNLOCK, vEvent_StsUnlock);
	bRegisterEvtTask(E_EVENT_STS_LOCK, vEvent_StsLock);
	bRegisterEvtTask(E_EVENT_STS_IN_CAUTION, vEvent_StsInCaution);
	bRegisterEvtTask(E_EVENT_STS_ALARM_UNLOCK, vEvent_StsAlarmUnlock);
	bRegisterEvtTask(E_EVENT_STS_ALARM_LOCK, vEvent_StsAlarmLock);
	bRegisterEvtTask(E_EVENT_STS_ALARM_LOG, vEvent_StsAlarmtLog);
	bRegisterEvtTask(E_EVENT_STS_MST_UNLOCK, vEvent_StsMstUnlock);
#ifdef DEBUG
	bRegisterEvtTask(E_EVENT_LCD_DRAWING, vEvent_LCDdrawing);
#endif
	bRegisterEvtTask(E_EVENT_HASH_ST, vEvent_HashStretching);

	//==========================================================================
	// スケジュールイベント登録
	//==========================================================================
	// タスク登録：アプリケーション初期化
	iEntryScheduleEvt(E_EVENT_INITIALIZE, 100, 0, FALSE);
	// タスク登録：入力バッファ更新
	iEntryScheduleEvt(E_EVENT_UPD_BUFFER, 50, 30, TRUE);
	// タスク登録：秒間隔処理
	iEntryScheduleEvt(E_EVENT_SECOND, 1000, 60, TRUE);
	// タスク登録：センサーチェック処理
	iEntryScheduleEvt(E_EVENT_SENSOR_CHK, 100, 90, TRUE);
	// タスク登録：サーボ設定チェック
	iEntryScheduleEvt(E_EVENT_SETTING_CHK, 100, 120, TRUE);
#ifdef DEBUG
	// タスク登録：LCD描画処理
	iEntryScheduleEvt(E_EVENT_LCD_DRAWING, 1000, 150, TRUE);
#endif
}

/*******************************************************************************
 *
 * NAME: vEvent_Init
 *
 * DESCRIPTION:アプリケーション初期化イベント処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_Init(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 初期化処理
	//==========================================================================
	sAppStsInfo.eAppStatus = E_APP_STS_INITIALIZE;
	//==========================================================================
	// 設定初期化
	//==========================================================================
	// アプリケーションイベントマップ初期化
	memset(&sAppEventMap, 0x00, sizeof(tsAppEventMap));
	// 入力バッファの更新
	vUpdInputBuffer();

	//==========================================================================
	// アプリケーションステータス移行
	//==========================================================================
	// 受信待ち設定
	vWirelessRxEnabled(sDevInfo.u32DeviceID);
	// 開錠処理を起動
	iEntrySeqEvt(E_EVENT_STS_UNLOCK);
}

/*******************************************************************************
 *
 * NAME: vEvent_UpdBuffer
 *
 * DESCRIPTION:各種バッファ更新
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint32         u32EvtTimeMs    R   イベント情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_UpdBuffer(uint32 u32EvtTimeMs) {
	// 入力バッファの更新
	vUpdInputBuffer();
}

/*******************************************************************************
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
 ******************************************************************************/
PUBLIC void vEvent_Second(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 日時・温度取得
	//==========================================================================
	vDateTimeUpdate();

	//==========================================================================
	// サーボ電源オフ
	//==========================================================================
	// 電源のオフ判定
	if (sAppStsInfo.bInProgressFlg == TRUE && sAppEventInfo.u32PwrOffTime <= u32TickCount_ms) {
		// 5V電源をオフにする
		v5VPwOff();
		// サーボ制御オフ
		vServoControlOff();
		// サーボ位置情報の更新
		vUpdServoPos();
		// 移行中フラグ
		sAppStsInfo.bInProgressFlg = FALSE;
	}

#ifdef DEBUG
	// 入力バッファの更新
//	vUpdInputBuffer();
	// アウトプット
//	sAppIO.iServoPos  = iIOUtil_adcReadBuffer(&sAppIO.sServoPosBuffer, ADC_CHK_TOLERANCE, 5);
//	sAppIO.iServoSetA = iIOUtil_adcReadBuffer(&sAppIO.sServoSetABuffer, ADC_CHK_TOLERANCE, 5);
//	sAppIO.iServoSetB = iIOUtil_adcReadBuffer(&sAppIO.sServoSetBBuffer, ADC_CHK_TOLERANCE, 5);
//	u8DebugSts = (u8DebugSts + 1) % 32 + 1;
//	vAHI_DioSetDirection(0x00, 0xFFFFFFFF);
	// AES Test
	vAES_test();
	// デバッグメッセージ
	char cMsg1[17];
	char cMsg2[17];
	DS3231_datetime* pdt = &sAppIO.sDatetime;
	sprintf(cMsg1, "Date:%04d/%02d/%02d ", pdt->u16Year, pdt->u8Month, pdt->u8Day);
//	sprintf(cMsg1, "Tick:%05d %05d", (int)u32AHI_TickTimerRead(), (int)u32TickCount_ms);
//	sprintf(cMsg2, "Time:%02d:%02d:%02d %02X", pdt->u8Hour, pdt->u8Minutes, pdt->u8Seconds, u8DebugSts);
//	sprintf(cMsg2, "Time:%02d:%02d:%02d %02X", pdt->u8Hour, pdt->u8Minutes, pdt->u8Seconds, u8DebugVal);
	sprintf(cMsg2, "%02d:%02d:%02d %03d %03d", pdt->u8Hour, pdt->u8Minutes, pdt->u8Seconds, u8DebugSts, (int)u32DebugVal);
//	sprintf(cMsg2, "%02d:%02d:%02d %07d", pdt->u8Hour, pdt->u8Minutes, pdt->u8Seconds, sAppStsInfo.u32RemoteDevID[pdt->u8Seconds % 32]);
//	sprintf(cMsg2, "%02d:%02d:%02d %07d", pdt->u8Hour, pdt->u8Minutes, pdt->u8Seconds, sWirelessInfo.sRxBuffer[pdt->u8Seconds % 10].u32SrcAddr);
//	sprintf(cMsg2, "%03d %03d %08X", u32DebugVal, sIndexInfo.u8EventLogCnt, sIndexInfo.u32RemoteDevMap);
//	sprintf(cMsg2, "%03d %03d %08X", pdt->u8Seconds, sIndexInfo.u8EventLogCnt, sAppStsInfo.u32RemoteDevID[pdt->u8Seconds % 5]);
//	sprintf(cMsg2, "%02X %02d %10s", u8DebugSts, sIndexInfo.u8EventLogCnt, sDevInfo.cDeviceName);
//	sprintf(cMsg2, "Time:%02d:%02d:%02d %02X", pdt->u8Hour, pdt->u8Minutes, pdt->u8Seconds, (int)sAppStsInfo.u32RemoteDevID[pdt->u8Seconds % 5]);
//	sprintf(cMsg2, "Data:%08d %02X", sAppIO.iTemperature, u8DebugSts);
//	sprintf(cMsg2, "Data:%03d %03d %02X ", (int)u8DebugCRC1, (int)u8DebugCRC2, u8DebugSts);
//	sprintf(cMsg2, "%06d %02X %02X  ", (int)sDebugMsg.u32DstAddr, sDebugMsg.u8Command, u8DebugSts);
//	sprintf(cMsg2, "%06d %02X %04d", (int)sDebugMsg.u32DstAddr, sDebugMsg.u8Command, u8DebugVal);
//	sprintf(cMsg2, "%06d %03d %03d  ", (int)sDebugMsg.u32DstAddr, u8DebugCRC1, u8DebugCRC2);
//	sprintf(cMsg2, "%06d %03d %03d  ", (int)sDebugMsg.u32DstAddr, (int)sDebugMsg.u8CRC, (int)u8DebugCRC2);
//	sprintf(cMsg2, "Data:%03d %03d %02X ", (int)sizeof(tsWirelessMsg), (int)u8DebugCRC2, u8DebugSts);
//	sprintf(cMsg2, "Tick:%05d %05d", sAppIO.iServoSetA, sAppIO.iServoSetB);
//	sprintf(cMsg2, "Tick:%05d %05d", sAppIO.iServoPos, sAppIO.iServoSetB);
//	sprintf(cMsg2, "Tick:%08X   ", sAppIO.u32DiMap);
	vLCDSetMessage(cMsg1, cMsg2);
#endif
}

/*******************************************************************************
 *
 * NAME: vEvent_RxPacketCheck
 *
 * DESCRIPTION:イベント処理：無線パケット受信チェック処理
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_RxPacketCheck(uint32 u32EvtTimeMs) {
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d vEvent_RxPacketCheck Idx:%03d Size:%03d\n",
			u32TickCount_ms, sWirelessInfo.u8RxToIdx, sWirelessInfo.u8RxSize);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	// 受信パケットの読み込み
	bool_t bValidFlg = FALSE;
	tsRxTxInfo sRxInfo;
	while (bWirelessRxDeq(&sRxInfo)) {
		// 受信メッセージチェック
		if (bEvt_RxDefaultChk(&sRxInfo) == FALSE) {
			continue;
		}
		// 受信パケットフラグ有効化
		bValidFlg = TRUE;
		// 受信機能を無効化、バッファもクリア
		vWirelessRxDisabled();
		// 通信トランザクション開始処理
		vEvt_BeginTxRxTrns(&sTxRxTrnsInfo, &sRxInfo);
	}
	// フェッチ済み判定
	if (bValidFlg == FALSE) {
		return;
	}
	// コマンド判定
	teAppEvent eAppEvent = 0x00;
	switch (sTxRxTrnsInfo.sRxWlsMsg.u8Command) {
	case E_APP_CMD_CHK_STATUS:
		// 送受信コマンド：ステータス要求
		bEvt_TxResponse(E_APP_CMD_ACK, FALSE);
		return;
	case E_APP_CMD_UNLOCK:
		// 送受信コマンド：開錠処理
		sTxRxTrnsInfo.eOkAppEvt = sAppEventMap.eEvtUnlockReq;
		eAppEvent = E_EVENT_RX_AUTH_00;
		break;
	case E_APP_CMD_LOCK:
		// 送受信コマンド：通常施錠要求
		sTxRxTrnsInfo.eOkAppEvt = sAppEventMap.eEvtLockReq;
		eAppEvent = E_EVENT_RX_AUTH_00;
		break;
	case E_APP_CMD_ALERT:
		// 送受信コマンド：警戒施錠要求
		sTxRxTrnsInfo.eOkAppEvt = sAppEventMap.eEvtInCautionReq;
		eAppEvent = E_EVENT_RX_AUTH_00;
		break;
	case E_APP_CMD_MST_UNLOCK:
		// 送受信コマンド：マスター開錠要求
		sTxRxTrnsInfo.eOkAppEvt = sAppEventMap.eEvtMstUnlockReq;
		eAppEvent = E_EVENT_RX_MST_AUTH_00;
		break;
	}
	if (eAppEvent != 0x00) {
		iEntrySeqEvt(eAppEvent);
	} else {
		// NACK返信
		bEvt_TxResponse(E_APP_CMD_NACK, FALSE);
		// 受信待ち有効化
		vWirelessRxEnabled(sDevInfo.u32DeviceID);
	}
}

/*******************************************************************************
 *
 * NAME: vEvent_RxMstAuth_00
 *
 * DESCRIPTION:イベント処理：マスターパスワード認証00
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_RxMstAuth_00(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 認証ハッシュの生成依頼
	//==========================================================================
	// 受信メッセージ
	tsWirelessMsg* psRxMsg = &sTxRxTrnsInfo.sRxWlsMsg;
	// マスターハッシュ生成情報
	sHashGenInfo = sAuth_generateHashInfo(psRxMsg->u8AuthToken, sDevInfo.u16MstPWStretching);
	// 完了後の復帰イベントに次の処理イベントを設定
	sTxRxTrnsInfo.eRtnAppEvt = E_EVENT_RX_MST_AUTH_01;
	// ハッシュ化処理をバックグラウンドプロセスとして起動
	iEntrySeqEvt(E_EVENT_HASH_ST);
}

/*******************************************************************************
 *
 * NAME: vEvent_RxMstAuth_01
 *
 * DESCRIPTION:イベント処理：マスターパスワード認証01
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_RxMstAuth_01(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 認証処理
	//==========================================================================
	// 受信メッセージ
	tsWirelessMsg* psRxMsg = &sTxRxTrnsInfo.sRxWlsMsg;
	// マスターパスワードハッシュ値の判定
	teAppCommand eCommand;
	if (memcmp(sHashGenInfo.u8HashCode, sDevInfo.u8MstPWHash, APP_AUTH_TOKEN_SIZE) == 0) {
		// ACKレスポンス
		eCommand = E_APP_CMD_ACK;
		// 認証成功イベント
		iEntrySeqEvt(sTxRxTrnsInfo.eOkAppEvt);
	} else {
		// NACKレスポンス
		eCommand = E_APP_CMD_NACK;
		// ステータス変更
		sAppStsInfo.eAppStatus = E_APP_STS_ALARM_LOCK;
		// ステータスマップ更新
		bEEPROMWriteDevInfo(APP_STS_MAP_AUTH_ERR);
		// エラーログの書き込み
		iEEPROMWriteLog(E_MSG_CD_RX_MST_TKN_ERR, psRxMsg->u8Command);
	}
	// レスポンスの送信
	if (bEvt_TxResponse(eCommand, FALSE) == FALSE) {
		// ステータスマップ更新
		bEEPROMWriteDevInfo(APP_STS_MAP_IO_ERR);
		// 送信エラーログ
		iEEPROMWriteLog(E_MSG_CD_TX_ERR, eCommand);
	}
	// 通信トランザクション終了処理
	vEvt_EndTxRxTrns(&sTxRxTrnsInfo);
}

/*******************************************************************************
 *
 * NAME: vEvent_RxAuth_00
 *
 * DESCRIPTION:イベント処理：通常認証処理00
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_RxAuth_00(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 受信メッセージチェック
	//==========================================================================
	tsWirelessMsg* psRxMsg = &sTxRxTrnsInfo.sRxWlsMsg;
	// 送信元アドレスチェック
	int iRmtDevIdx = iEEPROMIndexOfRemoteInfo(sTxRxTrnsInfo.u32DstAddr);
	if (iRmtDevIdx < 0) {
		vfPrintf(&sSerStream, "MS:%08d vEvent_RxAuth_00 No.1\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
		// 通信トランザクション終了処理
		vEvt_EndTxRxTrns(&sTxRxTrnsInfo);
		return;
	}
	//==========================================================================
	// リモートデバイス情報の読み込み
	//==========================================================================
	tsAuthRemoteDevInfo* psRemoteInfo = &sTxRxTrnsInfo.sRemoteInfo;
	if (iEEPROMReadRemoteInfo(psRemoteInfo, iRmtDevIdx) < 0) {
		vfPrintf(&sSerStream, "MS:%08d vEvent_RxAuth_00 No.2\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
		// メモリIOエラー
		bEEPROMWriteDevInfo(APP_STS_MAP_IO_ERR);
		iEEPROMWriteLog(E_MSG_CD_READ_RMT_DEV_ERR, psRxMsg->u8Command);
		// 通信トランザクション終了処理
		vEvt_EndTxRxTrns(&sTxRxTrnsInfo);
		return;
	}
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d vEvent_RxAuth_00 Dst:%010d Idx:%02d ID:%010d\n", u32TickCount_ms,
			sTxRxTrnsInfo.u32DstAddr, iRmtDevIdx, psRemoteInfo->u32DeviceID);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	//==========================================================================
	// ワンタイムトークンの生成依頼
	//==========================================================================
	// 同期トークンとワンタイム乱数を元にハッシュ関数を利用してワンタイムトークンを生成
	uint8 u8StCnt = (psRxMsg->u32SyncVal % (256 - APP_HASH_STRETCHING_CNT_MIN)) + APP_HASH_STRETCHING_CNT_MIN;
	sHashGenInfo = sAuth_generateHashInfo(psRemoteInfo->u8SyncToken, u8StCnt);
	sHashGenInfo.u32ShufflePtn = psRxMsg->u32SyncVal;
	// 完了後の復帰イベントに次の処理イベントを設定
	sTxRxTrnsInfo.eRtnAppEvt = E_EVENT_RX_AUTH_01;
	// ハッシュ化処理をバックグラウンドプロセスとして起動
	iEntrySeqEvt(E_EVENT_HASH_ST);
}

/*******************************************************************************
 *
 * NAME: vEvent_RxAuth_01
 *
 * DESCRIPTION:イベント処理：通常認証処理01
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_RxAuth_01(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 受信メッセージの復号化
	//==========================================================================
	tsWirelessMsg* psRxMsg = &sTxRxTrnsInfo.sRxWlsMsg;
	// ワンタイムトークンのコピー
	memcpy(sTxRxTrnsInfo.u8OneTimeTkn, sHashGenInfo.u8HashCode, APP_AUTH_TOKEN_SIZE);
	// 受信メッセージの暗号化された領域を復号化
	tsAES_state sAES_state =
			vAES_newCBCState(AES_KEY_LEN_256, sTxRxTrnsInfo.u8OneTimeTkn, sTxRxTrnsInfo.u8OneTimeTkn);
	vAES_decrypt(&sAES_state, &psRxMsg->u8AuthStCnt, 80);

	//==========================================================================
	// 返信ハッシュ生成情報
	//==========================================================================
	tsAuthRemoteDevInfo* psRemoteInfo = &sTxRxTrnsInfo.sRemoteInfo;
	// 返信ストレッチング回数を算出
	sTxRxTrnsInfo.u16RespStCnt =
			u16Auth_convToRespStCnt(u32Evt_getElapsedTime(psRemoteInfo, &sTxRxTrnsInfo.sRefDatetime));
	// 返信ハッシュ生成情報
	sHashGenInfo = sAuth_generateHashInfo(psRemoteInfo->u8AuthCode, sTxRxTrnsInfo.u16RespStCnt);
	vAuth_setSyncToken(&sHashGenInfo, psRemoteInfo->u8SyncToken);
	// 完了後の復帰イベントに次の処理イベントを設定
	sTxRxTrnsInfo.eRtnAppEvt = E_EVENT_RX_AUTH_02;
	// ハッシュ化処理をバックグラウンドプロセスとして起動
	iEntrySeqEvt(E_EVENT_HASH_ST);
}

/*******************************************************************************
 *
 * NAME: vEvent_RxAuth_02
 *
 * DESCRIPTION:イベント処理：通常認証処理02
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_RxAuth_02(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 返信ハッシュの退避
	//==========================================================================
	memcpy(sTxRxTrnsInfo.u8ResponseTkn, sHashGenInfo.u8HashCode, APP_AUTH_TOKEN_SIZE);

	//==========================================================================
	// 認証ハッシュ生成依頼
	//==========================================================================
	// 受信メッセージ
	tsWirelessMsg* psRxMsg = &sTxRxTrnsInfo.sRxWlsMsg;
	// リモートデバイス情報
	tsAuthRemoteDevInfo* psRemoteInfo = &sTxRxTrnsInfo.sRemoteInfo;
	// ストレッチング回数の残量を算出
	uint16 u16StCnt =
			psRxMsg->u8AuthStCnt + psRemoteInfo->u8RcvStretching + APP_HASH_STRETCHING_CNT_BASE;
	// 認証ハッシュ生成情報
	sHashGenInfo = sAuth_generateHashInfo(sTxRxTrnsInfo.u8ResponseTkn, u16StCnt - sTxRxTrnsInfo.u16RespStCnt);
	vAuth_setSyncToken(&sHashGenInfo, psRemoteInfo->u8SyncToken);
	// 完了後の復帰イベントに自イベントを設定
	sTxRxTrnsInfo.eRtnAppEvt = E_EVENT_RX_AUTH_03;
	// ハッシュ化処理をバックグラウンドプロセスとして起動
	iEntrySeqEvt(E_EVENT_HASH_ST);
}

/*******************************************************************************
 *
 * NAME: vEvent_RxAuth_03
 *
 * DESCRIPTION:イベント処理：通常認証処理03
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_RxAuth_03(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 認証処理
	//==========================================================================
	// 受信メッセージ
	tsWirelessMsg* psRxMsg = &sTxRxTrnsInfo.sRxWlsMsg;
#ifdef DEBUG
	char cDbgToken[APP_AUTH_TOKEN_SIZE * 2 + 1];
	vConv_ToStr(psRxMsg->u8AuthToken, cDbgToken, APP_AUTH_TOKEN_SIZE);
	vfPrintf(&sSerStream, "MS:%08d Rx:%64s\n", u32TickCount_ms, cDbgToken);
	vConv_ToStr(sHashGenInfo.u8HashCode, cDbgToken, APP_AUTH_TOKEN_SIZE);
	vfPrintf(&sSerStream, "MS:%08d Au:%64s\n", u32TickCount_ms, cDbgToken);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	if (memcmp(psRxMsg->u8AuthToken, sHashGenInfo.u8HashCode, APP_AUTH_TOKEN_SIZE) != 0) {
		// 認証エラー
		bEEPROMWriteDevInfo(APP_STS_MAP_AUTH_ERR);
		iEEPROMWriteLog(E_MSG_CD_RX_AUTH_TKN_ERR, psRxMsg->u8Command);
		// NACK返信
		bEvt_TxResponse(E_APP_CMD_NACK, TRUE);
		// 通信トランザクション終了処理
		vEvt_EndTxRxTrns(&sTxRxTrnsInfo);
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d Auth Error Status:%02X\n", u32TickCount_ms, sDevInfo.u8StatusMap);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
		return;
	}

	//==========================================================================
	// 更新後の認証情報生成
	//==========================================================================
	// 更新ストレッチング回数（送信側）
	uint16 u16StCnt = u16ValUtil_getRandVal();
	sTxRxTrnsInfo.u8UpdStretchingCntS = ((u16StCnt & 0xFF) % 251) + APP_HASH_STRETCHING_CNT_MIN;
	// 更新ストレッチング回数（受信側）
	sTxRxTrnsInfo.u8UpdStretchingCntR = ((u16StCnt >> 8) % 251) + APP_HASH_STRETCHING_CNT_MIN;
	// 更新後ストレッチング回数
	u16StCnt =
		sTxRxTrnsInfo.u8UpdStretchingCntS + sTxRxTrnsInfo.u8UpdStretchingCntR + APP_HASH_STRETCHING_CNT_BASE;
	// 更新後の認証コード生成
	vValUtil_setU8RandArray(sTxRxTrnsInfo.u8UpdateTkn, APP_AUTH_TOKEN_SIZE);
	// リモートデバイス情報
	tsAuthRemoteDevInfo* psRemoteInfo = &sTxRxTrnsInfo.sRemoteInfo;
	// 更新後の同期トークン生成
	memcpy(sTxRxTrnsInfo.u8UpdSyncTkn, psRemoteInfo->u8SyncToken, APP_AUTH_TOKEN_SIZE);
	vValUtil_masking(sTxRxTrnsInfo.u8UpdSyncTkn, sTxRxTrnsInfo.u8OneTimeTkn, APP_AUTH_TOKEN_SIZE);

	//==========================================================================
	// 更新後の認証ハッシュ生成
	//==========================================================================
	sHashGenInfo = sAuth_generateHashInfo(sTxRxTrnsInfo.u8UpdateTkn, u16StCnt);
	vAuth_setSyncToken(&sHashGenInfo, sTxRxTrnsInfo.u8UpdSyncTkn);
	// 完了後の復帰イベントに自イベントを設定
	sTxRxTrnsInfo.eRtnAppEvt = E_EVENT_RX_AUTH_04;
	// ハッシュ化処理をバックグラウンドプロセスとして起動
	iEntrySeqEvt(E_EVENT_HASH_ST);
}

/*******************************************************************************
 *
 * NAME: vEvent_RxAuth_04
 *
 * DESCRIPTION:イベント処理：通常認証後処理04
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_RxAuth_04(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 認証結果のレスポンス処理
	//==========================================================================
#ifdef DEBUG
	char cDbgToken[APP_AUTH_TOKEN_SIZE * 2 + 1];
	vConv_ToStr(sHashGenInfo.u8HashCode, cDbgToken, APP_AUTH_TOKEN_SIZE);
	vfPrintf(&sSerStream, "MS:%08d Ut:%64s\n", u32TickCount_ms, cDbgToken);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	// 返信電文の編集
	tsWirelessMsg sTxMsg;
	memset(&sTxMsg, 0x00, sizeof(tsWirelessMsg));
	sTxMsg.u32DstAddr = sTxRxTrnsInfo.u32DstAddr;		// 宛先アドレス
	sTxMsg.u8Command  = E_APP_CMD_AUTH_ACK;				// 認証ありACKコマンド
	//--------------------------------------------------------------------------
	// レスポンス認証情報の編集
	//--------------------------------------------------------------------------
	// リモートデバイス情報
	tsAuthRemoteDevInfo* psRemoteInfo = &sTxRxTrnsInfo.sRemoteInfo;
	// ストレッチング回数
	sTxMsg.u8AuthStCnt = psRemoteInfo->u8RcvStretching;
	// レスポンストークン
	memcpy(sTxMsg.u8AuthToken, sTxRxTrnsInfo.u8ResponseTkn, APP_AUTH_TOKEN_SIZE);
	//--------------------------------------------------------------------------
	// レスポンス更新認証情報の編集
	//--------------------------------------------------------------------------
	// 更新ストレッチング回数
	sTxMsg.u8UpdAuthStCnt = sTxRxTrnsInfo.u8UpdStretchingCntS;
	// 更新認証トークン
	memcpy(sTxMsg.u8UpdAuthToken, sHashGenInfo.u8HashCode, APP_AUTH_TOKEN_SIZE);
	// 返信データ
	sTxMsg.u16Year     = sAppIO.sDatetime.u16Year;		// 年
	sTxMsg.u8Month     = sAppIO.sDatetime.u8Month;		// 月
	sTxMsg.u8Day       = sAppIO.sDatetime.u8Day;		// 日
	sTxMsg.u8Hour      = sAppIO.sDatetime.u8Hour;		// 時
	sTxMsg.u8Minute    = sAppIO.sDatetime.u8Minutes;	// 分
	sTxMsg.u8Second    = sAppIO.sDatetime.u8Seconds;	// 秒
	sTxMsg.u8StatusMap = sDevInfo.u8StatusMap;			// ステータスマップ
	// 暗号化領域の暗号化
	tsAES_state sAES_state =
			vAES_newCBCState(AES_KEY_LEN_256, sTxRxTrnsInfo.u8OneTimeTkn, sTxRxTrnsInfo.u8OneTimeTkn);
	vAES_encrypt(&sAES_state, &sTxMsg.u8AuthStCnt, 80);
	// CRC8編集
	sTxMsg.u8CRC = u8CCITT8((uint8*)&sTxMsg, TX_REC_SIZE);
	//--------------------------------------------------------------------------
	// 電文の送信
	//--------------------------------------------------------------------------
	if (bWirelessTxEnq(sDevInfo.u32DeviceID, TRUE, &sTxMsg) == FALSE) {
		// 送信エラーログ
		bEEPROMWriteDevInfo(APP_STS_MAP_IO_ERR);
		iEEPROMWriteLog(E_MSG_CD_TX_ERR, E_APP_CMD_AUTH_ACK);
		// 通信トランザクション終了処理
		vEvt_EndTxRxTrns(&sTxRxTrnsInfo);
		return;
	}
	// イベントタスク登録：レスポンス送信
	iEntrySeqEvt(E_EVENT_TX_DATA);
	//==========================================================================
	// 認証情報の更新
	//==========================================================================
	memcpy(psRemoteInfo->u8SyncToken, sTxRxTrnsInfo.u8UpdSyncTkn, APP_AUTH_TOKEN_SIZE);
	memcpy(psRemoteInfo->u8AuthCode, sTxRxTrnsInfo.u8UpdateTkn, APP_AUTH_TOKEN_SIZE);
	psRemoteInfo->u8RcvStretching = sTxRxTrnsInfo.u8UpdStretchingCntR;
	// リモートデバイス情報の書き込み
	if (iEEPROMWriteRemoteInfo(psRemoteInfo) < 0) {
		// 書き込みエラー
		bEEPROMWriteDevInfo(APP_STS_MAP_IO_ERR);
		iEEPROMWriteLog(E_MSG_CD_WRITE_RMT_DEV_ERR, 0x00);
	}

	//==========================================================================
	// 認証成功時のイベント処理
	//==========================================================================
	// 成功イベント
	iEntrySeqEvt(sTxRxTrnsInfo.eOkAppEvt);
	//==========================================================================
	// トランザクション終了処理
	//==========================================================================
	// 通信トランザクション終了処理
	vEvt_EndTxRxTrns(&sTxRxTrnsInfo);
}

/*******************************************************************************
 *
 * NAME: vEvent_TxData
 *
 * DESCRIPTION:イベント処理：無線パケット送信処理
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_TxData(uint32 u32EvtTimeMs) {
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d vEvent_TxData\n", u32TickCount_ms);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	// 電文の送信処理
	if (bWirelessTxTry() == TRUE) {
		// スケジュールタスク登録：電文送信
		iEntryScheduleEvt(E_EVENT_TX_DATA, 0, 64, FALSE);
	}
}

/*******************************************************************************
 *
 * NAME: vEvent_SensorCheck
 *
 * DESCRIPTION:センサーチェック処理
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_SensorCheck(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 温度センサーチェック
	//==========================================================================
	if (sAppIO.iTemperature >= APP_TEMPERATURE_THRESHOLD) {
		// イベント処理
		sAppEventInfo.u8LogMsgCd  = E_MSG_CD_TEMPERATURE_SENS_ERR;
		sAppEventInfo.u8UpdStsMap = APP_STS_MAP_TEMPERATURE;
		iEntrySeqEvt(E_EVENT_STS_ALARM_UNLOCK);
		return;
	}
	// ステータスチェックの要否を判定
	if (sAppStsInfo.eAppStatus == E_APP_STS_INITIALIZE || sAppStsInfo.bInProgressFlg == TRUE) {
		return;
	}

	//==========================================================================
	// サーボ位置チェック（ADC）
	//==========================================================================
	// サーボ位置判定
	if (sAppEventMap.eEvtServoSensor != 0x00) {
		if (bServoPosChange() == TRUE) {
			// イベント処理
			sAppEventInfo.u8LogMsgCd = E_MSG_CD_SERVO_ERR;
			sAppEventInfo.u8UpdStsMap = APP_STS_MAP_SERVO_SENS;
			iEntrySeqEvt(sAppEventMap.eEvtServoSensor);
			return;
		}
	}

	//==========================================================================
	// 開放センサーチェック
	//==========================================================================
	if (sAppEventMap.eEvtOpenSensor != 0x00) {
		if ((sAppIO.u32DiMap & PIN_MAP_OPEN_SENS) != 0) {
			// イベント処理
			sAppEventInfo.u8LogMsgCd = E_MSG_CD_OPEN_SENS_ERR;
			sAppEventInfo.u8UpdStsMap = APP_STS_MAP_OPEN_SENS;
			iEntrySeqEvt(sAppEventMap.eEvtOpenSensor);
			return;
		}
	}

	//==========================================================================
	// ボタンチェック
	//==========================================================================
	if (sAppEventMap.eEvtTouchSensor != 0x00) {
		if ((sAppIO.u32DiMap & PIN_MAP_TOUCH_SENS) != 0) {
			// イベント処理
			sAppEventInfo.u8LogMsgCd = E_MSG_CD_BUTTON_ERR;
			sAppEventInfo.u8UpdStsMap = APP_STS_MAP_BUTTON;
			iEntrySeqEvt(sAppEventMap.eEvtTouchSensor);
			return;
		}
	}

	//==========================================================================
	// 人感センサーチェック
	//==========================================================================
	if (sAppEventMap.eEvtIRSensor != 0x00) {
		if ((sAppIO.u32DiMap & PIN_MAP_IR_SENS) != 0) {
			// イベント処理
			sAppEventInfo.u8LogMsgCd = E_MSG_CD_IR_SENS_ERR;
			sAppEventInfo.u8UpdStsMap = APP_STS_MAP_IR_SENS;
			iEntrySeqEvt(sAppEventMap.eEvtIRSensor);
			return;
		}
	}
}

/*******************************************************************************
 *
 * NAME: vEvent_StsUnlock
 *
 * DESCRIPTION:設定チェック処理
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_SettingCheck(uint32 u32EvtTimeMs) {
	// 開放センサー
	if ((sAppIO.u32DiMap & PIN_MAP_OPEN_SENS) == 0) {
		// 開放されていない場合
		return;
	}
	// ステータス判定
	if (sAppStsInfo.eAppStatus != E_APP_STS_UNLOCK && sAppStsInfo.eAppStatus != E_APP_STS_LOCK) {
		return;
	}
	// サーボ設定チェック
	uint8 u8Result = u8UpdServoSetting();
	if (u8Result == 0) {
		return;
	}
	// サーボ制御
	if (sAppStsInfo.eAppStatus == E_APP_STS_UNLOCK && (u8Result % 2) == 1) {
		// サーボ制御（アンロック）
		vSetServoUnlock();
	} else if (sAppStsInfo.eAppStatus == E_APP_STS_LOCK && u8Result >= 2) {
		// サーボ制御（ロック）
		vSetServoLock();
	}
}

/*******************************************************************************
 *
 * NAME: vEvent_StsUnlock
 *
 * DESCRIPTION:通常開錠状態への移行処理
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_StsUnlock(uint32 u32EvtTimeMs) {
	// ステータス判定
	if (sAppStsInfo.eAppStatus == E_APP_STS_UNLOCK) {
		// 既にステータス移行済み
		return;
	}
	//==========================================================================
	// アプリケーションステータス更新
	//==========================================================================
	// ステータス移行中
	sAppStsInfo.bInProgressFlg = TRUE;
	// 開錠状態
	sAppStsInfo.eAppStatus = E_APP_STS_UNLOCK;

	//==========================================================================
	// アプリケーションイベントマップ更新
	//==========================================================================
	sAppEventMap.eEvtIRSensor     = 0x00;					// 人感センサーイベント
	sAppEventMap.eEvtTouchSensor  = E_EVENT_STS_LOCK;		// タッチセンサーイベント
	sAppEventMap.eEvtOpenSensor   = 0x00;					// 開放センサーイベント
	sAppEventMap.eEvtServoSensor  = E_EVENT_STS_ALARM_LOCK;	// サーボセンサーイベント
	sAppEventMap.eEvtUnlockReq    = 0x00;					// 通常開錠リクエスト
	sAppEventMap.eEvtLockReq      = E_EVENT_STS_LOCK;		// 通常施錠リクエスト
	sAppEventMap.eEvtInCautionReq = E_EVENT_STS_IN_CAUTION;	// 警戒施錠リクエスト
	sAppEventMap.eEvtMstUnlockReq = 0x00;					// マスターパスワード開錠リクエスト

	//==========================================================================
	// ステータス移行処理
	//==========================================================================
	// サーボ制御（アンロック）
	vSetServoUnlock();
	// 指定時間後に5V電源OFFイベント
	sAppEventInfo.u32PwrOffTime = u32TickCount_ms + SERVO_WAIT;
}

/*******************************************************************************
 *
 * NAME: vEvent_StsLock
 *
 * DESCRIPTION:通常施錠状態への移行処理
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_StsLock(uint32 u32EvtTimeMs) {
	// ステータス判定
	if (sAppStsInfo.eAppStatus == E_APP_STS_LOCK) {
		// 既にステータス移行済み
		return;
	}
	//==========================================================================
	// アプリケーションステータス更新
	//==========================================================================
	// ステータス移行中
	sAppStsInfo.bInProgressFlg = TRUE;
	// 施錠状態
	sAppStsInfo.eAppStatus = E_APP_STS_LOCK;

	//==========================================================================
	// アプリケーションイベントマップ更新
	//==========================================================================
	sAppEventMap.eEvtIRSensor     = 0x00;					// 人感センサーイベント
	sAppEventMap.eEvtTouchSensor  = E_EVENT_STS_UNLOCK;		// タッチセンサーイベント
	sAppEventMap.eEvtOpenSensor   = 0x00;					// 開放センサーイベント
	sAppEventMap.eEvtServoSensor  = E_EVENT_STS_ALARM_LOCK;	// サーボセンサーイベント
	sAppEventMap.eEvtUnlockReq    = E_EVENT_STS_UNLOCK;		// 通常開錠リクエスト
	sAppEventMap.eEvtLockReq      = 0x00;					// 通常施錠リクエスト
	sAppEventMap.eEvtInCautionReq = E_EVENT_STS_IN_CAUTION;	// 警戒施錠リクエスト
	sAppEventMap.eEvtMstUnlockReq = E_EVENT_STS_MST_UNLOCK;	// マスターパスワード開錠リクエスト

	//==========================================================================
	// ステータス移行処理
	//==========================================================================
	// サーボ制御（ロック）
	vSetServoLock();
	// 指定時間後に5V電源OFFイベント
	sAppEventInfo.u32PwrOffTime = u32TickCount_ms + SERVO_WAIT;
}

/*******************************************************************************
 *
 * NAME: vEvent_StsInCaution
 *
 * DESCRIPTION:警戒施錠状態への移行処理
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_StsInCaution(uint32 u32EvtTimeMs) {
	// ステータス判定
	if (sAppStsInfo.eAppStatus == E_APP_STS_IN_CAUTION) {
		// 既にステータス移行済み
		return;
	}
	//==========================================================================
	// アプリケーションステータス更新
	//==========================================================================
	// ステータス移行中
	sAppStsInfo.bInProgressFlg = TRUE;
	// 警戒施錠状態
	sAppStsInfo.eAppStatus = E_APP_STS_IN_CAUTION;

	//==========================================================================
	// アプリケーションイベントマップ更新
	//==========================================================================
	sAppEventMap.eEvtIRSensor     = E_EVENT_STS_ALARM_LOCK;	// 人感センサーイベント
	sAppEventMap.eEvtTouchSensor  = E_EVENT_STS_ALARM_LOCK;	// タッチセンサーイベント
	sAppEventMap.eEvtOpenSensor   = E_EVENT_STS_ALARM_LOCK;	// 開放センサーイベント
	sAppEventMap.eEvtServoSensor  = E_EVENT_STS_ALARM_LOCK;	// サーボセンサーイベント
	sAppEventMap.eEvtUnlockReq    = E_EVENT_STS_UNLOCK;		// 通常開錠リクエスト
	sAppEventMap.eEvtLockReq      = E_EVENT_STS_LOCK;		// 通常施錠リクエスト
	sAppEventMap.eEvtInCautionReq = 0x00;					// 警戒施錠リクエスト
	sAppEventMap.eEvtMstUnlockReq = E_EVENT_STS_MST_UNLOCK;	// マスターパスワード開錠リクエスト

	//==========================================================================
	// ステータス移行処理
	//==========================================================================
	// サーボ制御（ロック）
	vSetServoLock();
	// 指定時間後に5V電源OFFイベント
	sAppEventInfo.u32PwrOffTime = u32TickCount_ms + SERVO_WAIT;
}

/*******************************************************************************
 *
 * NAME: vEvent_StsAlarmUnlock
 *
 * DESCRIPTION:警報開錠状態への移行処理
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_StsAlarmUnlock(uint32 u32EvtTimeMs) {
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d vEvent_StsAlarmUnlock\n", u32TickCount_ms);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	//==========================================================================
	// アプリケーションステータス更新
	//==========================================================================
	// ステータス移行中
	sAppStsInfo.bInProgressFlg = TRUE;
	// 警報開錠状態
	sAppStsInfo.eAppStatus = E_APP_STS_ALARM_UNLOCK;
	// ステータスマップ更新
	bEEPROMWriteDevInfo(sAppEventInfo.u8UpdStsMap);

	//==========================================================================
	// アプリケーションイベントマップ更新
	//==========================================================================
	sAppEventMap.eEvtIRSensor     = E_EVENT_STS_ALARM_LOG;		// 人感センサーイベント
	sAppEventMap.eEvtTouchSensor  = E_EVENT_STS_ALARM_LOG;		// タッチセンサーイベント
	sAppEventMap.eEvtOpenSensor   = E_EVENT_STS_ALARM_LOG;		// 開放センサーイベント
	sAppEventMap.eEvtServoSensor  = E_EVENT_STS_ALARM_UNLOCK;	// サーボセンサーイベント
	sAppEventMap.eEvtUnlockReq    = 0x00;						// 通常開錠リクエスト
	sAppEventMap.eEvtLockReq      = 0x00;						// 通常施錠リクエスト
	sAppEventMap.eEvtInCautionReq = 0x00;						// 警戒施錠リクエスト
	sAppEventMap.eEvtMstUnlockReq = 0x00;						// マスターパスワード開錠リクエスト

	//==========================================================================
	// イベント処理
	//==========================================================================
	// マスタートークンマスククリア
	vEEPROMTokenMaskClear();
	// エラーログを出力
	iEEPROMWriteLog(sAppEventInfo.u8LogMsgCd, 0x00);
	// サーボ制御（アンロック）
	vSetServoUnlock();
	// 指定時間後に5V電源OFFイベント
	sAppEventInfo.u32PwrOffTime = u32TickCount_ms + SERVO_WAIT;
}

/*******************************************************************************
 *
 * NAME: vEvent_StsAlarmLock
 *
 * DESCRIPTION:警報施錠状態への移行処理
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_StsAlarmLock(uint32 u32EvtTimeMs) {
#ifdef DEBUG
	vfPrintf(&sSerStream, "MS:%08d vEvent_StsAlarmLock\n", u32TickCount_ms);
	SERIAL_vFlush(sSerStream.u8Device);
#endif
	//==========================================================================
	// アプリケーションステータス更新
	//==========================================================================
	// ステータス移行中
	sAppStsInfo.bInProgressFlg = TRUE;
	// 警報施錠状態
	sAppStsInfo.eAppStatus = E_APP_STS_ALARM_LOCK;
	// ステータスマップ更新
	bEEPROMWriteDevInfo(sAppEventInfo.u8UpdStsMap);

	//==========================================================================
	// アプリケーションイベントマップ更新
	//==========================================================================
	sAppEventMap.eEvtIRSensor     = E_EVENT_STS_ALARM_LOG;		// 人感センサーイベント
	sAppEventMap.eEvtTouchSensor  = E_EVENT_STS_ALARM_LOG;		// タッチセンサーイベント
	sAppEventMap.eEvtOpenSensor   = E_EVENT_STS_ALARM_LOG;		// 開放センサーイベント
	sAppEventMap.eEvtServoSensor  = E_EVENT_STS_ALARM_LOCK;		// サーボセンサーイベント
	sAppEventMap.eEvtUnlockReq    = 0x00;						// 通常開錠リクエスト
	sAppEventMap.eEvtLockReq      = 0x00;						// 通常施錠リクエスト
	sAppEventMap.eEvtInCautionReq = 0x00;						// 警戒施錠リクエスト
	sAppEventMap.eEvtMstUnlockReq = E_EVENT_STS_ALARM_UNLOCK;	// マスターパスワード開錠リクエスト

	//==========================================================================
	// ステータス移行処理
	//==========================================================================
	// マスタートークンマスククリア
	vEEPROMTokenMaskClear();
	// エラーログを出力
	iEEPROMWriteLog(sAppEventInfo.u8LogMsgCd, 0x00);
	// サーボ制御（ロック）
	vSetServoLock();
	// 指定時間後に5V電源OFFイベント
	sAppEventInfo.u32PwrOffTime = u32TickCount_ms + SERVO_WAIT;
}

/*******************************************************************************
 *
 * NAME: vEvent_StsAlarmtLog
 *
 * DESCRIPTION:警報ログ出力処理
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_StsAlarmtLog(uint32 u32EvtTimeMs) {
	//==========================================================================
	// 実行判定
	//==========================================================================
	if ((sDevInfo.u8StatusMap & sAppEventInfo.u8UpdStsMap) == sAppEventInfo.u8UpdStsMap) {
		return;
	}
	//==========================================================================
	// エラーログを出力
	//==========================================================================
	// ステータスマップ更新
	bEEPROMWriteDevInfo(sAppEventInfo.u8UpdStsMap);
	// エラーログを出力
	iEEPROMWriteLog(sAppEventInfo.u8LogMsgCd, 0x00);

	//==========================================================================
	// イベントマップ更新
	//==========================================================================
	switch (sAppEventInfo.u8UpdStsMap) {
	case APP_STS_MAP_IR_SENS:
		// 人感センサー
		sAppEventMap.eEvtIRSensor    = 0x00;
		break;
	case APP_STS_MAP_BUTTON:
		// ボタン
		sAppEventMap.eEvtTouchSensor = 0x00;
		break;
	case APP_STS_MAP_OPEN_SENS:
		// 開放センサー
		sAppEventMap.eEvtOpenSensor  = 0x00;
		break;
	case APP_STS_MAP_SERVO_SENS:
		// サーボ位置
		sAppEventMap.eEvtServoSensor = 0x00;
		break;
	}
}

/*******************************************************************************
 *
 * NAME: vEvent_StsMstUnlock
 *
 * DESCRIPTION:マスターパスワード開錠状態への移行処理
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_StsMstUnlock(uint32 u32EvtTimeMs) {
	// ステータス判定
	if (sAppStsInfo.eAppStatus == E_APP_STS_MST_UNLOCK) {
		// 既にステータス移行済み
		return;
	}
	//==========================================================================
	// アプリケーションステータス更新
	//==========================================================================
	// ステータス移行中
	sAppStsInfo.bInProgressFlg = TRUE;
	// マスターパスワード開錠状態
	sAppStsInfo.eAppStatus = E_APP_STS_MST_UNLOCK;

	//==========================================================================
	// アプリケーションイベントマップ更新
	//==========================================================================
	sAppEventMap.eEvtTouchSensor  = 0x00;	// タッチセンサーイベント
	sAppEventMap.eEvtIRSensor     = 0x00;	// 人感センサーイベント
	sAppEventMap.eEvtOpenSensor   = 0x00;	// 開放センサーイベント
	sAppEventMap.eEvtServoSensor  = 0x00;	// サーボセンサーイベント
	sAppEventMap.eEvtUnlockReq    = 0x00;	// 通常開錠リクエスト
	sAppEventMap.eEvtLockReq      = 0x00;	// 通常施錠リクエスト
	sAppEventMap.eEvtInCautionReq = 0x00;	// 警戒施錠リクエスト
	sAppEventMap.eEvtMstUnlockReq = 0x00;	// マスターパスワード開錠リクエスト

	//==========================================================================
	// ステータス移行処理
	//==========================================================================
	// マスタートークンマスククリア
	vEEPROMTokenMaskClear();
	// 2秒後に5V電源OFFイベント
	sAppEventInfo.u32PwrOffTime = u32TickCount_ms + SERVO_WAIT;
	// サーボ制御（アンロック）
	vSetServoUnlock();
}

/*******************************************************************************
 *
 * NAME: vEvent_LCDdrawing
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
 ******************************************************************************/
PUBLIC void vEvent_LCDdrawing(uint32 u32EvtTimeMs) {
	// LCD描画処理
	vLCDdrawing();
}

/*******************************************************************************
 *
 * NAME: vEvent_HashStretching
 *
 * DESCRIPTION:拡張ハッシュストレッチング処理イベントプロセス
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32EvtTimeMs    R   イベント発生時刻
 *
 * RETURNS:
 *
 ******************************************************************************/
PUBLIC void vEvent_HashStretching(uint32 u32EvtTimeMs) {
	// ハッシュ関数実行
	if (bAuth_hashStretching(&sHashGenInfo)) {
		// ハッシュ生成完了時には元のイベントに戻る
		iEntrySeqEvt(sTxRxTrnsInfo.eRtnAppEvt);
	} else {
		// 次回ストレッチング処理
		iEntrySeqEvt(E_EVENT_HASH_ST);
	}
}

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: bEvt_RxDefaultChk
 *
 * DESCRIPTION:受信メッセージ基本チェック
 *
 * PARAMETERS:          Name            RW  Usage
 *   tsRxInfo*          psRxInfo        R   受信メッセージ
 *
 * RETURNS:
 *   TRUE:チェックOK
 *
 ******************************************************************************/
PRIVATE bool_t bEvt_RxDefaultChk(tsRxTxInfo* psRxInfo) {
	//--------------------------------------------------------------------------
	// レスポンスデータチェック
	//--------------------------------------------------------------------------
	// 受信メッセージ
	tsWirelessMsg* psWlsMsg = &psRxInfo->sMsg;
	// CRCチェック
	uint8 u8WkCRC = psWlsMsg->u8CRC;
	psWlsMsg->u8CRC = 0;
	if (u8CCITT8((uint8*)psWlsMsg, TX_REC_SIZE) != u8WkCRC) {
		iEEPROMWriteLog(E_MSG_CD_RX_CRC_ERR, psWlsMsg->u8Command);
		return FALSE;
	}
	// コマンドチェック
	switch (psWlsMsg->u8Command) {
	case E_APP_CMD_CHK_STATUS:	// 送受信コマンド：ステータス要求
	case E_APP_CMD_UNLOCK:		// 送受信コマンド：開錠要求
	case E_APP_CMD_LOCK:		// 送受信コマンド：通常施錠要求
	case E_APP_CMD_ALERT:		// 送受信コマンド：警戒施錠要求
	case E_APP_CMD_MST_UNLOCK:	// 送受信コマンド：マスター開錠要求
		break;
	default:
		iEEPROMWriteLog(E_MSG_CD_RX_CMD_ERR, psWlsMsg->u8Command);
		return FALSE;
	}
	// チェックOK
	return TRUE;
}

/*******************************************************************************
 *
 * NAME: u32Evt_getElapsedTime
 *
 * DESCRIPTION:経過時間の算出処理
 *
 * PARAMETERS:           Name            RW  Usage
 * tsAuthRemoteDevInfo*  psRemoteInfo    R   リモートデバイス情報
 * tsWirelessMsg*        psRxMsg         R   受信メッセージ
 *
 * RETURNS:
 *   経過時間（分単位）
 *
 ******************************************************************************/
PRIVATE uint32 u32Evt_getElapsedTime(tsAuthRemoteDevInfo* psRemoteInfo, DS3231_datetime* psDateTime) {
	uint32 u32To = u32ValUtil_dateToDays(psDateTime->u16Year, psDateTime->u8Month, psDateTime->u8Day);
	u32To = u32To * 24 * 60;
	u32To = u32To + psDateTime->u8Hour * 60 + psDateTime->u8Minutes;
	return u32To - psRemoteInfo->u32StartDateTime;
}

/*******************************************************************************
 *
 * NAME: bEvt_TxResponse
 *
 * DESCRIPTION:認証なしのACK/NACK返信送信
 *
 * PARAMETERS:        Name            RW  Usage
 *   teAppCommand     eCommand        R   返信コマンド
 *
 * RETURNS:
 *   TRUE:返信成功
 *
 ******************************************************************************/
PRIVATE bool_t bEvt_TxResponse(teAppCommand eCommand, bool_t bEncryption) {
	// 電文の編集
	tsWirelessMsg sWlsMsg;
	memset(&sWlsMsg, 0x00, sizeof(tsWirelessMsg));
	sWlsMsg.u32DstAddr  = sTxRxTrnsInfo.u32DstAddr;		// 宛先アドレス
	sWlsMsg.u8Command   = eCommand;						// ACK/NACKコマンド
	sWlsMsg.u16Year     = sAppIO.sDatetime.u16Year;		// 年
	sWlsMsg.u8Month     = sAppIO.sDatetime.u8Month;		// 月
	sWlsMsg.u8Day       = sAppIO.sDatetime.u8Day;		// 日
	sWlsMsg.u8Hour      = sAppIO.sDatetime.u8Hour;		// 時
	sWlsMsg.u8Minute    = sAppIO.sDatetime.u8Minutes;	// 分
	sWlsMsg.u8Second    = sAppIO.sDatetime.u8Seconds;	// 秒
	sWlsMsg.u8StatusMap = sDevInfo.u8StatusMap;			// ステータスマップ
	// CRC8編集
	sWlsMsg.u8CRC = u8CCITT8((uint8*)&sWlsMsg, TX_REC_SIZE);
	// 電文の送信
	if (bWirelessTxEnq(sDevInfo.u32DeviceID, bEncryption, &sWlsMsg) ==  FALSE) {
		// エラー処理
		iEEPROMWriteLog(E_MSG_CD_TX_ERR, eCommand);
		return FALSE;
	}
	// イベントタスク登録：メッセージ送信
	return (iEntrySeqEvt(E_EVENT_TX_DATA) >= 0);
}

/*******************************************************************************
 *
 * NAME: vEvt_BeginTxRxTrns
 *
 * DESCRIPTION:通信トランザクション開始
 *
 * PARAMETERS:        Name            RW  Usage
 * tsAppTxRxTrnsInfo* psTxRxTrnsInfo  R   通信トランザクション情報
 * tsRxTxInfo*        psRxInfo        R   リクエスト情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PRIVATE void vEvt_BeginTxRxTrns(tsAppTxRxTrnsInfo* psTxRxTrnsInfo, tsRxTxInfo* psRxInfo) {
	// 送受信トランザクション情報の初期化
	memset(psTxRxTrnsInfo, 0x00, sizeof(tsAppTxRxTrnsInfo));
	psTxRxTrnsInfo->sRefDatetime = sAppIO.sDatetime;	// 基準時刻
	psTxRxTrnsInfo->u32DstAddr   = psRxInfo->u32Addr;	// 送信元アドレス
	psTxRxTrnsInfo->sRxWlsMsg    = psRxInfo->sMsg;		// 受信メッセージ
}

/*******************************************************************************
 *
 * NAME: vEvt_EndTxRxTrns
 *
 * DESCRIPTION:通信トランザクション終了
 *
 * PARAMETERS:        Name            RW  Usage
 * tsAppTxRxTrnsInfo* psTxRxTrnsInfo  R   通信トランザクション情報
 *
 * RETURNS:
 *
 ******************************************************************************/
PRIVATE void vEvt_EndTxRxTrns(tsAppTxRxTrnsInfo* psTxRxTrnsInfo) {
	// トランザクションデータクリア
	memset(&sTxRxTrnsInfo, 0x00, sizeof(tsAppTxRxTrnsInfo));
	// 受信待ち有効化
	vWirelessRxEnabled(sDevInfo.u32DeviceID);
}
#ifdef DEBUG
/*******************************************************************************
 *
 * NAME: vConv_ToStr
 *
 * DESCRIPTION:配列の文字列化
 *
 * PARAMETERS:        Name            RW  Usage
 *     uint8*         pu8Src          R   文字列化対象データ
 *     char*          pcStr           W   文字列編集対象
 *     uint8          u8Size          R   文字列サイズ
 *
 * RETURNS:
 *
 ******************************************************************************/
PRIVATE void vConv_ToStr(uint8* pu8Src, char* pcStr, uint8 u8Size) {
	char *pcVals = "0123456789ABCDEF";
	uint8 u8Idx;
	for (u8Idx= 0; u8Idx < u8Size; u8Idx++) {
		pcStr[u8Idx * 2] = pcVals[pu8Src[u8Idx] / 16];
		pcStr[u8Idx * 2 + 1] = pcVals[pu8Src[u8Idx] % 16];
	}
	pcStr[u8Idx * 2] = '\0';
}
#endif

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
