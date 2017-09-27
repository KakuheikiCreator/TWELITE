/****************************************************************************
 *
 * MODULE :SHA256 functions source file
 *
 * CREATED:2015/04/25 08:32:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:SHA256のハッシュコードを生成する関数群
 *             SHA256 functions
 *             アルゴリズムは下記を参照
 *             FIP180-2(http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf)
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
#include <string.h>
#include "jendefs.h"
#include <AppHardwareApi.h>

#include "sha256.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// メッセージブロックサイズ
#define SHA256_MSGBLK_SIZE           64

// 右ビットシフト：SHR n(x) = x >> n
#define SHR(n, x) ((x)>>(n))
// 右ビット回転：ROTR n(x) = (x >> n) or (x << w - n)
#define ROTR(n, x) (SHR(n, x)|((x)<<(32-n)))

// Ch( x, y, z) = (x and y) xor (~x and z)
#define Ch(x, y, z) ((x&y)^(~x&z))
// Maj( x, y, z) = (x and y) xor (x and z) xor (y and z)
#define Maj(x, y, z) ((x&y)^(x&z)^(y&z))

// Σ０関数：S_0~256(x) = ROTR 2(x) xor ROTR 13(x) xor ROTR 22(x)
#define S256_0(x) (ROTR(2,x) ^ ROTR(13,x) ^ ROTR(22,x))
// Σ１関数：S_1~256(x) = ROTR 6(x) xor ROTR 11(x) xor ROTR 25(x)
#define S256_1(x) (ROTR(6,x) ^ ROTR(11,x) ^ ROTR(25,x))

// σ０関数：s_0~256(x) = ROTR 7(x) xor ROTR 18(x) xor SHR3(x)
#define S_0_256(x) (ROTR(7,(x)) ^ ROTR(18,(x)) ^ SHR(3,(x)))
// σ１関数：s_1~256(x) = ROTR 17(x) xor ROTR 19(x) xor SHR10(x)
#define S_1_256(x) (ROTR(17,(x)) ^ ROTR(19,(x)) ^ SHR(10,(x)))

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// 変数参照時の型の違い対策
typedef union {
	uint8  str[8];			// 文字列：1Byte × 8
	uint32 words32[2];		// 32Bitワード：4Byte × 2
	uint64 words64;			// 64Bitワード：8Byte × 1
} SHA256_typeConv;
// ブロック参照時の型の違い対策
typedef union {
	uint8  u8MsgBlock[256];	// 文字列：1Byte × 256
	uint32 msgWords[64];	// ワード：4Byte × 64
} SHA256_blockConv;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
/**
 *	ハッシュ値の算出処理
 *
 *	@param pState ハッシュコード生成処理ステータス情報
 */
