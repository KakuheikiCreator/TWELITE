/****************************************************************************
 *
 * MODULE :Value Utility functions source file
 *
 * CREATED:2016/02/22 00:08:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:値の変換等の値に関する基本的なユーティリティ関数群
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
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <string.h>
#include <jendefs.h>
#include <AppHardwareApi.h>
#include "framework.h"
#include "value_util.h"

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
/*****************************************************************************
 *
 * NAME:u32ValUtil_binaryToBcd
 *
 * DESCRIPTION:変換関数（数値→BCD形式）
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      u32Val          R   変換対象
 * RETURNS:
 *      uint64:変換結果
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint32 u64ValUtil_binaryToBcd(uint32 u32Val) {
	uint32 result = 0;	// 変換値
	uint8 shiftCnt;
	for (shiftCnt = 0; shiftCnt <= 32; shiftCnt += 4) {
		result = result | (u32Val % 10) << shiftCnt;
		u32Val /= 10;
	}
	return result;
}

/*****************************************************************************
 *
 * NAME:u32ValUtil_bcdToBinary
 *
 * DESCRIPTION:変換関数（BCD形式→数値）
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint32      val             R   変換対象
 * RETURNS:
 *      uint32:変換結果
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint32 u32ValUtil_bcdToBinary(uint32 u32Val) {
	uint32 result = 0;		// 変換値
	uint32 coefficient = 1;	// 係数
	uint8 cnt;
	for (cnt = 0; cnt < 8; cnt++) {
		result += ((u32Val & 0x0000000F) % 10) * coefficient;
		u32Val = u32Val >> 4;
		coefficient *= 10;
	}
	return result;
}
/*****************************************************************************
 *
 * NAME: u32ValUtil_u8ToBinary
 *
 * DESCRIPTION:変換関数（uint8→二進表現）
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Val           R   変換対象
 *
 * RETURNS:
 *     uint32 二進数表現に変換した結果
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint32 u32ValUtil_u8ToBinary(uint8 u8Val) {
	uint32 u32Result = 0;
	uint32 u32AddVal = 1;
	while (u8Val != 0) {
		if ((u8Val & 0x01) == 0x01) {
			u32Result = u32Result + u32AddVal;
		}
		u8Val = u8Val >> 1;
		u32AddVal = u32AddVal * 10;
	}
	return u32Result;
}

/*****************************************************************************
 *
 * NAME: u32ValUtil_StringToU32
 *
 * DESCRIPTION:変換関数（数字文字列→uint32）
 *
 * PARAMETERS:      Name            RW  Usage
 *       char       pcStr           R   変換対象
 *       uint8      u8Begin         R   先頭文字インデックス
 *       uint8      u8Length        R   文字列長
 *
 * RETURNS:
 *     uint16 変換結果
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint32 u32ValUtil_stringToU32(char* pcStr, uint8 u8Begin, uint8 u8Length) {
	uint32 u32Result = 0;
	char cVal;
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < u8Length; u8Idx++) {
		cVal = pcStr[u8Begin + u8Idx];
		if (cVal >= '0' && cVal <= '9') {
			u32Result = u32Result * 10 + (uint32)(cVal - '0');
		}
	}
	return u32Result;
}

/*****************************************************************************
 *
 * NAME: u32ValUtil_dateToDays
 *
 * DESCRIPTION:変換関数（紀元1月1日からの経過日数への変換）
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint16     u16Year         R   年
 *       uint8      u8Month         R   月(1-12)
 *       uint8      u8Day           R   日(1-31)
 *
 * RETURNS:
 *     uint32 変換結果（1年1月1日からの経過日数）
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint32 u32ValUtil_dateToDays(uint16 u16Year, uint8 u8Month, uint8 u8Day) {
	// うるう年を考慮しないで当年1月1日までの日数を算出
	uint32 u32WkTotalDays = (uint32)(u16Year - 1) * 365;
	// 当年末までのうるう日数を加算
	u32WkTotalDays = u32WkTotalDays + u16Year / 4 - u16Year / 100 + u16Year / 400;
	// 各月（1-12）の月初日までの経過日数
	uint32 u32ConvDays[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
	u32WkTotalDays = u32WkTotalDays + u32ConvDays[u8Month - 1];
	u32WkTotalDays = u32WkTotalDays + u8Day - 1;
	// 当年のうるう日を調整
	if (bValUtil_isLeapYear(u16Year) && u8Month < 3) {
		u32WkTotalDays--;
	}
	return u32WkTotalDays;
}

/*****************************************************************************
 *
 * NAME: sValUtil_dayToDate
 *
 * DESCRIPTION:変換関数（紀元1月1日からの経過日数から日付への変換）
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint32     u32Days         R   紀元1月1日からの経過日数
 *
 * RETURNS:
 *     tsDate 変換結果（西暦の日付）
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC tsDate sValUtil_dayToDate(uint32 u32Days) {
	tsDate sDate;
	// 1年1月1日からの経過年数を年に変換、うるう年対応として400年単位で計算
	sDate.u16Year = (uint16)(((365 + u32Days) * 400) / 146097);
	// 当年1月1日からの経過日数を算出
	uint16 u16WkDays = (u32Days + 365) - sDate.u16Year * 365;
	u16WkDays = u16WkDays - sDate.u16Year / 4 + sDate.u16Year / 100 - sDate.u16Year / 400;
	// 当年のうるう日を算出
	uint8 u8LeapDay = 0;
	if (bValUtil_isLeapYear(sDate.u16Year)) {
		u8LeapDay = 1;
	}
	// 各月（1-12）の日数
	uint32 u32ConvDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	// 翌月1日までの日数と比較
	uint8 u8Idx = 0;
	uint16 u16ChkDays = u32ConvDays[u8Idx];
	while(u16WkDays >= u16ChkDays) {
		u16WkDays = u16WkDays - u16ChkDays;
		u16ChkDays = u32ConvDays[++u8Idx] + u8LeapDay;
		u8LeapDay = 0;
	}
	sDate.u8Month = u8Idx + 1;
	sDate.u8Day = u16WkDays + 1;
	return sDate;
}

/*****************************************************************************
 *
 * NAME: u16ValUtil_getRandVal
 *
 * DESCRIPTION:生成関数（乱数）
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *     uint16 生成した乱数
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint16 u16ValUtil_getRandVal() {
	// 継続的に乱数生成、割り込み処理無効
	vAHI_StartRandomNumberGenerator(E_AHI_RND_SINGLE_SHOT, E_AHI_INTS_DISABLED);
	// 乱数生成編集
	while(!bAHI_RndNumPoll());
	uint16 genVal = u16AHI_ReadRandomNumber();
	// 乱数生成器停止
	vAHI_StopRandomNumberGenerator();
	return genVal;
}

/*****************************************************************************
 *
 * NAME: u32ValUtil_getRandVal
 *
 * DESCRIPTION:生成関数（乱数）
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *     uint32 生成した乱数
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint32 u32ValUtil_getRandVal() {
	// 継続的に乱数生成、割り込み処理無効
	vAHI_StartRandomNumberGenerator(E_AHI_RND_CONTINUOUS, E_AHI_INTS_DISABLED);
	// 乱数生成編集
	while(!bAHI_RndNumPoll());
	uint32 genVal = u16AHI_ReadRandomNumber();
	while(!bAHI_RndNumPoll());
	genVal = (genVal << 16) + u16AHI_ReadRandomNumber();
	// 乱数生成器停止
	vAHI_StopRandomNumberGenerator();
	return genVal;
}

/*****************************************************************************
 *
 * NAME: vValUtil_setU8RandArray
 *
 * DESCRIPTION:生成関数（乱数配列）
 *
 * PARAMETERS:      Name            RW  Usage
 *     uint8*       u8RandArray     R   編集対象配列
 *     uint8        u8Len           R   編集サイズ
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vValUtil_setU8RandArray(uint8 u8RandArray[], uint8 u8Len) {
	// 継続的に乱数生成、割り込み処理無効
	vAHI_StartRandomNumberGenerator(E_AHI_RND_CONTINUOUS, E_AHI_INTS_DISABLED);
	TypeConverter converter;
	// 乱数生成編集
	uint8 u8Cnt = u8Len / 2;
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < u8Cnt; u8Idx++) {
		while(!bAHI_RndNumPoll());
		converter.u16Values[0] = u16AHI_ReadRandomNumber();
		u8RandArray[u8Idx * 2] = converter.u8Values[0];
		u8RandArray[u8Idx * 2 + 1] = converter.u8Values[1];
	}
	if ((u8Len % 2) == 1) {
		while(!bAHI_RndNumPoll());
		converter.u16Values[0] = u16AHI_ReadRandomNumber();
		u8RandArray[u8Idx * 2] = converter.u8Values[0];
	}
	// 乱数生成器停止
	vAHI_StopRandomNumberGenerator();
}

/*****************************************************************************
 *
 * NAME: vValUtil_setU32RandArray
 *
 * DESCRIPTION:生成関数（乱数配列）
 *
 * PARAMETERS:      Name            RW  Usage
 *     uint32*      u32RandArray    R   編集対象配列
 *     uint8        u8Len           R   編集サイズ
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vValUtil_setU32RandArray(uint32 u32RandArray[], uint8 u8Len) {
	// 継続的に乱数生成、割り込み処理無効
	vAHI_StartRandomNumberGenerator(E_AHI_RND_CONTINUOUS, E_AHI_INTS_DISABLED);
	// 乱数生成編集
	TypeConverter converter;
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < u8Len; u8Idx++) {
		while(!bAHI_RndNumPoll());
		converter.u16Values[0] = u16AHI_ReadRandomNumber();
		while(!bAHI_RndNumPoll());
		converter.u16Values[1] = u16AHI_ReadRandomNumber();
		u32RandArray[u8Idx] = converter.u32Values[0];
	}
	// 乱数生成器停止
	vAHI_StopRandomNumberGenerator();
}

/*****************************************************************************
 *
 * NAME: vValUtil_setRandString
 *
 * DESCRIPTION:生成関数（乱数文字列）
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8*      pu8RandString   R   編集対象文字列
 *      char*       pc8SrcString    R   編集元文字列
 *      uint8       u8Len           R   編集サイズ
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vValUtil_setRandString(char cRandString[], char cSrcString[], uint8 u8Len) {
	// 継続的に乱数生成、割り込み処理無効
	vAHI_StartRandomNumberGenerator(E_AHI_RND_CONTINUOUS, E_AHI_INTS_DISABLED);
	// 乱数生成編集
	uint8 u8SrcSize = strlen(cSrcString);
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < u8Len; u8Idx++) {
		while(!bAHI_RndNumPoll());
		cRandString[u8Idx] = cSrcString[u16AHI_ReadRandomNumber() % u8SrcSize];
	}
	// 乱数生成器停止
	vAHI_StopRandomNumberGenerator();
}

/****************************************************************************
 *
 * NAME: u8ValUtil_masking
 *
 * DESCRIPTION:マスキング処理
 *
 * PARAMETERS:      Name       RW  Usage
 *      uint8       u8Val      R   マスキング対象トークン
 *      uint8*      pu8Mask    R   マスクとなるトークン
 *      uint8       u8Mask     R   マスキングサイズ
 *
 * RETURNS:
 *      uint8:マスキング結果
 *
 ****************************************************************************/
