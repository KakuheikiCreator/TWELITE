/****************************************************************************
 *
 * MODULE :I2C Utility functions source file
 *
 * CREATED:2015/03/22 08:32:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:
 *   I2C接続機能を提供するユーティリティ関数群
 *   I2C Utility functions (source file)
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

#include "i2c_util.h"
#include "timer_util.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// I2C Acceess Interval
#define I2C_INTERVAL               (100)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/**
 * I2Cバス情報
 */
typedef struct {
	uint8 u8PreScaler;		// 動作周波数
	uint64 u64LastStart;	// 最終アクセス時刻
} tsI2C_state;

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
// 通信設定：書き込み開始、ACK返信
PRIVATE const bool_t I2C_START_WRITE_ACK[] = {
		E_AHI_SI_START_BIT, E_AHI_SI_NO_STOP_BIT,			// 開始終了
		E_AHI_SI_NO_SLAVE_READ, E_AHI_SI_SLAVE_WRITE,		// 読み書き
		E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK				// ACK/NACKとタイプ
};
// 通信設定：書き込み開始、NACK返信
//PRIVATE const bool_t I2C_START_WRITE_NACK[] = {
//		E_AHI_SI_START_BIT, E_AHI_SI_NO_STOP_BIT,			// 開始終了
//		E_AHI_SI_NO_SLAVE_READ, E_AHI_SI_SLAVE_WRITE,		// 読み書き
//		E_AHI_SI_SEND_NACK, E_AHI_SI_NO_IRQ_ACK				// ACK/NACKとタイプ
//};
// 通信設定：読み込み開始、ACK返信
//PRIVATE const bool_t I2C_START_READ_ACK[] = {
//		E_AHI_SI_START_BIT, E_AHI_SI_NO_STOP_BIT,			// 開始終了
//		E_AHI_SI_SLAVE_READ, E_AHI_SI_NO_SLAVE_WRITE,		// 読み書き
//		E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK				// ACK/NACKとタイプ
//};
// 通信設定：読み込み開始、NACK返信
//PRIVATE const bool_t I2C_START_READ_NACK[] = {
//		E_AHI_SI_START_BIT,	E_AHI_SI_NO_STOP_BIT,			// 開始終了
//		E_AHI_SI_SLAVE_READ, E_AHI_SI_NO_SLAVE_WRITE,		// 読み書き
//		E_AHI_SI_SEND_NACK, E_AHI_SI_NO_IRQ_ACK				// ACK/NACKとタイプ
//};
// 通信設定：書き込みACK返信
PRIVATE const bool_t I2C_CONT_WRITE_ACK[] = {
		E_AHI_SI_NO_START_BIT, E_AHI_SI_NO_STOP_BIT,		// 開始終了
		E_AHI_SI_NO_SLAVE_READ, E_AHI_SI_SLAVE_WRITE,		// 読み書き
		E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK				// ACK/NACKとタイプ
};
// 通信設定：書き込みNACK返信
//PRIVATE const bool_t I2C_CONT_WRITE_NACK[] = {
//		E_AHI_SI_NO_START_BIT, E_AHI_SI_NO_STOP_BIT,		// 開始終了
//		E_AHI_SI_NO_SLAVE_READ, E_AHI_SI_SLAVE_WRITE,		// 読み書き
//		E_AHI_SI_SEND_NACK, E_AHI_SI_NO_IRQ_ACK				// ACK/NACKとタイプ
//};
// 通信設定：読み込みACK返信
PRIVATE const bool_t I2C_CONT_READ_ACK[] = {
		E_AHI_SI_NO_START_BIT, E_AHI_SI_NO_STOP_BIT,		// 開始終了
		E_AHI_SI_SLAVE_READ, E_AHI_SI_NO_SLAVE_WRITE,		// 読み書き
		E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK				// ACK/NACKとタイプ
};
// 通信設定：読み込みNACK返信
PRIVATE const bool_t I2C_CONT_READ_NACK[] = {
		E_AHI_SI_NO_START_BIT, E_AHI_SI_NO_STOP_BIT,		// 開始終了
		E_AHI_SI_SLAVE_READ, E_AHI_SI_NO_SLAVE_WRITE,		// 読み書き
		E_AHI_SI_SEND_NACK, E_AHI_SI_NO_IRQ_ACK				// ACK/NACKとタイプ
};
// 通信設定：読み書き停止、ACK返信
PRIVATE const bool_t I2C_STOP_ACK[] = {
		E_AHI_SI_NO_START_BIT, E_AHI_SI_STOP_BIT,			// 開始終了
		E_AHI_SI_NO_SLAVE_READ, E_AHI_SI_SLAVE_WRITE,		// 読み書き
		E_AHI_SI_SEND_ACK, E_AHI_SI_NO_IRQ_ACK				// ACK/NACKとタイプ
};
// 通信設定：読み書き停止、NACK返信
PRIVATE const bool_t I2C_STOP_NACK[] = {
		E_AHI_SI_NO_START_BIT, E_AHI_SI_STOP_BIT,			// 開始終了
		E_AHI_SI_NO_SLAVE_READ, E_AHI_SI_SLAVE_WRITE,		// 読み書き
		E_AHI_SI_SEND_NACK, E_AHI_SI_NO_IRQ_ACK				// ACK/NACKとタイプ
};

