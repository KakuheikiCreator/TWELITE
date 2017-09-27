/****************************************************************************
 *
 * MODULE :Account authentication functions header file
 *
 * CREATED:2017/03/27 22:58:00
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
#ifndef APP_AUTH_H_INCLUDED
#define APP_AUTH_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "ToCoNet.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// トークンサイズ
#define APP_AUTH_TOKEN_SIZE     (32)

/** ストレッチング回数ベース */
#ifndef APP_HASH_STRETCHING_CNT_BASE
	// ベースとなる回数は1920とする
	#define APP_HASH_STRETCHING_CNT_BASE    (1920)
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// Process State
typedef enum {
	E_AUTH_HASH_PROC_BEGIN = 0,
	E_AUTH_HASH_PROC_RUNNING,
	E_AUTH_HASH_PROC_COMPLETE
} teAuthHashGenProcSts;

// 構造体：ハッシュ値生成情報
typedef struct {
	teAuthHashGenProcSts eStatus;				// ステータス（0:未処理、1:処理中、2:処理完了）
	uint8 u8SrcCode[APP_AUTH_TOKEN_SIZE];		// 元コード
	uint8 u8SyncToken[APP_AUTH_TOKEN_SIZE];		// 同期トークン
	uint32 u32ShufflePtn;						// シャッフルパターン
	uint16 u16StCntEnd;							// ストレッチング回数
	uint16 u16StCntNow;							// ストレッチング実施回数
	uint8 u8HashCode[APP_AUTH_TOKEN_SIZE];		// ハッシュ値
} tsAuthHashGenState;

// 構造体：デバイス情報
typedef struct {
	uint32 u32DeviceID;							// デバイスID（8桁）
	char cDeviceName[11];						// デバイス名
	uint8 u8DeviceType;							// デバイスタイプ
	uint8 u8MstPWHash[APP_AUTH_TOKEN_SIZE];		// マスターパスワードハッシュ
	uint16 u16MstPWStretching;					// マスターパスワードのハッシュストレッチング回数
	uint8 u8AESPassword[13];					// 通信暗号化パスワード
	uint8 u8StatusMap;							// ステータスマップ
} tsAuthDeviceInfo;

// 構造体：リモートデバイス情報
typedef struct {
	uint32 u32DeviceID;							// デバイスID（8桁）
	char cDeviceName[11];						// デバイス名
	uint32 u32StartDateTime;					// 認証情報の利用開始日時（紀元からの分数）
	uint8 u8SyncToken[APP_AUTH_TOKEN_SIZE];		// 同期トークン
	uint8 u8AuthHash[APP_AUTH_TOKEN_SIZE];		// 認証ハッシュ（送信側）
	uint8 u8SndStretching;						// ストレッチングカウント（送信側）
	uint8 u8AuthCode[APP_AUTH_TOKEN_SIZE];		// 認証コード（受信側）
	uint8 u8RcvStretching;						// ストレッチングカウント（受信側）
	uint8 u8StatusMap;							// ステータスマップ
	uint8 u8Filler[9];							// 余白
} tsAuthRemoteDevInfo;


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/** 返信ストレッチング回数 */
PUBLIC uint16 u16Auth_convToRespStCnt(uint32 u32ElapsedMin);
/** ハッシュ生成情報の生成処理 */
PUBLIC tsAuthHashGenState sAuth_generateHashInfo(uint8* pu8Code, uint16 u16StCnt);
/** 同期トークン設定処理 */
PUBLIC void vAuth_setSyncToken(tsAuthHashGenState* psHashGenInfo, uint8* pu8SyncToken);
/** 拡張ハッシュストレッチング処理 */
PUBLIC bool_t bAuth_hashStretching(tsAuthHashGenState* psHashGenInfo);
/** デバイス情報へのマスターパスワード情報設定 */
PUBLIC void vAuth_setMstPwInfo(tsAuthHashGenState* psHashInfo, tsAuthDeviceInfo* psDevInfo);
/** リモートデバイス情報へのキーペア設定 */
PUBLIC void vAuth_setKeyPair(tsAuthHashGenState* psHashInfo, tsAuthRemoteDevInfo* psFromInfo, tsAuthRemoteDevInfo* psToInfo);
/** ワンタイムトークンの生成編集処理 */
PUBLIC void vAuth_editOneTimeTkn(uint8* pu8OTTkn, uint16 u16RndVal, uint32 u32RefMin, uint8* pu8SyncTkn);
/** マスターパスワードハッシュの有効判定 */
PUBLIC bool_t bAuth_isEnableMstPwHash(tsAuthHashGenState* psHashInfo, tsAuthDeviceInfo* psDevInfo);
/** パスワードハッシュの有効判定 */
PUBLIC bool_t bAuth_isEnablePwHash(uint8* pu8HashCode, tsAuthHashGenState* psHashInfo);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* APP_AUTH_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
