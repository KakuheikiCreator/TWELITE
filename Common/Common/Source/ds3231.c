/****************************************************************************
 *
 * MODULE :DS3231 RTC Driver functions source file
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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>

#include "ToCoNet.h"

#include "ds3231.h"
#include "i2c_util.h"
#include "framework.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// メモリアドレス
#define DS3231_ADDR_NOW_SEC            (0x00)	// 現在日時：秒
#define DS3231_ADDR_NOW_MIN            (0x01)	// 現在日時：分
#define DS3231_ADDR_NOW_HOUR           (0x02)	// 現在日時：時
#define DS3231_ADDR_NOW_WDAY           (0x03)	// 現在日時：曜日
#define DS3231_ADDR_NOW_DAY            (0x04)	// 現在日時：日
#define DS3231_ADDR_NOW_MONTH          (0x05)	// 現在日時：２１世紀フラグ+月
#define DS3231_ADDR_NOW_YEAR           (0x06)	// 現在日時：年
#define DS3231_ADDR_AL1_SEC            (0x07)	// アラーム日時１：秒
#define DS3231_ADDR_AL1_MIN            (0x08)	// アラーム日時１：分
#define DS3231_ADDR_AL1_HOUR           (0x09)	// アラーム日時１：時
#define DS3231_ADDR_AL1_WDAY           (0x0A)	// アラーム日時１：曜日
#define DS3231_ADDR_AL1_DAY            (0x0A)	// アラーム日時１：日
#define DS3231_ADDR_AL2_MIN            (0x0B)	// アラーム日時２：分
#define DS3231_ADDR_AL2_HOUR           (0x0C)	// アラーム日時２：時
#define DS3231_ADDR_AL2_WDAY           (0x0D)	// アラーム日時２：曜日
#define DS3231_ADDR_AL2_DAY            (0x0D)	// アラーム日時２：日
#define DS3231_ADDR_CONTROL            (0x0E)	// 制御
#define DS3231_ADDR_STATUS             (0x0F)	// ステータス
#define DS3231_ADDR_AGING_OFFSET       (0x10)	// オフセット
#define DS3231_ADDR_TEMPERATURE1       (0x11)	// 1bit(符号) + 7bit(温度)：単位1℃
#define DS3231_ADDR_TEMPERATURE2       (0x12)	// 2bit(温度) + 6bit(0)：単位0.25℃

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/** 曜日表記（簡易） */
PRIVATE const char DS3231_SIMPLE_WEEKDAY[8][4] =
	{"Err", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
/** 曜日表記 */
PRIVATE const char DS3231_WEEKDAY[8][10] =
	{"Err      ", "Sunday   ", "Monday   ", "Tuesday  ", "Wednesday", "Thursday ", "Friday   ", "Saturday "};
/** デバイスアドレス */
PRIVATE uint8 u8DS3231_device_addr = I2C_ADDR_DS3231;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
// データの読み込み
PRIVATE bool_t bDS3231_readData(uint8 address, uint8 *data, uint8 len);
// データの書き込み
PRIVATE bool_t bDS3231_writeData(uint8 address, const uint8 *data, uint8 len);
// 値変換：２進化十進→バイナリ
PRIVATE uint8 u8DS3231_bcdToDec(uint8 num);
// 値変換：２進化十進→時間
PRIVATE uint8 u8DS3231_bcdToHour(uint8 num);
// 値変換：２進化十進→曜日
PRIVATE uint8 u8DS3231_bcdToWday(uint8 num);
// 値変換：２進化十進→日
PRIVATE uint8 u8DS3231_bcdToDay(uint8 num);
// 値変換：バイナリ→指定された桁のビット値
PRIVATE uint8 u8DS3231_bitVal(uint8 val, uint8 num);
// 値変換：バイナリ→２進化十進
PRIVATE uint8 u8DS3231_decToBcd(uint8 num);
// 値変換：バイナリ→アラーム形式の２進化十進
PRIVATE uint8 u8DS3231_decToAlarmBcd(uint8 num, bool_t validFlg);
// 有効アラーム項目判定
PRIVATE bool_t bDS3231_isValidAlarm(uint8 num);

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/*****************************************************************************
 *
 * NAME: bDS3231_deviceSelect
 *
 * DESCRIPTION:利用するデバイスの選択を行う
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       address         R   選択するデバイスのI2Cアドレス
 *
 * RETURNS:
 *      TRUE:選択成功
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bDS3231_deviceSelect(uint8 address) {
	// I2Cアドレスを設定
	u8DS3231_device_addr = address;
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bDS3231_getAlarm1
 *
 * DESCRIPTION:アラーム日時１の情報取得
 *
 * PARAMETERS:      Name            RW  Usage
 * DS3231_datetime* datetime        W   取得したアラーム日時を設定する構造体
 *
 * RETURNS:
 *      TRUE :取得成功
 *      FALSE:取得失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bDS3231_getAlarm1(DS3231_datetime *datetime) {
	uint8 data[4];
	// 日付情報の読み込み処理
	if (!bDS3231_readData(DS3231_ADDR_AL1_SEC, data, 4)) return FALSE;
	// 読み込み値の編集
	datetime->u8Seconds = u8DS3231_bcdToDec(data[0] & 0x7f);
	datetime->bSecondsValidFlg = bDS3231_isValidAlarm(data[0]);
	datetime->u8Minutes = u8DS3231_bcdToDec(data[1] & 0x7f);
	datetime->bMinutesValidFlg = bDS3231_isValidAlarm(data[1]);
	datetime->u8Hour    = u8DS3231_bcdToHour(data[2]);
	datetime->bHourValidFlg    = bDS3231_isValidAlarm(data[2]);
	// 日付／曜日判定
	if ((data[3] & 0x40) == 0) {
		datetime->u8Wday    = 0;
		datetime->bWdayValidFlg    = FALSE;
		datetime->u8Day     = u8DS3231_bcdToDay(data[3]);
		datetime->bDayValidFlg     = bDS3231_isValidAlarm(data[3]);
	} else {
		datetime->u8Wday    = u8DS3231_bcdToWday(data[3]);
		datetime->bWdayValidFlg    = bDS3231_isValidAlarm(data[3]);
		datetime->u8Day     = 0;
		datetime->bDayValidFlg     = FALSE;
	}
	datetime->u8Month   = 0;
	datetime->u16Year    = 0;
	// 成功通知
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bDS3231_getAlarm2
 *
 * DESCRIPTION:アラーム日時２の情報取得
 *
 * PARAMETERS:      Name            RW  Usage
 * DS3231_datetime* datetime        W   取得したアラーム日時を設定する構造体
 *
 * RETURNS:
 *      TRUE :取得成功
 *      FALSE:取得失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bDS3231_getAlarm2(DS3231_datetime *datetime) {
	uint8 data[3];
	if (!bDS3231_readData(DS3231_ADDR_AL2_MIN, data, 3)) return FALSE;
	datetime->u8Seconds = 0;
	datetime->bSecondsValidFlg = FALSE;
	datetime->u8Minutes = u8DS3231_bcdToDec(data[0] & 0x7f);
	datetime->bMinutesValidFlg = bDS3231_isValidAlarm(data[0]);
	datetime->u8Hour    = u8DS3231_bcdToHour(data[1]);
	datetime->bHourValidFlg    = bDS3231_isValidAlarm(data[1]);
	// 日付／曜日判定
	if ((data[2] & 0x40) == 0) {
		datetime->u8Wday    = 0;
		datetime->bWdayValidFlg    = FALSE;
		datetime->u8Day     = u8DS3231_bcdToDay(data[2]);
		datetime->bDayValidFlg     = bDS3231_isValidAlarm(data[2]);
	} else {
		datetime->u8Wday    = u8DS3231_bcdToWday(data[2]);
		datetime->bWdayValidFlg    = bDS3231_isValidAlarm(data[2]);
		datetime->u8Day     = 0;
		datetime->bDayValidFlg     = FALSE;
	}
	datetime->u8Month   = 0;
	datetime->u16Year    = 0;
	// 成功通知
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bDS3231_getControl
 *
 * DESCRIPTION:制御情報の取得
 *
 * PARAMETERS:      Name            RW  Usage
 * DS3231_datetime* control         W   取得した制御情報を設定する構造体
 *
 * RETURNS:
 *      TRUE :取得成功
 *      FALSE:取得失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bDS3231_getControl(DS3231_control *control) {
	// 制御バイトの読み込み
	uint8 cntrData[1];
	if (!bDS3231_readData(DS3231_ADDR_CONTROL, cntrData, 1)) return FALSE;
	// 制御情報の編集
	control->bEoscFlg  = (u8DS3231_bitVal(cntrData[0], 7) == 1);
	control->bBbsqwFlg = (u8DS3231_bitVal(cntrData[0], 6) == 1);
	control->bConvFlg  = (u8DS3231_bitVal(cntrData[0], 5) == 1);
	control->bRs2      = (u8DS3231_bitVal(cntrData[0], 4) == 1);
	control->bRs1      = (u8DS3231_bitVal(cntrData[0], 3) == 1);
	control->bIntcnFlg = (u8DS3231_bitVal(cntrData[0], 2) == 1);
	control->bA2ieFlg  = (u8DS3231_bitVal(cntrData[0], 1) == 1);
	control->bA1ieFlg  = (u8DS3231_bitVal(cntrData[0], 0) == 1);
	// 成功通知
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bDS3231_getDatetime
 *
 * DESCRIPTION:現在日時情報の取得
 *
 * PARAMETERS:      Name            RW  Usage
 * DS3231_datetime* datetime        W   取得した現在日時を設定する構造体
 *
 * RETURNS:
 *      TRUE :取得成功
 *      FALSE:取得失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bDS3231_getDatetime(DS3231_datetime *datetime) {
	uint8 data[7];
	if (!bDS3231_readData(DS3231_ADDR_NOW_SEC, data, 7)) return FALSE;
	datetime->u8Seconds = u8DS3231_bcdToDec(data[0] & 0x7f);
	datetime->u8Minutes = u8DS3231_bcdToDec(data[1] & 0x7f);
	// 24時間表記をデフォルトとする
	datetime->u8Hour    = u8DS3231_bcdToHour(data[2]);
	datetime->u8Wday    = u8DS3231_bcdToDec(data[3] & 0x07);
	datetime->u8Day     = u8DS3231_bcdToDec(data[4] & 0x3f);
	datetime->u8Month   = u8DS3231_bcdToDec(data[5] & 0x1f);
	datetime->u16Year    = 1900 + u8DS3231_bitVal(data[5], 7) * 100
	                            + u8DS3231_bcdToDec(data[6]);
	// 有効無効フラグ初期化
	datetime->bDayValidFlg     = TRUE;
	datetime->bWdayValidFlg    = TRUE;
	datetime->bHourValidFlg    = TRUE;
	datetime->bMinutesValidFlg = TRUE;
	datetime->bSecondsValidFlg = TRUE;
	// 成功通知
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bDS3231_getStatus
 *
 * DESCRIPTION:ステータス情報の取得
 *
 * PARAMETERS:      Name            RW  Usage
 * DS3231_datetime* status          W   取得したステータス情報を設定する構造体
 *
 * RETURNS:
 *      TRUE :取得成功
 *      FALSE:取得失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bDS3231_getStatus(DS3231_status *status) {
	// ステータスバイトの読み込み
	uint8 statusData[1];
	if (!bDS3231_readData(DS3231_ADDR_STATUS, statusData, 1)) return FALSE;
	// ステータス情報の編集
	status->bOsFlg      = u8DS3231_bitVal(statusData[0], 7);
	status->bEn32khzFlg = u8DS3231_bitVal(statusData[0], 3);
	status->bBusyFlg    = u8DS3231_bitVal(statusData[0], 2);
	status->bAlarm2Flg  = u8DS3231_bitVal(statusData[0], 1);
	status->bAlarm1Flg  = u8DS3231_bitVal(statusData[0], 0);
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: iDS3231_getTemperature
 *
 * DESCRIPTION:現在温度の取得
 *
 * PARAMETERS:      Name            RW  Usage
 * int*             temperature     W   現在温度を百倍した値
 *
 * RETURNS:
 *      TRUE :取得成功
 *      FALSE:取得失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bDS3231_getTemperature(int *temperature) {
	uint8 data[2];
	if (!bDS3231_readData(DS3231_ADDR_TEMPERATURE1, data, 2)) return FALSE;
	int tempVal = (int)((data[0] & 0x7f) * 100);
	tempVal   = tempVal + (int)((data[1] >> 6) * 25);
	int sign  = (int)((data[0] >> 7) & 0x01);
	int multi = (sign * -1) + (int)(sign ^ 0x01);
	*temperature = tempVal * multi;
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bDS3231_setAlarm1
 *
 * DESCRIPTION:アラーム日時１情報の設定
 *
 * PARAMETERS:      Name            RW  Usage
 * DS3231_datetime* datetime        R   アラーム日時１の設定情報を保持した構造体
 *
 * RETURNS:
 *      TRUE :設定成功
 *      FALSE:設定失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bDS3231_setAlarm1(DS3231_datetime *datetime) {
	// 入力値チェック
	if (!bDS3231_validAlarmTime(datetime)) return FALSE;
	// 設定値編集
	uint8 data[4];
	data[0] = u8DS3231_decToAlarmBcd(datetime->u8Seconds, datetime->bSecondsValidFlg);
	data[1] = u8DS3231_decToAlarmBcd(datetime->u8Minutes, datetime->bMinutesValidFlg);
	// 24時間表記をデフォルトとする
	data[2] = u8DS3231_decToAlarmBcd(datetime->u8Hour, datetime->bHourValidFlg);
	// 日付曜日判定：日付優先とする
	if (datetime->bDayValidFlg) {
		data[3] = u8DS3231_decToAlarmBcd(datetime->u8Day, datetime->bDayValidFlg);
	} else {
		data[3] = u8DS3231_decToAlarmBcd(datetime->u8Wday, datetime->bWdayValidFlg) | 0x40;
	}
	// 書き込み処理
	return bDS3231_writeData(DS3231_ADDR_AL1_SEC, data, 4);
}

/*****************************************************************************
 *
 * NAME: bDS3231_setAlarm2
 *
 * DESCRIPTION:アラーム日時２情報の設定
 *
 * PARAMETERS:      Name            RW  Usage
 * DS3231_datetime* datetime        R   アラーム日時１の設定情報を保持した構造体
 *
 * RETURNS:
 *      TRUE :設定成功
 *      FALSE:設定失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bDS3231_setAlarm2(DS3231_datetime *datetime) {
	// 入力値チェック
	if (!bDS3231_validAlarmTime(datetime)) return FALSE;
	// 設定値編集
	uint8 data[3];
	data[0] = u8DS3231_decToAlarmBcd(datetime->u8Minutes, datetime->bMinutesValidFlg);
	// 24時間表記をデフォルトとする
	data[1] = u8DS3231_decToAlarmBcd(datetime->u8Hour, datetime->bHourValidFlg);
	// 日付曜日判定：日付優先とする
	if (datetime->bDayValidFlg) {
		data[2] = u8DS3231_decToAlarmBcd(datetime->u8Day, datetime->bDayValidFlg);
	} else {
		data[2] = u8DS3231_decToAlarmBcd(datetime->u8Wday, datetime->bWdayValidFlg) | 0x40;
	}
	// 書き込み処理
	return bDS3231_writeData(DS3231_ADDR_AL2_MIN, data, 3);
}

/*****************************************************************************
 *
 * NAME: bDS3231_setControl
 *
 * DESCRIPTION:制御情報の設定
 *
 * PARAMETERS:      Name            RW  Usage
 * DS3231_control*  control         R   制御情報を保持した構造体
 *
 * RETURNS:
 *      TRUE :設定成功
 *      FALSE:設定失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bDS3231_setControl(DS3231_control *control) {
	// 制御バイトの読み込み
	uint8 cntrData[1];
	// 制御情報の編集
	cntrData[0] = (control->bEoscFlg * 0x80) | (control->bBbsqwFlg * 0x40)
				| (control->bConvFlg * 0x20) | (control->bRs2  * 0x10)
				| (control->bRs1   * 0x08) | (control->bIntcnFlg * 0x04)
				| (control->bA2ieFlg * 0x02) | control->bA1ieFlg;
	// 書き込み処理
	return bDS3231_writeData(DS3231_ADDR_CONTROL, cntrData, 1);
}

/*****************************************************************************
 *
 * NAME: bDS3231_setDatetime
 *
 * DESCRIPTION:現在日時の設定
 *
 * PARAMETERS:      Name            RW  Usage
 * DS3231_datetime* datetime        R   現在日時情報を保持した構造体
 *
 * RETURNS:
 *      TRUE :設定成功
 *      FALSE:設定失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bDS3231_setDatetime(DS3231_datetime *datetime) {
	// 入力値チェック
	if (!bDS3231_validDatetime(datetime)) return FALSE;
	// 設定値編集
	uint8 data[7];
	data[0] = u8DS3231_decToBcd(datetime->u8Seconds);
	data[1] = u8DS3231_decToBcd(datetime->u8Minutes);
	data[2] = u8DS3231_decToBcd(datetime->u8Hour);	// 24時間表記をデフォルトとする
	data[3] = u8DS3231_decToBcd(datetime->u8Wday);
	data[4] = u8DS3231_decToBcd(datetime->u8Day);
	if (datetime->u16Year < 2000) {
		data[5] = u8DS3231_decToBcd(datetime->u8Month);
	} else {
		data[5] = u8DS3231_decToBcd(datetime->u8Month) | 0x80;
	}
	data[6] = u8DS3231_decToBcd(datetime->u16Year % 100);
	// 書き込み処理
	return bDS3231_writeData(DS3231_ADDR_NOW_SEC, data, 7);
}

/*****************************************************************************
 *
 * NAME: bDS3231_validDatetime
 *
 * DESCRIPTION:日時情報の入力チェック
 *
 * PARAMETERS:      Name            RW  Usage
 * DS3231_datetime* datetime        R   日時情報を保持した構造体
 *
 * RETURNS:
 *      TRUE :有効値
 *      FALSE:無効値
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bDS3231_validDatetime(DS3231_datetime *datetime) {
	if (datetime == NULL) return FALSE;
	if (datetime->u8Seconds > 59) return FALSE;
	if (datetime->u8Minutes > 59) return FALSE;
	if (datetime->u8Hour > 23) return FALSE;
	if (datetime->u8Wday == 0 || datetime->u8Wday > 7) return FALSE;
	if (datetime->u8Day > 31) return FALSE;
	if (datetime->u8Month > 12) return FALSE;
	return (datetime->u16Year >= 1900 && datetime->u16Year <= 2099);
}

/*****************************************************************************
 *
 * NAME: bDS3231_validAlarmTime
 *
 * DESCRIPTION:アラーム時刻情報の入力チェック
 *
 * PARAMETERS:      Name            RW  Usage
 * DS3231_datetime* datetime        R   アラーム時刻情報を保持した構造体
 *
 * RETURNS:
 *      TRUE :有効値
 *      FALSE:無効値
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bDS3231_validAlarmTime(DS3231_datetime *datetime) {
	if (datetime == NULL) return FALSE;
	if (datetime->u8Seconds > 59 && datetime->bSecondsValidFlg) return FALSE;
	if (datetime->u8Minutes > 59 && datetime->bMinutesValidFlg) return FALSE;
	if (datetime->u8Hour > 23 && datetime->bHourValidFlg) return FALSE;
	if (datetime->u8Wday > 7 && datetime->bWdayValidFlg) return FALSE;
	return !(datetime->u8Day > 31 && datetime->bDayValidFlg);
}

/*****************************************************************************
 *
 * NAME: bDS3231_convSimpleWeekday
 *
 * DESCRIPTION:曜日を表す文字列(３文字)への変換
 *
 * PARAMETERS:      Name            RW  Usage
 * uint8            wday            R   曜日の値（1-7）
 *
 * RETURNS:曜日を表す文字列(３文字)
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC const char* cpDS3231_convSimpleWeekday(uint8 wday) {
	if (wday > 7 ) return DS3231_SIMPLE_WEEKDAY[0];
	return DS3231_SIMPLE_WEEKDAY[wday];
}

/*****************************************************************************
 *
 * NAME: bDS3231_convWeekday
 *
 * DESCRIPTION:曜日を表す文字列への変換
 *
 * PARAMETERS:      Name            RW  Usage
 * uint8            wday            R   曜日の値（1-7）
 *
 * RETURNS:曜日を表す文字列
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC const char* cpDS3231_convWeekday(uint8 wday) {
	if (wday > 7) return DS3231_WEEKDAY[0];
	return DS3231_WEEKDAY[wday];
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/*****************************************************************************
 *
 * NAME: bDS3231_readData
 *
 * DESCRIPTION:デバイスからのデータの読み出し
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       address         R   デバイス上のメモリアドレス
 *      uint8*      data            W   読み込みデータを書き込む配列
 *      uint8       len             R   読み込みバイト数
 *
 * RETURNS:
 *      TRUE :読み込み成功
 *      FALSE:読み込み失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE bool_t bDS3231_readData(uint8 address, uint8 *data, uint8 len) {
	// 書き込み開始宣言
	if (!bI2C_startWrite(u8DS3231_device_addr)) {
		bI2C_stopACK();
		return FALSE;
	}
	// 参照アドレス書き込み
	if (u8I2C_write(address) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
#ifdef DEBUG
		vfPrintf(&sSerStream, "MS:%08d bDS3231_readData Error:2\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
#endif
		return FALSE;
	}
	// 読み込み開始宣言
	if(!bI2C_startRead(u8DS3231_device_addr)) {
		bI2C_stopACK();
#ifdef DEBUG
		vfPrintf(&sSerStream, "MS:%08d bDS3231_readData Error:3\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
#endif
		return FALSE;
	}
	// データの読み込み処理
	if (!bI2C_read(data, len, FALSE)) {
		bI2C_stopACK();
#ifdef DEBUG
		vfPrintf(&sSerStream, "MS:%08d bDS3231_readData Error:4\n", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
#endif
		return FALSE;
	}
	// I2C通信完了
	return bI2C_stopACK();
}

/*****************************************************************************
 *
 * NAME: bDS3231_writeData
 *
 * DESCRIPTION:デバイスへのデータの書き込み
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       address         R   デバイス上のメモリアドレス
 *      uint8*      data            W   書き込みデータ配列
 *      uint8       len             R   書き込みバイト数
 *
 * RETURNS:
 *      TRUE :書き込み成功
 *      FALSE:書き込み失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE bool_t bDS3231_writeData(uint8 address, const uint8 *data, uint8 len) {
	// 入力チェック
	if (len == 0) return TRUE;
	// 書き込み開始宣言
	if (!bI2C_startWrite(u8DS3231_device_addr)) {
		bI2C_stopACK();
		return FALSE;
	}
	// 書き込みアドレスの送信
	if (u8I2C_write(address) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
		return FALSE;
	}
	// データの書き込みループ
	while (len > 1) {
		// 書き込みデータの送信
		if (u8I2C_write(*data) != I2CUTIL_STS_ACK) {
			bI2C_stopACK();
			return FALSE;
		}
		data++;
		len--;
	}
	// 終端データの送信
	if (u8I2C_writeStop(*data) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
		return FALSE;
	}
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: u8DS3231_bcdToDec
 *
 * DESCRIPTION:値変換（２進化十進→バイナリ）
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       num             R   変換対象値
 *
 * RETURNS:
 *      uint8:変換結果の値
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE uint8 u8DS3231_bcdToDec(uint8 num) {
	return ((num >> 4) * 10 + (num & 0x0f));
}

/*****************************************************************************
 *
 * NAME: u8DS3231_bcdToHour
 *
 * DESCRIPTION:値変換（２進化十進→時間）
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       num             R   変換対象値
 *
 * RETURNS:
 *      uint8:変換結果の時間
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE uint8 u8DS3231_bcdToHour(uint8 num) {
	// 24時間表記判定
	if ((num & 0x40) == 0) {
		return u8DS3231_bcdToDec(num & 0x3f);
	}
	// AM/PM表記
	return u8DS3231_bitVal(num, 5) * 12 + u8DS3231_bcdToDec(num & 0x1f);
}

/*****************************************************************************
 *
 * NAME: u8DS3231_bcdToWday
 *
 * DESCRIPTION:値変換（２進化十進→曜日）
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       num             R   変換対象値
 *
 * RETURNS:
 *      uint8:変換結果の曜日(1-7)
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE uint8 u8DS3231_bcdToWday(uint8 num) {
	// 日付表記判定
	if ((num & 0x40) == 0) return 0;
	// 曜日表記
	return u8DS3231_bcdToDec(num & 0x0f);
}

/*****************************************************************************
 *
 * NAME: u8DS3231_bcdToDay
 *
 * DESCRIPTION:値変換（２進化十進→日）
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       num             R   変換対象値
 *
 * RETURNS:
 *      uint8:変換結果の日
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE uint8 u8DS3231_bcdToDay(uint8 num) {
	// 日付表記判定
	if ((num & 0x40) != 0) return 0;
	// 時刻表記
	return u8DS3231_bcdToDec(num & 0x3f);
}

/*****************************************************************************
 *
 * NAME: u8DS3231_bitVal
 *
 * DESCRIPTION:値変換（バイナリ→指定された桁のビット値）
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       val             R   変換対象値
 *      uint8       num             R   対象桁位置
 *
 * RETURNS:
 *      uint8:変換結果の値
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE uint8 u8DS3231_bitVal(uint8 val, uint8 num) {
	return ((0x01 << num) & val) >> num;
}

/*****************************************************************************
 *
 * NAME: u8DS3231_decToBcd
 *
 * DESCRIPTION:値変換（バイナリ→２進化十進）
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       num             R   変換対象値
 *
 * RETURNS:
 *      uint8:変換結果の値
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE uint8 u8DS3231_decToBcd(uint8 num) {
	return ((num / 10 * 16) + (num % 10));
}

/*****************************************************************************
 *
 * NAME: u8DS3231_decToAlarmBcd
 *
 * DESCRIPTION:値変換（バイナリ→アラーム形式の２進化十進）
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       num             R   変換対象値
 *      bool_t      validFlg        R   アラームの有効無効
 *
 * RETURNS:
 *      uint8:変換結果の値
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE uint8 u8DS3231_decToAlarmBcd(uint8 num, bool_t validFlg) {
	if (validFlg) {
		return u8DS3231_decToBcd(num);
	} else {
		return (0x80 | u8DS3231_decToBcd(num));
	}
}

/*****************************************************************************
 *
 * NAME: bDS3231_isValidAlarm
 *
 * DESCRIPTION:有効アラーム項目判定
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       num             R   チェック対象値
 *
 * RETURNS:
 *      TRUE :有効
 *      FALSE:無効
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE bool_t bDS3231_isValidAlarm(uint8 num) {
	return ((num & 0x80) == 0);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