PUBLIC uint8 u8ValUtil_masking(uint8 u8Val, uint8* pu8Mask, uint8 u8Size) {
	uint8 u8WkVal = u8Val;
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < u8Size; u8Idx++) {
		u8WkVal = u8WkVal ^ pu8Mask[u8Idx];
	}
	return u8WkVal;
}

/****************************************************************************
 *
 * NAME: u32ValUtil_masking
 *
 * DESCRIPTION:マスキング処理
 *
 * PARAMETERS:      Name       RW  Usage
 *      uint32      u32Val     R   マスキング対象トークン
 *      uint8*      pu8Mask    R   マスクとなるトークン
 *      uint8       u8Mask     R   マスキングサイズ
 *
 * RETURNS:
 *      uint32:マスキング結果
 *
 ****************************************************************************/
PUBLIC uint32 u32ValUtil_masking(uint32 u32Val, uint8* pu8Mask, uint8 u8Size) {
	uint32 u32WkVal = u32Val;
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < u8Size; u8Idx++) {
		u32WkVal = u32WkVal ^ ((uint32)pu8Mask[u8Idx] << (8 * (u8Idx % 4)));
	}
	return u32WkVal;
}
/****************************************************************************
 *
 * NAME: vValUtil_masking
 *
 * DESCRIPTION:マスキング処理
 *
 * PARAMETERS:      Name       RW  Usage
 *      uint8*      pu8Token   W   マスキング対象トークン
 *      uint8*      pu8Mask    R   マスクとなるトークン
 *      uint8       u8Mask     R   マスキングサイズ
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void vValUtil_masking(uint8* pu8Token, uint8* pu8Mask, uint8 u8Size) {
	uint8 u8Idx;
	for (u8Idx = 0; u8Idx < u8Size; u8Idx++) {
		pu8Token[u8Idx] = pu8Token[u8Idx] ^ pu8Mask[u8Idx];
	}
}

/****************************************************************************
 *
 * NAME: bValUtil_validDate
 *
 * DESCRIPTION:日付チェック
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint16      u16Year         R   年
 *      uint8       u8Month         R   月
 *      uint8       u8Day           R   日
 *
 * RETURNS:
 *     bool_t TRUE:チェックOK
 *
 ****************************************************************************/
