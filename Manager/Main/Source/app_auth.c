/****************************************************************************
 *
 * MODULE :Account authentication functions source file
 *
 * CREATED:2017/04/02 02:38:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:アカウント認証処理に関する関数群
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 ****************************************************************************
 * Copyright (c) 2017, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <jendefs.h>
#include <AppHardwareApi.h>

/****************************************************************************/
/***        ToCoNet Definitions                                           ***/
/****************************************************************************/
#include "serial.h"
//#include "fprintf.h"
#include "sprintf.h"
//#include "utils.h"


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
#include "app_auth.h"
#include "sha256.h"
#include "value_util.h"
#include "config.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

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
 * NAME: u16Auth_convToStCnt
 *
 * DESCRIPTION:返信ストレッチング回数の生成処理
 *
 * PARAMETERS:        Name           RW  Usage
 *      uint32        u32ElapsedMin  R   経過分数
 *
 * RETURNS:
 *  uint16:ストレッチング回数
 *
 ****************************************************************************/
PUBLIC uint16 u16Auth_convToRespStCnt(uint32 u32ElapsedMin) {
	return APP_HASH_STRETCHING_CNT_BASE - (u32ElapsedMin % APP_HASH_STRETCHING_CNT_BASE);
}

/****************************************************************************
 *
 * NAME: sAuth_generateHashInfo
 *
 * DESCRIPTION:ハッシュ生成情報の生成処理
 *
 * PARAMETERS:          Name         RW  Usage
 *      uint8*          pu8Code      R   元コード
 *      uint16          u16StCnt     R   ストレッチングカウント
 *
 * RETURNS:
 *   tsAuthHashGenInfo:ハッシュ生成情報
 *
 ****************************************************************************/
PUBLIC tsAuthHashGenState sAuth_generateHashInfo(uint8* pu8Code, uint16 u16StCnt) {
	tsAuthHashGenState sHashGenInfo;
	sHashGenInfo.eStatus = E_AUTH_HASH_PROC_BEGIN;					// ステータス（0:未処理、1:処理中、2:処理完了）
	memcpy(sHashGenInfo.u8SrcCode, pu8Code, APP_AUTH_TOKEN_SIZE);	// 元コード
	memset(sHashGenInfo.u8SyncToken, 0x00, APP_AUTH_TOKEN_SIZE);	// 同期トークン
	sHashGenInfo.u32ShufflePtn = 0;									// シャッフルパターン
	sHashGenInfo.u16StCntEnd = u16StCnt;							// ストレッチング回数
	sHashGenInfo.u16StCntNow = 0;									// ストレッチング実施回数
	memcpy(sHashGenInfo.u8HashCode, pu8Code, APP_AUTH_TOKEN_SIZE);	// ハッシュ値
	return sHashGenInfo;
}

/****************************************************************************
 *
 * NAME: vAuth_setSyncToken
 *
 * DESCRIPTION:同期トークン設定処理
 *
 * PARAMETERS:            Name           RW  Usage
 *   tsAuthHashGenInfo*   psHashGenInfo  W   ハッシュ生成情報
 *   uint8*               pu8SyncToken   R   同期トークン
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vAuth_setSyncToken(tsAuthHashGenState* psHashGenInfo, uint8* pu8SyncToken) {
	// 同期トークンの編集
	memcpy(psHashGenInfo->u8SyncToken, pu8SyncToken, APP_AUTH_TOKEN_SIZE);
	// シャッフルパターンの生成
	TypeConverter converter;
	converter.u32Values[0] = 0;
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < 32; u8Idx++) {
		converter.u8Values[u8Idx % 4] = converter.u8Values[u8Idx % 4] ^ pu8SyncToken[u8Idx];
	}
	psHashGenInfo->u32ShufflePtn = converter.u32Values[0];
}

/****************************************************************************
 *
 * NAME: bAuth_hashStretching
 *
 * DESCRIPTION:拡張ハッシュストレッチング処理
 *
 * PARAMETERS:            Name           RW  Usage
 *   tsAuthHashGenInfo*   psHashGenInfo  RW  ハッシュ生成情報
 *
 * RETURNS:
 *   bool_t TRUE:処理完了 FALSE:処理中
 *
 ****************************************************************************/
