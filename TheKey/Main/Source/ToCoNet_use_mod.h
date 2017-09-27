/****************************************************************************
 *
 * MODULE :The definition of the various set values
 *
 * CREATED:2016/04/30 16:06:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:使用するToCoNetのモジュールを定義
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
#ifndef  TOCONET_USE_MOD_H_INCLUDED
#define  TOCONET_USE_MOD_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// Select Modules (define befor include "ToCoNet.h")
//#define ToCoNet_USE_MOD_ENERGYSCAN				// チャンネル入力レベル計測
//#define ToCoNet_USE_MOD_NBSCAN					// 近隣モジュール探索（マスタ）
//#define ToCoNet_USE_MOD_NBSCAN_SLAVE				// 近隣モジュール探索（スレーブ）
//#define ToCoNet_USE_MOD_RAND_MT					// 乱数生成（MT法）
//#define ToCoNet_USE_MOD_RAND_XOR_SHIFT			// 乱数生成（XOR Shift）
#define ToCoNet_USE_MOD_DUPCHK					// パケットの重複チェッカ
//#define ToCoNet_USE_MOD_CHANNEL_MGR				// チャンネルアジリティ利用、複数チャンネル駆動時に使用
#define ToCoNet_USE_MOD_NWK_MESSAGE_POOL		// メッセージプール機能
//#define ToCoNet_USE_MOD_NWK_LAYERTREE				// レイヤーツリー型ネットワーク層
//#define ToCoNet_USE_MOD_NWK_LAYERTREE_MININODES	// レイヤーツリー型ネットワーク層（同報送信用ミニノード）
#define ToCoNet_USE_MOD_NWK_TXRXQUEUE_SMALL		// 送信キュー（サイズ：3）
//#define ToCoNet_USE_MOD_NWK_TXRXQUEUE_MID			// 送信キュー（サイズ：6）
//#define ToCoNet_USE_MOD_NWK_TXRXQUEUE_BIG			// 送信キュー（サイズ：20）

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

#endif  /* TOCONET_MOD_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
