/******************************************************************************
 *
 * MODULE :AES functions source file
 *
 * CREATED:2018/01/27 22:58:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:AES functions
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
/*****************************************************************************/
/***     Include files                                                     ***/
/*****************************************************************************/
#include "jendefs.h"
#include <AppHardwareApi.h>

#include "string.h"
#include "aes.h"
#include "framework.h"


/*****************************************************************************/
/***        Macro Definitions                                              ***/
/*****************************************************************************/
/**
 * Number of columns (32-bit words) comprising the State.
 * 定数：暗号化単位のブロック（128bit）のワード数
 */
#define Nb 4

/**
 * Nk:Number of 32-bit words comprising the Cipher Key.
 * Nr:Number of rounds, which is a function of Nk and Nb.
 */
#define Nk_128                 (4)
#define Nr_128                 (10)
#define Nk_192                 (6)
#define Nr_192                 (12)
#define Nk_256                 (8)
#define Nr_256                 (14)

/*****************************************************************************/
/***        Type Definitions                                               ***/
/*****************************************************************************/

/*****************************************************************************/
/***        Exported Variables                                             ***/
/*****************************************************************************/

/*****************************************************************************/
/***        Local Variables                                                ***/
/*****************************************************************************/

/**
 * Non-linear substitution table used in several byte substitution transformations
 * and in the Key Expansion routine to perform a one-for-one substitution of a byte value.
 * バイト単位に実行される非線形な値への置換処理で利用されるテーブル
 */