PUBLIC bool_t bAuth_hashStretching(tsAuthHashGenState* psHashGenInfo) {
	// ストレッチング処理の実行判定
	if (psHashGenInfo->u16StCntNow == psHashGenInfo->u16StCntEnd) {
		psHashGenInfo->eStatus = E_AUTH_HASH_PROC_COMPLETE;
		return TRUE;
	}
	// 処理ステータス更新
	psHashGenInfo->eStatus = E_AUTH_HASH_PROC_RUNNING;
	// ハッシュ関数実行
	SHA256_state sha256State = sSHA256_newState();
	vSHA256_append(&sha256State, psHashGenInfo->u8HashCode, APP_AUTH_TOKEN_SIZE);
	vSHA256_generateHash(&sha256State, psHashGenInfo->u8HashCode);
	// シャッフル実行
	uint32 u32ShufflePtn = psHashGenInfo->u32ShufflePtn;
	uint8 u8Idx = 0;
	uint8 u8WkCode;
	while (u32ShufflePtn > 0) {
		if ((u32ShufflePtn & 0x01) == 0x01) {
			u8WkCode = psHashGenInfo->u8HashCode[u8Idx];
			psHashGenInfo->u8HashCode[u8Idx] = psHashGenInfo->u8HashCode[(u8Idx + 1) % APP_AUTH_TOKEN_SIZE];
			psHashGenInfo->u8HashCode[(u8Idx + 1) % APP_AUTH_TOKEN_SIZE] = u8WkCode;
		}
		u8Idx++;
		u32ShufflePtn = u32ShufflePtn >> 1;
	}
	// ストレッチング実施回数カウントアップ
	psHashGenInfo->u16StCntNow++;
	// 次回ストレッチング実施判定
	if (psHashGenInfo->u16StCntNow == psHashGenInfo->u16StCntEnd) {
		psHashGenInfo->eStatus = E_AUTH_HASH_PROC_COMPLETE;
		return TRUE;
	}
	return FALSE;
}

/****************************************************************************
 *
 * NAME: vAuth_setMstPwInfo
 *
 * DESCRIPTION:デバイス情報へのマスターパスワード情報設定
 *
 * PARAMETERS:           Name           RW  Usage
 *   tsAuthHashGenInfo*  psHashInfo     R   ハッシュ生成情報
 *   tsAuthDeviceInfo*   psDevInfo      W   デバイス情報
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vAuth_setMstPwInfo(tsAuthHashGenState* psHashInfo, tsAuthDeviceInfo* psDevInfo) {
	// ハッシュ値の設定
	memcpy(psDevInfo->u8MstPWHash, psHashInfo->u8HashCode, APP_AUTH_TOKEN_SIZE);
	// ストレッチング回数の設定
	psDevInfo->u16MstPWStretching = psHashInfo->u16StCntNow;
}

/****************************************************************************
 *
 * NAME: vAuth_setKeyPair
 *
 * DESCRIPTION:リモートデバイス情報へのキーペア設定
 *
 * PARAMETERS:           Name           RW  Usage
 *   tsAuthHashGenInfo*  psHashInfo     R   ハッシュ生成情報
 *   tsAuthDeviceInfo*   psDevInfo      W   デバイス情報
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vAuth_setKeyPair(tsAuthHashGenState* psHashInfo, tsAuthRemoteDevInfo* psFromInfo, tsAuthRemoteDevInfo* psToInfo) {
	// 送信元デバイス情報設定
	memcpy(psFromInfo->u8AuthHash, psHashInfo->u8HashCode, APP_AUTH_TOKEN_SIZE);
	memcpy(psFromInfo->u8SyncToken, psHashInfo->u8SyncToken, APP_AUTH_TOKEN_SIZE);
	// 送信先デバイス情報設定
	memcpy(psToInfo->u8AuthCode, psHashInfo->u8SrcCode, APP_AUTH_TOKEN_SIZE);
	memcpy(psToInfo->u8SyncToken, psHashInfo->u8SyncToken, APP_AUTH_TOKEN_SIZE);
}

/****************************************************************************
 *
 * NAME: vAuth_editOneTimeTkn
 *
 * DESCRIPTION:ワンタイムトークンの生成編集処理
 *
 * PARAMETERS:        Name            RW  Usage
 *   uint8*           pu8OTTkn        W   ワンタイムトークン
 *   uint16           u16RndVal       R   乱数
 *   uint32           u32RefMin       R   基準時間
 *   uint8*           pu8SyncTkn      R   同期トークン
 *
 * RETURNS:
 *   uint16 ワンタイムトークン生成時に使用した乱数
 *
 ****************************************************************************/