PUBLIC bool_t bValUtil_validDate(uint16 u16Year, uint8 u8Month, uint8 u8Day) {
	if (u16Year < 1900 || u16Year > 2099) {
		return FALSE;
	}
	if (u8Month <= 0 || u8Month > 12) {
		return FALSE;
	}
	uint8 u8Days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	uint8 u8LastDay = u8Days[u8Month - 1];
	// うるう年判定
	if (u8Month == 2) {
		if(((u16Year % 4) == 0 && (u16Year % 100) != 0) || (u16Year % 400) == 0) {
			u8LastDay++;
		}
	}
	if (u8Day <= 0 || u8Day > u8LastDay) {
		return FALSE;
	}
	return TRUE;
}

/****************************************************************************
 *
 * NAME: bValUtil_validTime
 *
 * DESCRIPTION:時刻チェック
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       u8Hour          R   時
 *      uint8       u8Min           R   分
 *      uint8       u8Sec           R   秒
 *
 * RETURNS:
 *     bool_t TRUE:チェックOK
 *
 ****************************************************************************/
PUBLIC bool_t bValUtil_validTime(uint8 u8Hour, uint8 u8Min, uint8 u8Sec) {
	if (u8Hour >= 24) {
		return FALSE;
	}
	if (u8Min >= 60) {
		return FALSE;
	}
	return (u8Sec < 60);
}

/****************************************************************************
 *
 * NAME: bValUtil_isLeapYear
 *
 * DESCRIPTION:うるう年チェック
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint16      u16Year         R   年
 *
 * RETURNS:
 *     bool_t TRUE:うるう年
 *
 ****************************************************************************/
PUBLIC bool_t bValUtil_isLeapYear(uint16 u16Year) {
	// 0年（紀元前1年）はうるう年として判断する
	return ((u16Year % 4 == 0) && (u16Year % 100 != 0)) || (u16Year % 400 == 0);
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
