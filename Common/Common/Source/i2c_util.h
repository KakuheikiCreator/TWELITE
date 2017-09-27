/****************************************************************************
 *
 * MODULE :I2C Utility functions header file
 *
 * CREATED:2015/03/22 08:32:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:
 *   I2C接続機能を提供するユーティリティ関数群
 *   I2C Utility functions (header file)
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
#ifndef  I2CUTIL_H_INCLUDED
#define  I2CUTIL_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// I2C Execution result status : ACK
#define  I2CUTIL_STS_ACK         TRUE
// I2C Execution result status : NACK
#define  I2CUTIL_STS_NACK        FALSE
// I2C Execution result status : ERROR
#define  I2CUTIL_STS_ERR         (0xFF)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
// 初期化処理
PUBLIC void vI2C_init(uint8 u8PreScaler);
// 読み込み開始
PUBLIC bool_t bI2C_startRead(uint8 u8Address);
// 書き込み開始処理
PUBLIC bool_t bI2C_startWrite(uint8 u8Address);
// 読み込み処理(継続)
PUBLIC bool_t bI2C_read(uint8* pu8Data, uint8 u8Length, bool_t bAckEndFlg);
// 書き込み処理(継続)
PUBLIC uint8 u8I2C_write(uint8 u8Data);
// 書き込み処理(終端)
PUBLIC uint8 u8I2C_writeStop(uint8 u8Data);
// 読み書き終了処理（ACK返信）
PUBLIC bool_t bI2C_stopACK();
// 読み書き終了処理（NACK返信）
PUBLIC bool_t bI2C_stopNACK();
// バス駆動周波数参照
PUBLIC uint32 u32I2C_getFrequency();

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* I2CUTIL_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