static const uint8 S_BOX[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

/**
 * inverse S-BOX.
 * S-BOXを使用して変換された値からの逆変換する際に利用されるテーブル
 */
static const uint8 INV_S_BOX[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

// The round constant word array, Rcon[i], contains the values given by
// x to the power (i-1) being powers of x (x is denoted as {02}) in the field GF(2^8)
/**
 * The round constant word array.
 * ラウンド定数
 */
static const uint8 RCON[11] = {
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

/*****************************************************************************/
/***        Local Function Prototypes                                      ***/
/*****************************************************************************/
/** Initialize Round Key */
static void vInitRoundKey(tsAES_state* sState, uint8* u8Key);
/** Encryption processing. */
static void vCiphertext(tsAES_state* sState, uint8* u8Buff);
/** Series of transformations that converts ciphertext to plaintext using the Cipher Key. */
static void vInvCipher(tsAES_state* sState, uint8* u8Buff);
/** Transformation in the Cipher and Inverse Cipher in which a Round Key is added to the State using an XOR operation. */
static void vAddRoundKey(uint8* u8Buff, tsAES_state* sState, uint8 u8Round);
/** Block unit XOR */
static void vXorWithBlk(uint8* u8Buff, uint8* u8Block);
/** Word rotation processing */
static void vRotWord(uint8* u8Word);
/** Conversion of values using S-box */
static void vSubBytes(uint8* u8Buff);
/** Convert S-Box Word */
static void vSubWord(uint8* u8Word);
/** Shift Rows */
static void vShiftRows(uint8* u8Buff);
/**
 * Transformation in the Cipher that takes all of the columns of the State and mixes their data
 * (independently of one another) to produce new columns.
 */
static void vMixColumns(uint8* u8Buff);
/** Convert Byte */
static uint8 vXtime(uint8 u8Val);
/** Conversion of values using Inverse S-box */
static void vInvSubBytes(uint8* u8Buff);
/** Transformation in the Inverse Cipher that is the inverse of vShiftRows(). */
static void vInvShiftRows(uint8* u8Buff);
/** Transformation in the Inverse Cipher that is the inverse of vMixColumns(). */
static void vInvMixColumns(uint8* u8Buff);
/**  */
static uint8 vMultiply(uint8 x, uint8 y);

/*****************************************************************************/
/***        Exported Functions                                             ***/
/*****************************************************************************/

/**
 * AES処理ステータスを保持する変数の生成
 *
 * @param teAES_KeyLength 鍵長
 * @param uint8* 鍵
 * @return tsAES_state ステータス
 */
PUBLIC tsAES_state vAES_newECBState(teAES_KeyLength eKeyLen, uint8* u8Key) {
	// 暗号化ステータス
	tsAES_state sState;
	// ECBモード
	sState.mode = AES_CIPHER_MODE_ECB;
	// 鍵長
	sState.keyLen = eKeyLen;
	// NkとNrの初期化
	switch (eKeyLen) {
	case AES_KEY_LEN_128:
		sState.u8Nk = Nk_128;
		sState.u8Nr = Nr_128;
		break;
	case AES_KEY_LEN_192:
		sState.u8Nk = Nk_192;
		sState.u8Nr = Nr_192;
		break;
	case AES_KEY_LEN_256:
		sState.u8Nk = Nk_256;
		sState.u8Nr = Nr_256;
		break;
	}
	// ラウンドキーの初期化
	vInitRoundKey(&sState, u8Key);
	// ベクトルの初期化
	memset(sState.u8Vector, 0x00, AES_BLOCK_LEN);
	// 初期化した値を返却
	return sState;
}

/**
 * AES処理ステータスを保持する変数の生成
 *
 * @param teAES_KeyLength 鍵長
 * @param uint8* 鍵
 * @param uint8* 初期ベクトル
 * @return tsAES_state ステータス
 */
PUBLIC tsAES_state vAES_newCBCState(teAES_KeyLength eKeyLen, uint8* u8Key, uint8* u8Iv) {
	// 暗号化ステータス
	tsAES_state sState;
	// CBCモード
	sState.mode = AES_CIPHER_MODE_CBC;
	// 鍵長
	sState.keyLen = eKeyLen;
	// NkとNrの初期化
	switch (eKeyLen) {
	case AES_KEY_LEN_128:
		sState.u8Nk = Nk_128;
		sState.u8Nr = Nr_128;
		break;
	case AES_KEY_LEN_192:
		sState.u8Nk = Nk_192;
		sState.u8Nr = Nr_192;
		break;
	case AES_KEY_LEN_256:
		sState.u8Nk = Nk_256;
		sState.u8Nr = Nr_256;
		break;
	}
	// ラウンドキーの初期化
	vInitRoundKey(&sState, u8Key);
	// ベクトルの初期化
	vAES_setVector(&sState, u8Iv);
	// 初期化した値を返却
	return sState;
}

/**
 * ベクトル設定処理
 *
 * @param tsAES_state ステータス
 * @param uint8* 初期ベクトル
 */
PUBLIC void vAES_setVector(tsAES_state* sState, uint8* u8Vector) {
	// ベクトルの初期化
	memcpy(sState->u8Vector, u8Vector, AES_BLOCK_LEN);
}

/**
 * 暗号化処理
 *
 * @param tsAES_state ステータス
 * @param uint8* 暗号化対象
 * @param uint32 暗号化対象バイト数
 */
PUBLIC void vAES_encrypt(tsAES_state* sState, uint8* u8Buff, uint32 u32Len) {
	// 最終ブロック直前まで暗号化
	uint32 u32Idx;
	if (sState->mode == AES_CIPHER_MODE_ECB) {
		// ECBモード
		for (u32Idx = 0; u32Idx < u32Len; u32Idx = u32Idx + AES_BLOCK_LEN) {
			vCiphertext(sState, &u8Buff[u32Idx]);
		}
	} else {
		// CBCモード
		for (u32Idx = 0; u32Idx < u32Len; u32Idx = u32Idx + AES_BLOCK_LEN) {
			vXorWithBlk(&u8Buff[u32Idx], sState->u8Vector);
			vCiphertext(sState, &u8Buff[u32Idx]);
			memcpy(sState->u8Vector, &u8Buff[u32Idx], AES_BLOCK_LEN);
		}
	}
}

/**
 * 暗号化処理(PKCS#7でパディング)
 *
 * @param tsAES_state ステータス
 * @param uint8* 暗号化対象
 * @param uint32 暗号化対象バイト数
 */
PUBLIC void vAES_encryptPad(tsAES_state* sState, uint8* u8Buff, uint32 u32Len) {
	// パディング処理(PKCS#7)
	uint8 u8PadSize = AES_BLOCK_LEN - (u32Len % AES_BLOCK_LEN);
	if (u8PadSize == 0) {
		memset(&u8Buff[u32Len], u8PadSize, AES_BLOCK_LEN);
	} else {
		memset(&u8Buff[u32Len], u8PadSize, u8PadSize);
	}
	// 暗号化
	vAES_encrypt(sState, u8Buff, u32Len + u8PadSize);
}

/**
 * 復号化処理
 *
 * @param tsAES_state ステータス
 * @param uint8* 暗号化対象
 * @param uint32 暗号化対象バイト数
 */
PUBLIC void vAES_decrypt(tsAES_state* sState, uint8* u8Buff, uint32 u32Len) {
	// 最終ブロック直前まで暗号化
	if (sState->mode == AES_CIPHER_MODE_ECB) {
		// ECBモード
		uint32 u32Idx;
		for (u32Idx = 0; u32Idx < u32Len; u32Idx += AES_BLOCK_LEN) {
			vInvCipher(sState, &u8Buff[u32Idx]);
		}
	} else {
		// CBCモード
		uint8 u8StoreNextIv[AES_BLOCK_LEN];
		uint32 u32Idx;
		for (u32Idx = 0; u32Idx < u32Len; u32Idx += AES_BLOCK_LEN) {
			memcpy(u8StoreNextIv, &u8Buff[u32Idx], AES_BLOCK_LEN);
			vInvCipher(sState, &u8Buff[u32Idx]);
			vXorWithBlk(&u8Buff[u32Idx], sState->u8Vector);
			memcpy(sState->u8Vector, u8StoreNextIv, AES_BLOCK_LEN);
		}
	}
	// 末尾のパディング部のクリアはしない
}

/*****************************************************************************/
/***        Local Functions                                                ***/
/*****************************************************************************/
/**
 * Initialize Round Key
 * @param uint8* ラウンドキー配列
 * @param uint8* 鍵
 */
static void vInitRoundKey(tsAES_state* sState, uint8* u8Key) {
	// ラウンドキーの初期化
	memset(sState->u8RoundKey, 0x00, 240);
	// ワード単位でキーをコピー
	uint8 u8Result = sState->u8Nk * 4;
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < u8Result; u8Idx = u8Idx + 4) {
		sState->u8RoundKey[u8Idx]     = u8Key[u8Idx];
		sState->u8RoundKey[u8Idx + 1] = u8Key[u8Idx + 1];
		sState->u8RoundKey[u8Idx + 2] = u8Key[u8Idx + 2];
		sState->u8RoundKey[u8Idx + 3] = u8Key[u8Idx + 3];
	}

	// 鍵の最終ワードから（ラウンド数＋１）ワード分まで増分１でループ
	// Nk:ワード数(4 or 6 or 8)
	// Nb:ブロック化定数(4)
	// Nr:10 or 12 or 14
	uint8 wkWord[4];
	uint8 u8FromIdx;
	uint8 u8ToIdx;
	uint8 u8Surplus;
	for (u8Idx = sState->u8Nk; u8Idx < (Nb * (sState->u8Nr + 1)); u8Idx++) {
		// 最終ワードから順番に１ワード分をコピー
		u8FromIdx = (u8Idx - 1) * 4;
		wkWord[0] = sState->u8RoundKey[u8FromIdx];
		wkWord[1] = sState->u8RoundKey[u8FromIdx + 1];
		wkWord[2] = sState->u8RoundKey[u8FromIdx + 2];
		wkWord[3] = sState->u8RoundKey[u8FromIdx + 3];

		// インデックスがワード数単位の先頭位置なのか判定
		u8Surplus = u8Idx % sState->u8Nk;
		if (u8Surplus == 0) {
			// コピーしたワードをローテーション
			// Function RotWord()
			vRotWord(wkWord);
			// コピーしたワードの各バイトを乱数配列の該当列に置換
			// Function Subword()
			vSubWord(wkWord);
			// 先頭バイトをラウンドコンスタント配列（インデックス÷ワード数）でXOR
			wkWord[0] = wkWord[0] ^ RCON[u8Idx / sState->u8Nk];
		} else if (sState->keyLen == AES_KEY_LEN_256 && u8Surplus == 4) {
			// AES256の場合でインデックス％ワード数が4
			// Function Subword()
			vSubWord(wkWord);
		}

		// 元キー領域から拡張領域へ一時領域の値をXORしながらコピー
		u8FromIdx = (u8Idx - sState->u8Nk) * 4;
		u8ToIdx = u8Idx * 4;
		sState->u8RoundKey[u8ToIdx]     = sState->u8RoundKey[u8FromIdx]     ^ wkWord[0];
		sState->u8RoundKey[u8ToIdx + 1] = sState->u8RoundKey[u8FromIdx + 1] ^ wkWord[1];
		sState->u8RoundKey[u8ToIdx + 2] = sState->u8RoundKey[u8FromIdx + 2] ^ wkWord[2];
		sState->u8RoundKey[u8ToIdx + 3] = sState->u8RoundKey[u8FromIdx + 3] ^ wkWord[3];
	}
}

/**
 * Block unit encryption processing.
 * ブロック単位の暗号化処理
 *
 * @param tsAES_state* ステータス
 * @param uint8* 暗号化対象
 */
static void vCiphertext(tsAES_state* sState, uint8* u8Buff) {
	// 暗号化対象のラウンド数に対応したブロックにラウンドキーの値をXOR
	uint8 u8Round = 0;
	vAddRoundKey(u8Buff, sState, u8Round);

	// 既定ラウンド数分、ラウンド毎の処理を実施
	for (u8Round = 1; u8Round < sState->u8Nr; u8Round++) {
		// 先頭から１ブロック分をサブワードに変換
		vSubBytes(u8Buff);
		// ブロック内の各バイトを特定のパターンでシャッフルする
		vShiftRows(u8Buff);
		// 排他的論理和を使って暗号化対象をバイト単位で混ぜ合わせる
		vMixColumns(u8Buff);
		// 暗号化対象のラウンド数に対応したブロックにラウンドキーの値をXOR
		vAddRoundKey(u8Buff, sState, u8Round);
	}

	// 先頭から１ブロック分をサブワードに変換
	vSubBytes(u8Buff);
	// ブロック内の各バイトを特定のパターンでシャッフルする
	vShiftRows(u8Buff);
	// 暗号化対象のラウンド数に対応したブロックにラウンドキーの値をXOR
	vAddRoundKey(u8Buff, sState, sState->u8Nr);
}

/**
 * ブロック単位の復号化処理
 *
 * @param tsAES_state* ステータス
 * @param uint8* 暗号化対象
 */
static void vInvCipher(tsAES_state* sState, uint8* u8Buff) {
	// 暗号化対象のラウンド数に対応したブロック（４×４バイト）にラウンドキーの値をXOR
	vAddRoundKey(u8Buff, sState, sState->u8Nr);
	// 暗号化の処理を逆から実行していく
	uint8 u8Round;
	for (u8Round = (sState->u8Nr - 1); u8Round > 0; u8Round--) {
		// ブロック内の各バイトを特定のパターンでシャッフルする（リバース版）
		vInvShiftRows(u8Buff);
		// 先頭から１ブロック分をサブワードからインデックスに変換
		vInvSubBytes(u8Buff);
		// 暗号化対象のラウンド数に対応したブロックにラウンドキーの値をXOR
		vAddRoundKey(u8Buff, sState, u8Round);
		// 排他的論理和を使って暗号化対象をバイト単位で混ぜ合わせる（リバース版）
		vInvMixColumns(u8Buff);
	}
	// ブロック内の各バイトを特定のパターンでシャッフルする（リバース版）
	vInvShiftRows(u8Buff);
	// 先頭から１ブロック分をサブワードからインデックスに変換
	vInvSubBytes(u8Buff);
	// 先頭から１ブロック分をサブワードからインデックスに変換
	vAddRoundKey(u8Buff, sState, u8Round);
}

/**
 * This function adds the round key to buffer.
 * 暗号化対象のラウンド数に対応したブロックにラウンドキーの値をXOR
 * @param uint8* 処理対象のバッファ
 * @param tsAES_state* ステータス
 * @param uint8 ラウンド数
 */
static void vAddRoundKey(uint8* u8Buff, tsAES_state* sState, uint8 u8Round) {
	uint8 u8WkVal1 = u8Round * Nb * 4;
	uint8 u8WkVal2;
	uint8 u8WordIdx;
	uint8 u8ByteIdx;
	for (u8WordIdx = 0; u8WordIdx < 4; u8WordIdx++) {
		u8WkVal2 = u8WordIdx * Nb;
		for (u8ByteIdx = 0; u8ByteIdx < 4; u8ByteIdx++) {
			u8Buff[u8WordIdx * 4 + u8ByteIdx] ^= sState->u8RoundKey[u8WkVal1 + u8WkVal2 + u8ByteIdx];
		}
	}
}

/**
 * Block unit XOR
 * ブロック単位のXOR処理
 *
 * @param uint8* 対象バッファ
 * @param uint8* 対象ブロック
 */
static void vXorWithBlk(uint8* u8Buff, uint8* u8Block) {
	// 1ブロック分XOR
	u8Buff[0]  = u8Buff[0]  ^ u8Block[0];
	u8Buff[1]  = u8Buff[1]  ^ u8Block[1];
	u8Buff[2]  = u8Buff[2]  ^ u8Block[2];
	u8Buff[3]  = u8Buff[3]  ^ u8Block[3];
	u8Buff[4]  = u8Buff[4]  ^ u8Block[4];
	u8Buff[5]  = u8Buff[5]  ^ u8Block[5];
	u8Buff[6]  = u8Buff[6]  ^ u8Block[6];
	u8Buff[7]  = u8Buff[7]  ^ u8Block[7];
	u8Buff[8]  = u8Buff[8]  ^ u8Block[8];
	u8Buff[9]  = u8Buff[9]  ^ u8Block[9];
	u8Buff[10] = u8Buff[10] ^ u8Block[10];
	u8Buff[11] = u8Buff[11] ^ u8Block[11];
	u8Buff[12] = u8Buff[12] ^ u8Block[12];
	u8Buff[13] = u8Buff[13] ^ u8Block[13];
	u8Buff[14] = u8Buff[14] ^ u8Block[14];
	u8Buff[15] = u8Buff[15] ^ u8Block[15];
}

/**
 *  Word rotation processing.
 *  ワードのローテーション処理
 *  @param uint8* ローテーション処理
 */
static void vRotWord(uint8* word) {
	uint8 u8Val = word[0];
	word[0] = word[1];
	word[1] = word[2];
	word[2] = word[3];
	word[3] = u8Val;
}

/**
 * The SubBytes Function Substitutes the values in the
 * state matrix with values in an S-box.
 * 先頭から１ブロック分を非線形テーブルの値に変換
 * @param uint8* ローテーション処理
 */
static void vSubBytes(uint8* u8Buff) {
	u8Buff[0]  = S_BOX[u8Buff[0]];
	u8Buff[1]  = S_BOX[u8Buff[1]];
	u8Buff[2]  = S_BOX[u8Buff[2]];
	u8Buff[3]  = S_BOX[u8Buff[3]];
	u8Buff[4]  = S_BOX[u8Buff[4]];
	u8Buff[5]  = S_BOX[u8Buff[5]];
	u8Buff[6]  = S_BOX[u8Buff[6]];
	u8Buff[7]  = S_BOX[u8Buff[7]];
	u8Buff[8]  = S_BOX[u8Buff[8]];
	u8Buff[9]  = S_BOX[u8Buff[9]];
	u8Buff[10] = S_BOX[u8Buff[10]];
	u8Buff[11] = S_BOX[u8Buff[11]];
	u8Buff[12] = S_BOX[u8Buff[12]];
	u8Buff[13] = S_BOX[u8Buff[13]];
	u8Buff[14] = S_BOX[u8Buff[14]];
	u8Buff[15] = S_BOX[u8Buff[15]];
}

/**
 * Convert S-Box Word
 * @param uint8* 変換対象ワード
 */
static void vSubWord(uint8* u8Word) {
	u8Word[0] = S_BOX[u8Word[0]];
	u8Word[1] = S_BOX[u8Word[1]];
	u8Word[2] = S_BOX[u8Word[2]];
	u8Word[3] = S_BOX[u8Word[3]];
}

/**
 * Column shift processing for blocks.
 * @param uint8* 変換対象ワード
 */
static void vShiftRows(uint8* u8Buff) {
	// Rotate first row 1 columns to left
	uint8 u8Wk = u8Buff[1];
	u8Buff[1]  = u8Buff[5];
	u8Buff[5]  = u8Buff[9];
	u8Buff[9]  = u8Buff[13];
	u8Buff[13] = u8Wk;

	// Rotate second row 2 columns to left
	u8Wk       = u8Buff[2];
	u8Buff[2]  = u8Buff[10];
	u8Buff[10] = u8Wk;

	u8Wk       = u8Buff[6];
	u8Buff[6]  = u8Buff[14];
	u8Buff[14] = u8Wk;

	// Rotate third row 3 columns to left
	u8Wk       = u8Buff[3];
	u8Buff[3]  = u8Buff[15];
	u8Buff[15] = u8Buff[11];
	u8Buff[11] = u8Buff[7];
	u8Buff[7]  = u8Wk;
}

/**
 * Transformation in the Cipher that takes all of the columns of the State and mixes their data
 * (independently of one another) to produce new columns.
 * バイト単位にミックスする処理
 * @param uint8* 対象データ配列
 */
static void vMixColumns(uint8* u8Buff) {
	uint8 u8WordIdx;
	uint8 u8ByteIdx;
	uint8 u8Wk1;
	uint8 u8Wk2;
	uint8 u8Wk3;
	for (u8WordIdx = 0; u8WordIdx < 4; u8WordIdx++) {
		u8ByteIdx = u8WordIdx * 4;
		u8Wk1 = u8Buff[u8ByteIdx];
		u8Wk2 = u8Buff[u8ByteIdx] ^ u8Buff[u8ByteIdx + 1] ^ u8Buff[u8ByteIdx + 2] ^ u8Buff[u8ByteIdx + 3];
		u8Wk3 = u8Buff[u8ByteIdx] ^ u8Buff[u8ByteIdx + 1];
		u8Wk3 = vXtime(u8Wk3);
		u8Buff[u8ByteIdx] ^= u8Wk2 ^ u8Wk3;

		u8Wk3 = u8Buff[u8ByteIdx + 1] ^ u8Buff[u8ByteIdx + 2];
		u8Wk3 = vXtime(u8Wk3);
		u8Buff[u8ByteIdx + 1] ^= u8Wk2 ^ u8Wk3;

		u8Wk3 = u8Buff[u8ByteIdx + 2] ^ u8Buff[u8ByteIdx + 3];
		u8Wk3 = vXtime(u8Wk3);
		u8Buff[u8ByteIdx + 2] ^= u8Wk2 ^ u8Wk3;

		u8Wk3 = u8Buff[u8ByteIdx + 3] ^ u8Wk1;
		u8Wk3 = vXtime(u8Wk3);
		u8Buff[u8ByteIdx + 3] ^= u8Wk3 ^ u8Wk2;
	}
}

/**
 * 左に1bit循環シフトした後、シフト前の最上位桁が1の場合には0x1bとXOR
 */
static uint8 vXtime(uint8 u8Val) {
	return ((u8Val << 1) ^ ((u8Val >> 7) * 0x1b));
}

/**
 * 変換：S-BOXの値→インデックス
 * @param uint8* 対象データ配列
 */
static void vInvSubBytes(uint8* u8Buff) {
	u8Buff[0]  = INV_S_BOX[u8Buff[0]];
	u8Buff[1]  = INV_S_BOX[u8Buff[1]];
	u8Buff[2]  = INV_S_BOX[u8Buff[2]];
	u8Buff[3]  = INV_S_BOX[u8Buff[3]];
	u8Buff[4]  = INV_S_BOX[u8Buff[4]];
	u8Buff[5]  = INV_S_BOX[u8Buff[5]];
	u8Buff[6]  = INV_S_BOX[u8Buff[6]];
	u8Buff[7]  = INV_S_BOX[u8Buff[7]];
	u8Buff[8]  = INV_S_BOX[u8Buff[8]];
	u8Buff[9]  = INV_S_BOX[u8Buff[9]];
	u8Buff[10] = INV_S_BOX[u8Buff[10]];
	u8Buff[11] = INV_S_BOX[u8Buff[11]];
	u8Buff[12] = INV_S_BOX[u8Buff[12]];
	u8Buff[13] = INV_S_BOX[u8Buff[13]];
	u8Buff[14] = INV_S_BOX[u8Buff[14]];
	u8Buff[15] = INV_S_BOX[u8Buff[15]];
}

/**
 * 変換：S-BOXの値→インデックス
 * @param uint8* 対象データ配列
 */
static void vInvShiftRows(uint8* u8Buff) {
	// Rotate first row 1 columns to right
	uint8 u8Wk = u8Buff[13];
	u8Buff[13] = u8Buff[9];
	u8Buff[9]  = u8Buff[5];
	u8Buff[5]  = u8Buff[1];
	u8Buff[1]  = u8Wk;

	// Rotate second row 2 columns to right
	u8Wk       = u8Buff[2];
	u8Buff[2]  = u8Buff[10];
	u8Buff[10] = u8Wk;

	u8Wk       = u8Buff[6];
	u8Buff[6]  = u8Buff[14];
	u8Buff[14] = u8Wk;

	// Rotate third row 3 columns to right
	u8Wk       = u8Buff[3];
	u8Buff[3]  = u8Buff[7];
	u8Buff[7]  = u8Buff[11];
	u8Buff[11] = u8Buff[15];
	u8Buff[15] = u8Wk;
}

/**
 * バイト単位にミックスする処理
 * @param uint8* 対象データ配列
 */
static void vInvMixColumns(uint8* u8Buff) {
	uint8 u8Wk1;
	uint8 u8Wk2;
	uint8 u8Wk3;
	uint8 u8Wk4;
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < AES_BLOCK_LEN; u8Idx = u8Idx + 4) {
		u8Wk1 = u8Buff[u8Idx];
		u8Wk2 = u8Buff[u8Idx + 1];
		u8Wk3 = u8Buff[u8Idx + 2];
		u8Wk4 = u8Buff[u8Idx + 3];
		u8Buff[u8Idx]     =
			vMultiply(u8Wk1, 0x0e) ^ vMultiply(u8Wk2, 0x0b) ^ vMultiply(u8Wk3, 0x0d) ^ vMultiply(u8Wk4, 0x09);
		u8Buff[u8Idx + 1] =
			vMultiply(u8Wk1, 0x09) ^ vMultiply(u8Wk2, 0x0e) ^ vMultiply(u8Wk3, 0x0b) ^ vMultiply(u8Wk4, 0x0d);
		u8Buff[u8Idx + 2] =
			vMultiply(u8Wk1, 0x0d) ^ vMultiply(u8Wk2, 0x09) ^ vMultiply(u8Wk3, 0x0e) ^ vMultiply(u8Wk4, 0x0b);
		u8Buff[u8Idx + 3] =
			vMultiply(u8Wk1, 0x0b) ^ vMultiply(u8Wk2, 0x0d) ^ vMultiply(u8Wk3, 0x09) ^ vMultiply(u8Wk4, 0x0e);
	}
}

/**
 * ミックス処理から呼ばれる処理
 * @param uint8* 対象データ配列
 */
static uint8 vMultiply(uint8 u8x, uint8 u8y) {
	return (((u8y & 1) * u8x) ^
		 ((u8y>>1 & 1) * vXtime(u8x)) ^
		 ((u8y>>2 & 1) * vXtime(vXtime(u8x))) ^
		 ((u8y>>3 & 1) * vXtime(vXtime(vXtime(u8x)))) ^
		 ((u8y>>4 & 1) * vXtime(vXtime(vXtime(vXtime(u8x))))));
}