PUBLIC void vAuth_editOneTimeTkn(uint8* pu8OTTkn, uint16 u16RndVal, uint32 u32RefMin, uint8* pu8SyncTkn) {
	// 経過時間と乱数を元にシャッフルパターンを生成
	uint16 u16RandVal = u16ValUtil_getRandVal();
	uint32 u32ShufflePtn = u32RefMin ^ (uint32)(((uint32)u16RandVal << 16) + u16RandVal);
	// 同期トークンのシャッフル
	uint8 u8WkCode[APP_AUTH_TOKEN_SIZE];
	memcpy(u8WkCode, pu8SyncTkn, APP_AUTH_TOKEN_SIZE);
	uint8 u8Idx = 0;
	uint8 u8WkCh;
	while (u32ShufflePtn > 0) {
		if ((u32ShufflePtn & 0x01) == 0x01) {
			u8WkCh = u8WkCode[u8Idx];
			u8WkCode[u8Idx] = u8WkCode[(u8Idx + 1) % APP_AUTH_TOKEN_SIZE];
			u8WkCode[(u8Idx + 1) % APP_AUTH_TOKEN_SIZE] = u8WkCh;
		}
		u8Idx++;
		u32ShufflePtn = u32ShufflePtn >> 1;
	}
	// ハッシュコード（ワンタイムトークン）算出
	SHA256_state sha256State = sSHA256_newState();
	vSHA256_append(&sha256State, u8WkCode, APP_AUTH_TOKEN_SIZE);
	vSHA256_generateHash(&sha256State, pu8OTTkn);
}

/****************************************************************************
 *
 * NAME: bAuth_isEnableMstPwHash
 *
 * DESCRIPTION:マスターパスワードハッシュの有効判定
 *
 * PARAMETERS:            Name          RW  Usage
 *   tsAuthHashGenInfo*   psHashInfo    R   ハッシュ生成情報
 *   tsAuthDeviceInfo*    psDevInfo     W   デバイス情報
 *
 * RETURNS:
 *   bool_t TRUE:パスワード有効
 *
 ****************************************************************************/
PUBLIC bool_t bAuth_isEnableMstPwHash(tsAuthHashGenState* psHashInfo, tsAuthDeviceInfo* psDevInfo) {
	// ハッシュコードの判定
	return (memcmp(psHashInfo->u8HashCode, psDevInfo->u8MstPWHash, APP_AUTH_TOKEN_SIZE) == 0);
}

/****************************************************************************
 *
 * NAME: bAuth_isEnablePwHash
 *
 * DESCRIPTION:パスワードハッシュの有効判定
 *
 * PARAMETERS:           Name           RW  Usage
 *   uint8*              pu8HashCode    R   ハッシュコード
 *   tsAuthHashGenInfo*  psHashInfo     R   ハッシュ生成情報
 *
 * RETURNS:
 *   bool_t TRUE:パスワード有効
 *
 ****************************************************************************/
PUBLIC bool_t bAuth_isEnablePwHash(uint8* pu8HashCode, tsAuthHashGenState* psHashInfo) {
	// ハッシュコードの判定
	return (memcmp(pu8HashCode, psHashInfo->u8HashCode, APP_AUTH_TOKEN_SIZE) == 0);
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