PRIVATE void vSHA256_updateHashValue(SHA256_state *psState);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
// constant 32-bit words
// ハッシュ値のエントロピーを上げ、ランダム性を持たせる為の値？
// 先頭から64個分の素数の立方根の小数部分？
PRIVATE const uint32 SHA256_K[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/**
 * ハッシュコード演算の中間状態値初期化処理
 */
PUBLIC SHA256_state sSHA256_newState() {
	SHA256_state state;
	// ハッシュ値の初期化
	state.u32Hash[0] = 0x6a09e667;
	state.u32Hash[1] = 0xbb67ae85;
	state.u32Hash[2] = 0x3c6ef372;
	state.u32Hash[3] = 0xa54ff53a;
	state.u32Hash[4] = 0x510e527f;
	state.u32Hash[5] = 0x9b05688c;
	state.u32Hash[6] = 0x1f83d9ab;
	state.u32Hash[7] = 0x5be0cd19;
	// メッセージブロックの初期化
	memset(state.u8MsgBlock, 0, SHA256_MSGBLK_SIZE);
	// メッセージブロックの編集済みバイト数
	state.u32MsgBlockIdx = 0;
	// メッセージ長（累積値）
	state.u32MsgLen = 0;
	// 初期化された構造体の返却
	return state;
}

/**
 *	ハッシュコードに変換するメッセージを追加する
 *	@param psState ハッシュコード生成処理ステータス情報
 *	@param pu8Msg  追加メッセージの先頭のポインタ
 *	@param u32Len  追加メッセージの長さ
 */
PUBLIC void vSHA256_append(SHA256_state *psState, const uint8 *pu8Msg, uint32 u32Len) {
	uint32 editSize;
	uint32 msgIdx = 0;
	while (msgIdx < u32Len) {
		// 編集サイズを算出
		editSize = u32Len - msgIdx;
		if (editSize > (SHA256_MSGBLK_SIZE - psState->u32MsgBlockIdx)) {
			editSize = SHA256_MSGBLK_SIZE - psState->u32MsgBlockIdx;
		}
		while (editSize > 0) {
			psState->u8MsgBlock[psState->u32MsgBlockIdx++] = pu8Msg[msgIdx++];
			// 編集済みメッセージ長の更新
			psState->u32MsgLen++;
			editSize--;
		}
		// ハッシュ値更新判定
		if (psState->u32MsgBlockIdx >= SHA256_MSGBLK_SIZE) {
			// 1ブロック分のハッシュ更新処理
			vSHA256_updateHashValue(psState);
		}
	}
}

/**
 * Appendされたメッセージからハッシュコードの生成処理
 *
 *	@param pState ハッシュコード生成処理ステータス情報
 *	@param pHash 結果を代入する配列(256bit必要です)へのポインタ
 */
PUBLIC void vSHA256_generateHash(SHA256_state *psState, uint8 *pu8Hash) {
	// 終端文字列の生成
	int length = SHA256_MSGBLK_SIZE - psState->u32MsgBlockIdx;
	if (length <= 8) length += SHA256_MSGBLK_SIZE;
	// メッセージブロックの編集
	SHA256_blockConv conv;
	memset(conv.u8MsgBlock, 0, length);
	// 終端ビット編集
	conv.u8MsgBlock[0] = 0x80;
	// 文字列ビット長編集
	SHA256_typeConv typeConv;
	typeConv.words64 = ((uint32)psState->u32MsgLen) * 8;
	conv.u8MsgBlock[length - 8] = typeConv.str[0];
	conv.u8MsgBlock[length - 7] = typeConv.str[1];
	conv.u8MsgBlock[length - 6] = typeConv.str[2];
	conv.u8MsgBlock[length - 5] = typeConv.str[3];
	conv.u8MsgBlock[length - 4] = typeConv.str[4];
	conv.u8MsgBlock[length - 5] = typeConv.str[5];
	conv.u8MsgBlock[length - 2] = typeConv.str[6];
	conv.u8MsgBlock[length - 1] = typeConv.str[7];
	// 終端文字列の書き込み
	vSHA256_append(psState, conv.u8MsgBlock, length);
	// 演算結果のコピー
	memcpy(pu8Hash, psState->u32Hash, 32);
	// ハッシュ値の表示
	memset(conv.u8MsgBlock, 0, length);
	memcpy(conv.u8MsgBlock, pu8Hash, 32);
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/**
 * ハッシュ値の更新処理
 */
PRIVATE void vSHA256_updateHashValue(SHA256_state *psState){
	// メッセージブロックの変換
	SHA256_blockConv conv;
	memcpy(conv.u8MsgBlock, psState->u8MsgBlock, SHA256_MSGBLK_SIZE);
	// 拡張分のブロック生成
	int i;
	for (i=16; i < 64; i++) {
		conv.msgWords[i] = S_1_256(conv.msgWords[i-2])  + conv.msgWords[i-7]
		                 + S_0_256(conv.msgWords[i-15]) + conv.msgWords[i-16];
	}
	// ハッシュ値配列初期化
	uint32 wkHash[8];
	memcpy(wkHash, psState->u32Hash, 32);
	int idxList[] = {1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7};
	int idx, idxA, idxB, idxC, idxD, idxE, idxF, idxG, idxH;
	uint32 t1, t2;
	int round;
	for (round = 0; round < 64; round++) {
		idx = 7 - (round % 8);
		idxA = idxList[idx++];
		idxB = idxList[idx++];
		idxC = idxList[idx++];
		idxD = idxList[idx++];
		idxE = idxList[idx++];
		idxF = idxList[idx++];
		idxG = idxList[idx++];
		idxH = idxList[idx];
		t1 = wkHash[idxH]
		   + S256_1(wkHash[idxE]) + Ch(wkHash[idxE], wkHash[idxF], wkHash[idxG])
		   + SHA256_K[round] + conv.msgWords[round];
		t2 = S256_0(wkHash[idxA]) + Maj(wkHash[idxA], wkHash[idxB], wkHash[idxC]);
		// 要素の単純シフトでは無い値のみ更新
		wkHash[idxD] = wkHash[idxD] + t1;
		wkHash[idxH] = t1 + t2;
	}
	// ハッシュ値の更新
	psState->u32Hash[0] += wkHash[0];
	psState->u32Hash[1] += wkHash[1];
	psState->u32Hash[2] += wkHash[2];
	psState->u32Hash[3] += wkHash[3];
	psState->u32Hash[4] += wkHash[4];
	psState->u32Hash[5] += wkHash[5];
	psState->u32Hash[6] += wkHash[6];
	psState->u32Hash[7] += wkHash[7];
	// ブロックの初期化
	memset(psState->u8MsgBlock, 0, SHA256_MSGBLK_SIZE);
	// ブロック内インデックスの初期化
	psState->u32MsgBlockIdx = 0;
}
