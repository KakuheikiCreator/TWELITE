/****************************************************************************
 *
 * MODULE :Value Utility functions header file
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
#ifndef  VAL_UTIL_H_INCLUDED
#define  VAL_UTIL_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/** Analog Input Buffer Size */
#ifndef IO_UTIL_AI_BUFF_SIZE
	#define IO_UTIL_AI_BUFF_SIZE      (30)
#endif
/** Digital Input Buffer Size */
#ifndef IO_UTIL_DI_BUFF_SIZE
	#define IO_UTIL_DI_BUFF_SIZE      (30)
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/** 構造体：日付保持 */
typedef struct {
	uint16 u16Year;
	uint8  u8Month;
	uint8  u8Day;
} tsDate;

/** 構造体：時刻保持 */
typedef struct {
	uint8 u8Hour;
	uint8 u8Minutes;
	uint8 u8Seconds;
} tsTime;

/** 共用体：変数コンバート用 */
typedef union {
	uint64 u64Values[4];
	uint32 u32Values[8];
	uint16 u16Values[16];
	uint8  u8Values[32];
	int    iValues[8];
} TypeConverter;

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
/** 変換関数：数値→BCD形式 */
PUBLIC uint32 u64ValUtil_binaryToBcd(uint32 u32Val);
/** 変換関数：BCD形式→数値 */
PUBLIC uint32 u32ValUtil_bcdToBinary(uint32 u32Val);
/** 変換関数：uint8→二進表現 */
PUBLIC uint32 u32ValUtil_u8ToBinary(uint8 u8Val);
/** 数字文字列から数値変換 */
PUBLIC uint32 u32ValUtil_stringToU32(char* pcStr, uint8 u8Begin, uint8 u8Length);
/** 紀元1月1日からの経過日数への変換 */
PUBLIC uint32 u32ValUtil_dateToDays(uint16 u16Year, uint8 u8Month, uint8 u8Day);
/** 紀元1月1日からの経過日数から日付への変換 */
PUBLIC tsDate sValUtil_dayToDate(uint32 u32Days);
/** 生成関数：乱数 */
PUBLIC uint16 u16ValUtil_getRandVal();
/** 生成関数：乱数 */
PUBLIC uint32 u32ValUtil_getRandVal();
/** 生成関数：乱数配列（uint8） */
PUBLIC void vValUtil_setU8RandArray(uint8 u8RandArray[], uint8 u8Len);
/** 生成関数：乱数配列（uint16） */
PUBLIC void vValUtil_setU16RandArray(uint16 u16RandArray[], uint8 u8Len);
/** 生成関数：乱数配列（uint32） */
PUBLIC void vValUtil_setU32RandArray(uint32 u32RandArray[], uint8 u8Len);
/** 生成関数：乱数文字列 */
PUBLIC void vValUtil_setRandString(char cRandString[], char cSrcString[], uint8 u8Len);
/** マスキング処理(uint8) */
PUBLIC uint8 u8ValUtil_masking(uint8 u8Val, uint8* pu8Mask, uint8 u8Size);
/** マスキング処理(uint32) */
PUBLIC uint32 u32ValUtil_masking(uint32 u32Val, uint8* pu8Mask, uint8 u8Size);
/** マスキング処理 */
PUBLIC void vValUtil_masking(uint8* pu8Token, uint8* pu8Mask, uint8 u8Size);
/** 日付チェック */
PUBLIC bool_t bValUtil_validDate(uint16 u16Year, uint8 u8Month, uint8 u8Day);
/** 時刻チェック */
PUBLIC bool_t bValUtil_validTime(uint8 u8Hour, uint8 u8Min, uint8 u8Sec);
/** うるう年チェック */
PUBLIC bool_t bValUtil_isLeapYear(uint16 u16Year);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* VAL_UTIL_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

