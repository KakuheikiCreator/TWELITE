/******************************************************************************
 *
 * MODULE :AES functions header file
 *
 * CREATED:2018/01/27 22:58:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:AES functions (header file)
 *             AES暗号の暗号化と復号化を行う関数群です。
 *             対応するキーサイズは128bit、192bit、256bit
 *             ブロック暗号なので、余白分の対応としてPKCS#7の機能を提供する。
 *             仕様書はNISTのサイトを参照
 *             FIPS-197(http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf)
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 ******************************************************************************
 * Copyright (c) 2018, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 *****************************************************************************/
#ifndef AES_H_INCLUDED
#define AES_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdio.h>
#include <jendefs.h>


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/** Block Length (Fixed length) */
#define AES_BLOCK_LEN          (16)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/**
 * Crypto usage mode
 * 暗号利用モード
 */
typedef enum {
    AES_CIPHER_MODE_ECB        // ECB Mode
  , AES_CIPHER_MODE_CBC        // CBC Mode
} teAES_CipherMode;

/**
 * Key Length
 * 鍵長
 */
typedef enum {
	AES_KEY_LEN_128 = 16       // 128bit
  , AES_KEY_LEN_192 = 24       // 192bit
  , AES_KEY_LEN_256 = 32       // 256bit
} teAES_KeyLength;

/**
 * AESによる暗号化処理状態
 */
typedef struct {
	teAES_CipherMode mode;
	teAES_KeyLength keyLen;
	uint8 u8Nk;
	uint8 u8Nr;
	uint8 u8RoundKey[240];
	uint8 u8Vector[AES_BLOCK_LEN];
} tsAES_state;


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/**
 * AES処理ステータスを保持する変数の生成
 *
 * @param teAES_KeyLength 鍵長
 * @param uint8_t* 鍵
 * @return tsAES_state ステータス
 */
PUBLIC tsAES_state vAES_newECBState(teAES_KeyLength eKeyLen, uint8* u8Key);

/**
 * AES処理ステータスを保持する変数の生成
 *
 * @param teAES_KeyLength 鍵長
 * @param uint8_t* 鍵
 * @param uint8_t* 初期ベクトル
 * @return tsAES_state ステータス
 */
PUBLIC tsAES_state vAES_newCBCState(teAES_KeyLength eKeyLen, uint8* u8Key, uint8* u8Iv);

/**
 * ベクトル設定処理
 *
 * @param tsAES_state ステータス
 * @param uint8_t* ベクトル
 */
PUBLIC void vAES_setVector(tsAES_state* sState, uint8* u8Vector);

/**
 * 暗号化処理
 *
 * @param tsAES_state ステータス
 * @param uint8_t* 暗号化対象
 * @param uint32 暗号化対象バイト数
 */
PUBLIC void vAES_encrypt(tsAES_state* sState, uint8* u8Buff, const uint32 u32Len);

/**
 * 暗号化処理(PKCS#7でパディング)
 *
 * @param tsAES_state ステータス
 * @param uint8_t* 暗号化対象
 * @param uint32 暗号化対象バイト数
 */
PUBLIC void vAES_encryptPad(tsAES_state* sState, uint8* u8Buff, const uint32 u32Len);

/**
 * 復号化処理
 *
 * @param tsAES_state ステータス
 * @param uint8_t* 暗号化対象
 * @param uint32 暗号化対象バイト数
 */
PUBLIC void vAES_decrypt(tsAES_state* sState, uint8* u8Buff, const uint32 u32Len);

#if defined __cplusplus
}
#endif

#endif  /* AES_H_INCLUDED */

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