// I2Cバス情報
PRIVATE tsI2C_state sI2C_state;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
// 通信間隔制御
PRIVATE void vI2C_intervalWait();
// 送受信確定
PRIVATE bool_t bI2C_transmit(const bool_t* pbPrms);

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/*****************************************************************************
 *
 * NAME: vI2C_init
 *
 * DESCRIPTION:マスターとしてI2Cバス接続する際の初期設定処理
 *
 * PARAMETERS:      Name            RW Usage
 *   uint8          u8PreScaler     R  動作周波数 = 16/[(u8PreScaler + 1) x 5] MHz
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vI2C_init(uint8 u8PreScaler) {
	// バス駆動周波数設定
	sI2C_state.u8PreScaler  = u8PreScaler;
	// 最終アクセス開始時刻
	sI2C_state.u64LastStart = u64TimerUtil_readUsec();
	// マスターとしてI2C接続設定
	// 割り込み無し
	// 16/[(PreScaler + 1) x 5]MHz
	//		--> 7:400KHz, 31:100KHz, 47:66KHz
	vAHI_SiMasterConfigure(TRUE, FALSE, u8PreScaler);
}

/*****************************************************************************
 *
 * NAME: bI2C_startRead
 *
 * DESCRIPTION:I2Cバスからの読み込み開始処理
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u8Address		R	I2Cアドレス
 *
 * RETURNS:
 *     TRUE ：通信が成功した
 *     FALSE：通信が失敗した
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bI2C_startRead(uint8 u8Address) {
	// 通信間隔制御
	vI2C_intervalWait();
	// 通信相手のスレーブ選択
	vAHI_SiMasterWriteSlaveAddr(u8Address, TRUE);
	// I2Cバスの通信処理：受信対象アドレス送信し、読み込みを開始する
	return bI2C_transmit(I2C_START_WRITE_ACK);
}

/*****************************************************************************
 *
 * NAME: bI2C_startWrite
 *
 * DESCRIPTION:I2Cバスへの書き込み開始処理
 *
 * PARAMETERS:Name          RW  Usage
 *            u8Address		R	I2Cアドレス
 *
 * RETURNS:
 *     TRUE ：通信が成功した
 *     FALSE：通信が失敗した
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bI2C_startWrite(uint8 u8Address) {
	// 通信間隔制御
	vI2C_intervalWait();
	// 送信先のスレーブアドレス指定
	vAHI_SiMasterWriteSlaveAddr(u8Address, FALSE);
	// I2Cバスの通信処理：送信開始通知
	return bI2C_transmit(I2C_START_WRITE_ACK);
}

/*****************************************************************************
 *
 * NAME: bI2C_read
 *
 * DESCRIPTION:I2Cバスからの読み込み処理
 *
 * PARAMETERS:      Name            RW  Usage
 *                  pu8Data         W   読み込み先データ領域へのポインタ
 *                  u8Length        R   読み出しバイト数 (0 なら、付随データなし)
 *                  bAckEndFlg      R   終端ACK返信フラグ（TRUE:ACK返信で終了）
 *
 * RETURNS:
 *     TRUE ：通信が成功した
 *     FALSE：通信が失敗した
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bI2C_read(uint8* pu8Data, uint8 u8Length, bool_t bAckEndFlg) {
	if (u8Length == 0) return TRUE;
	// 受信ループ
	int idx;
	for (idx = 0; idx < (u8Length - 1); idx++) {
		// I2Cバスの通信処理：受信通知
		if(!bI2C_transmit(I2C_CONT_READ_ACK)) return FALSE;
		// データの読み込み
		pu8Data[idx] = u8AHI_SiMasterReadData8();
	}
	if (bAckEndFlg) {
		// I2Cバスの通信処理：受信通知（ACK返信）
		if(!bI2C_transmit(I2C_CONT_READ_ACK)) return FALSE;
	} else {
		// I2Cバスの通信処理：受信通知（NACK返信）
		if(!bI2C_transmit(I2C_CONT_READ_NACK)) return FALSE;
	}
	// 最終データの読み込み
	pu8Data[idx] = u8AHI_SiMasterReadData8();
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: u8I2C_write
 *
 * DESCRIPTION:I2Cバスへの書き込み処理、後続の書き込みがある前提で書き込みを行う
 *
 * PARAMETERS:Name          RW  Usage
 *            pu8Data 		R	書き込みデータ
 *
 * RETURNS:
 *     I2CUTIL_STS_ACK (TRUE) ：ACK返信
 *     I2CUTIL_STS_NACK(FALSE)：NACK返信
 *     I2CUTIL_STS_ERR (0xFF) ：通信エラー発生
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint8 u8I2C_write(uint8 u8Data) {
	// データの送信
	vAHI_SiMasterWriteData8(u8Data);
	// I2Cバスの通信処理：送信通知
	if (!bI2C_transmit(I2C_CONT_WRITE_ACK)) return I2CUTIL_STS_ERR;
	return (bAHI_SiMasterCheckRxNack() == FALSE);
}

/*****************************************************************************
 *
 * NAME: u8I2C_writeStop
 *
 * DESCRIPTION:I2Cバスへの書き込み処理、後続の書き込みが無い終端バイトとして書き込みを行う
 *
 * PARAMETERS:Name          RW  Usage
 *            pu8Data 		R	書き込みデータ
 *
 * RETURNS:
 *     I2CUTIL_STS_ACK (TRUE) ：ACK返信
 *     I2CUTIL_STS_NACK(FALSE)：NACK返信
 *     I2CUTIL_STS_ERR (0xFF) ：通信エラー発生
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint8 u8I2C_writeStop(uint8 u8Data) {
	// データの送信
	vAHI_SiMasterWriteData8(u8Data);
	// I2Cバスの通信処理：送信通知
	if (!bI2C_transmit(I2C_STOP_ACK)) return I2CUTIL_STS_ERR;
	return (bAHI_SiMasterCheckRxNack() == FALSE);
}

/*****************************************************************************
 *
 * NAME: bI2C_stopACK
 *
 * DESCRIPTION:I2Cバスとの通信完了処理（ACK返信）
 *
 * PARAMETERS:Name          RW  Usage
 *
 * RETURNS:
 *     TRUE ：通信が成功した
 *     FALSE：通信が失敗した
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bI2C_stopACK() {
	// I2Cバスの通信処理：読み書き完了通知
	return bI2C_transmit(I2C_STOP_ACK);
}

/*****************************************************************************
 *
 * NAME: bI2C_stopNACK
 *
 * DESCRIPTION:I2Cバスとの通信完了処理（NACK返信）
 *
 * PARAMETERS:Name          RW  Usage
 *
 * RETURNS:
 *     TRUE ：通信が成功した
 *     FALSE：通信が失敗した
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bI2C_stopNACK() {
	// I2Cバスの通信処理：読み書き完了通知
	return bI2C_transmit(I2C_STOP_NACK);
}

/*****************************************************************************
 *
 * NAME: u32I2C_getFrequency
 *
 * DESCRIPTION:I2Cバス駆動周波数の取得処理
 *
 * PARAMETERS:Name          RW  Usage
 *
 * RETURNS:
 *     uint32：駆動周波数
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint32 u32I2C_getFrequency() {
	// バス駆動周波数設定
	return 3200000 / (sI2C_state.u8PreScaler + 1);
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/*****************************************************************************
 *
 * NAME: vI2C_intervalWait
 *
 * DESCRIPTION:通信間隔制御
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE void vI2C_intervalWait() {
	uint64 u64EndTime = sI2C_state.u64LastStart + I2C_INTERVAL;
	uint64 u64NowTime;
	do {
		u64NowTime = u64TimerUtil_readUsec();
	} while(u64NowTime < u64EndTime);
	sI2C_state.u64LastStart = u64TimerUtil_readUsec();
}

/*****************************************************************************
 *
 * NAME: bI2C_determine
 *
 * DESCRIPTION:送受信バッファの転送処理
 *
 * PARAMETERS:      Name            RW  Usage
 *                  prms            R   送受信設定
 *
 * RETURNS:
 *     TRUE ：通信が成功した
 *     FALSE：通信が失敗した
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE bool_t bI2C_transmit(const bool_t* pbPrms) {
	// 転送処理
	if (!bAHI_SiMasterSetCmdReg(pbPrms[0], pbPrms[1], pbPrms[2], pbPrms[3], pbPrms[4], pbPrms[5])) return FALSE;
	// I2Cバスの通信処理待ち
	while (bAHI_SiMasterPollTransferInProgress());
	// マルチマスタ時の通信バス調停失敗判定
	if (bAHI_SiMasterPollArbitrationLost()) {
		// I2Cバスの通信処理：送信停止
		const bool_t *prms = I2C_STOP_ACK;
		if (!bAHI_SiMasterSetCmdReg(prms[0], prms[1], prms[2], prms[3], prms[4], prms[5])) return FALSE;
		// I2Cバスの通信処理待ち
		while (bAHI_SiMasterPollTransferInProgress());
		return FALSE;
	}
	return TRUE;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
