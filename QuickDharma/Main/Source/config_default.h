/****************************************************************************
 *
 * MODULE :It defines the standard value of various set values
 *
 * CREATED:2018/05/22 21:24:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:共通性の高い各種設定の固定値とデフォルト値を定義
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
#ifndef  CONFIG_DEFAULT_H_INCLUDED
#define  CONFIG_DEFAULT_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <AppHardwareApi.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
//============================================================================
// ToCoNet Config Defaults
//============================================================================
//----------------------------------------------------------------------------
// Network Config Defaults
//----------------------------------------------------------------------------
/* Serial Configuration */
/** 通信レート：115200bps */
#ifndef UART_BAUD
	#define UART_BAUD   		 (115200)
#endif
/** パリティチェック有効無効：無効 */
#ifndef UART_PARITY_ENABLE
	#define UART_PARITY_ENABLE	 E_AHI_UART_PARITY_DISABLE
#endif
/** パリティチェックタイプ：奇数パリティ */
#ifndef UART_PARITY_TYPE
	#define UART_PARITY_TYPE 	 E_AHI_UART_ODD_PARITY
#endif
/** ワード長：８ビット */
#ifndef UART_BITLEN
	#define UART_BITLEN			 E_AHI_UART_WORD_LEN_8
#endif
/** ストップビット：1ビット */
#ifndef UART_STOPBITS
	#define UART_STOPBITS 		 E_AHI_UART_1_STOP_BIT
#endif

/* Specify which serial port to use when outputting debug information */
/** マスターポート： */
#ifndef UART_PORT_MASTER
	#define UART_PORT_MASTER     E_AHI_UART_0 // for Coordinator
#endif
/** スレーブポート： */
#ifndef UART_PORT_SLAVE
	#define UART_PORT_SLAVE      E_AHI_UART_0 // for End Device
#endif

/* Specify the PAN ID and CHANNEL to be used by tags, readers and gateway */
/** アプリケーションID */
#ifndef APP_ID
	#define APP_ID               (0x07770001UL)
#endif
/** チャンネルマスク */
#ifndef CHANNEL_MASK
	#define CHANNEL_MASK         (0x07fff800UL)
#endif
/** チャンネル（11-25,26） */
#ifndef CHANNEL
	#define CHANNEL              (18)
#endif
/** 送信出力（3:最大, 2:-11.5db, 1:-23db, 0:-34.5db） */
#ifndef TX_PWR
	#define TX_PWR               (3)
#endif

//============================================================================
// Nakanohito Config Defaults
//============================================================================
//----------------------------------------------------------------------------
// Framework Config Defaults
//----------------------------------------------------------------------------
/** ユーザー定義イベントタスク数 */
#ifndef APP_EVENT_TASK_SIZE
	// デフォルトで最大で32個とする
	#define APP_EVENT_TASK_SIZE        32
#endif

/** 最大数：スケジュール実行イベント */
#ifndef APP_SCHEDULE_EVT_SIZE
	// デフォルトで最大で20個とする
	#define APP_SCHEDULE_EVT_SIZE      20
#endif

/** 最大数：順次実行イベント */
#ifndef APP_SEQUENTIAL_EVT_SIZE
	// デフォルトで最大で20個とする
	#define APP_SEQUENTIAL_EVT_SIZE    20
#endif

/** ユーザー定義タスクの実行単位（1サイクルで処理が起動され続ける時間） */
#ifndef APP_EVENT_TIMEOUT
	// デフォルトで最大40ms
	#define APP_EVENT_TIMEOUT          40
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* CONFIG_DEFAULT_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
