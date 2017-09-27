/****************************************************************************
 *
 * MODULE :The definition of the various set values
 *
 * CREATED:2016/01/05 06:02:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:共通性の高い各種設定値を定義
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
#ifndef  CONFIG_H_INCLUDED
#define  CONFIG_H_INCLUDED

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
/* Debug Mode Setting */
//#define DEBUG
/* Stack over flow Function */
//#define USER_DEFINE_STACK_OVER_FLOW

/* Serial Configuration */
#define UART_BAUD   		 (115200)
#define UART_PARITY_ENABLE	 E_AHI_UART_PARITY_DISABLE
#define UART_PARITY_TYPE 	 E_AHI_UART_ODD_PARITY // if enabled
#define UART_BITLEN			 E_AHI_UART_WORD_LEN_8
#define UART_STOPBITS 		 E_AHI_UART_1_STOP_BIT

/* Specify which serial port to use when outputting debug information */
#define UART_PORT_MASTER     E_AHI_UART_0 // for Coordinator
#define UART_PORT_SLAVE      E_AHI_UART_0 // for End Device

/* Specify the PAN ID and CHANNEL to be used by tags, readers and gateway */
#define APP_ID               (0x07770001UL)
// チャンネルマスク
#define CHANNEL_MASK         (0x07fff800UL)
// チャンネル（11-25,26）
#define CHANNEL              (18)
// 送信出力（3:最大, 2:-11.5db, 1:-23db, 0:-34.5db）
#define TX_PWR               (3)
// 受信バッファサイズ
#define RX_BUFFER_SIZE       (3)
// 送信バッファサイズ
#define TX_BUFFER_SIZE       (3)

/******************** ハードウェアセッティング ********************/
// キー参照回数
#define APP_CMN_KEY_REF_CNT        (4)
// キー参照時の許容値
#define APP_CMN_KEY_TOLERANCE      (15)
// LCD Contrast
#define LCD_CONTRAST               (45)

/******************** ソフトウェアセッティング ********************/
// 先頭アドレス：デバイス情報
#define TOP_ADDR_DEV               (0x0000)
// 先頭アドレス：インデックス
#define TOP_ADDR_INDEX             (0x0040)
// 先頭アドレス：リモートデバイス
#define TOP_ADDR_REMOTE_DEV        (0x0080)
// 先頭アドレス：イベントログ
#define TOP_ADDR_EVENT_LOG         (0x1080)
// リモートデバイス数
#define MAX_REMOTE_DEV_CNT         (32)
// イベントログサイズ
#define MAX_EVT_LOG_CNT            (128)
// Stretching Count
#define STRETCHING_CNT_BASE        (1920)

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

#endif  /* CONFIG_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
