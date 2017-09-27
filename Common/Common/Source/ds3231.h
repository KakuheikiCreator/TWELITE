/****************************************************************************
 *
 * MODULE :DS3231 RTC Driver functions header file
 *
 * CREATED:2015/05/03 19:34:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:DS3231 RTC draiver
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
#ifndef  DS3231_H_INCLUDED
#define  DS3231_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// I2C Address
#define I2C_ADDR_DS3231      (0x68)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/**
 * DS3231用の日時型
 */
typedef struct {
	uint16 u16Year;			// 年：1900-2099
	uint8 u8Month;			// 月：1-12
	uint8 u8Day;			// 日：1-31
	uint8 u8Wday;			// 曜日：1-7
	uint8 u8Hour;			// 時：0-23
	uint8 u8Minutes;		// 分：0-59
	uint8 u8Seconds;		// 秒：0-59
	bool_t bDayValidFlg;		// 有効無効フラグ（日）：TRUE or FALSE
	bool_t bWdayValidFlg;	// 有効無効フラグ（曜日）：TRUE or FALSE
	bool_t bHourValidFlg;	// 有効無効フラグ（時）：TRUE or FALSE
	bool_t bMinutesValidFlg;	// 有効無効フラグ（分）：TRUE or FALSE
	bool_t bSecondsValidFlg;	// 有効無効フラグ（秒）：TRUE or FALSE
} DS3231_datetime;

/**
 * 制御バイト
 */
typedef struct {
	bool_t bEoscFlg;		// Enable Oscillator：発振有効フラグ
	bool_t bBbsqwFlg;		// Battery-Backed Square-Wave Enable：？
	bool_t bConvFlg;		// Convert Temperature：温度計制御
	bool_t bRs1;			// Rate Select 1：発振レート設定
	bool_t bRs2;			// Rate Select 2：（00:1Hz, 01:1024Hz 10:4096Hz 11:8192Hz）
	bool_t bIntcnFlg;		// Interrupt Control：割り込み制御
	bool_t bA1ieFlg;		// Alarm 1 Interrupt Enable：アラーム１制御
	bool_t bA2ieFlg;		// Alarm 2 Interrupt Enable：アラーム２制御
} DS3231_control;

/**
 * DS3231用のステータス情報
 */
typedef struct {
	bool_t bOsFlg;			// Oscillator Stop Flag：発振停止有効フラグ
	bool_t bEn32khzFlg;		// Enable 32kHz Output：32KHz発振出力フラグ
	bool_t bBusyFlg;		// Busy：ビジーフラグ
	bool_t bAlarm1Flg;		// Alarm 1 Flag：アラーム１フラグ
	bool_t bAlarm2Flg;		// Alarm 2 Flag：アラーム２フラグ
} DS3231_status;

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
/** デバイス選択 */
PUBLIC bool_t bDS3231_deviceSelect(uint8 address);
/** アラーム日時１情報取得 */
PUBLIC bool_t bDS3231_getAlarm1(DS3231_datetime *datetime);
/** アラーム日時２情報取得 */
PUBLIC bool_t bDS3231_getAlarm2(DS3231_datetime *datetime);
/** 制御情報取得 */
PUBLIC bool_t bDS3231_getControl(DS3231_control *control);
/** 現在日時情報取得 */
PUBLIC bool_t bDS3231_getDatetime(DS3231_datetime *datetime);
/** ステータス情報取得 */
PUBLIC bool_t bDS3231_getStatus(DS3231_status *status);
/** 現在温度×１００の値取得 */
PUBLIC bool_t bDS3231_getTemperature(int *temperature);
/** アラーム日時１情報設定 */
PUBLIC bool_t bDS3231_setAlarm1(DS3231_datetime *datetime);
/** アラーム日時２情報設定 */
PUBLIC bool_t bDS3231_setAlarm2(DS3231_datetime *datetime);
/** 制御情報設定 */
PUBLIC bool_t bDS3231_setControl(DS3231_control *control);
/** 現在日時情報設定 */
PUBLIC bool_t bDS3231_setDatetime(DS3231_datetime *datetime);
/** 現在日時情報設定 */
PUBLIC uint8 u8DS3231_setDatetime(DS3231_datetime *datetime);
/** 現在時刻の入力チェック */
PUBLIC bool_t bDS3231_validDatetime(DS3231_datetime *datetime);
/** アラーム時刻の入力チェック */
PUBLIC bool_t bDS3231_validAlarmTime(DS3231_datetime *datetime);
/** 曜日文字列(３文字)への変換 */
PUBLIC const char* cpDS3231_convSimpleWeekday(uint8 wday);
/** 曜日文字列への変換 */
PUBLIC const char* cpDS3231_convWeekday(uint8 wday);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* DS3231_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
