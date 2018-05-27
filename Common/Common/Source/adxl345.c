/****************************************************************************
 *
 * MODULE :ADXL345 Driver functions source file
 *
 * CREATED:2016//05/03 23:15:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:ADXL345 3-AXIS SENSOR driver
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

#include "ToCoNet_use_mod.h"
#include "ToCoNet.h"
#include "ToCoNet_mod_prototype.h"

#include "adxl345.h"
#include "i2c_util.h"
#include "timer_util.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// レジスタアドレス（Read用）
#define ADXL345_CALIBRATION_CNT        (10)
#define ADXL345_READ_START             (29)
#define ADXL345_READ_LENGTH            (29)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/**
 * 加速度編集用共用体
 */
typedef union {
	uint8 u8Axes[2];
	int16 i16Axes;
} tuConvU8ToI16;

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
// デバイス情報
PRIVATE tsADXL345_Info *spADXL345_Info;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
// アクセス待ち処理
PRIVATE void vADXL345_wait();
// データの読み込み
PRIVATE bool_t bADXL345_readData(uint8 address, uint8 *data, uint8 len);
// データの書き込み
PRIVATE bool_t bADXL345_writeData(uint8 address, uint8 *data, uint8 len);
// 値判定：加速度単位
PRIVATE int16 i16ADXL345_getAccelerationUnit(uint8 u8DataFmt);
// オフセット値への変換処理
PRIVATE int8 i8ADXL345_convOffsetVal(int32 i32Val);
// インターバルの取得処理
PRIVATE uint32 u32ADXL345_getInterval(uint8 u8Rate);

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/*****************************************************************************
 *
 * NAME: bADXL345_deviceSelect
 *
 * DESCRIPTION:利用するデバイスの選択を行う
 *
 * PARAMETERS:      Name         RW  Usage
 * tsADXL345_Info*  spInfo       R   選択対象デバイス情報
 *
 * RETURNS:
 *      TRUE:選択成功
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bADXL345_deviceSelect(tsADXL345_Info *spInfo) {
	// デバイス情報設定
	spADXL345_Info = spInfo;
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bADXL345_init
 *
 * DESCRIPTION:センサー初期化処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint8          u8Rate          R   サンプリングレート
 *
 * RETURNS:
 *      TRUE :初期化成功
 *      FALSE:初期化失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bADXL345_init(uint8 u8Rate) {
	// 最終アクセス時刻を初期化
	spADXL345_Info->u64LastRefTime = 0;
	// アクセス間隔を初期化
	spADXL345_Info->u32Interval = 5000;
	// 初期状態の読み込み
	if (!bADXL345_readRegister()) {
		return FALSE;
	}
	// デフォルト値の編集
	vADXL345_editDefault();
	// 電力モード（通常）・サンプリングレート更新
	vADXL345_editPower(0, u8Rate & 0x0F);
	// 設定値の書き込み
	return bADXL345_writeRegister();
}

/*****************************************************************************
 *
 * NAME: bADXL345_readRegister
 *
 * DESCRIPTION:レジスタ情報の読み込み
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      TRUE :読み込み成功
 *      FALSE:読み込み失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bADXL345_readRegister() {
	// レジスタ情報の読み込み
	tsADXL345_Register *spRgst = &spADXL345_Info->sReadRgst;
	if (!bADXL345_readData(ADXL345_READ_START, &spRgst->u8TapThresh, ADXL345_READ_LENGTH)) {
		return FALSE;
	}
	// 加速度データ編集
	tsADXL345_AxesData *spReadRgst = &spADXL345_Info->sReadAxesData;
	// 編集用共用体
	tuConvU8ToI16 uConvU8ToI16;
	uConvU8ToI16.u8Axes[0] = spRgst->u8DataX1;
	uConvU8ToI16.u8Axes[1] = spRgst->u8DataX0;
	spReadRgst->i16DataX = uConvU8ToI16.i16Axes;
	uConvU8ToI16.u8Axes[0] = spRgst->u8DataY1;
	uConvU8ToI16.u8Axes[1] = spRgst->u8DataY0;
	spReadRgst->i16DataY = uConvU8ToI16.i16Axes;
	uConvU8ToI16.u8Axes[0] = spRgst->u8DataZ1;
	uConvU8ToI16.u8Axes[1] = spRgst->u8DataZ0;
	spReadRgst->i16DataZ = uConvU8ToI16.i16Axes;
	// 書き込み編集領域に反映
	memcpy(&spADXL345_Info->sWriteSubRgst1, &spRgst->u8TapThresh, sizeof(spADXL345_Info->sWriteSubRgst1));
	memcpy(&spADXL345_Info->sWriteSubRgst2, &spRgst->u8BWRate, sizeof(spADXL345_Info->sWriteSubRgst2));
	spADXL345_Info->u8WriteDataFormat = spRgst->u8DataFormat;
	spADXL345_Info->u8WriteFIFOCtl = spRgst->u8FIFOCtl;
	// 加速度単位編集
	spADXL345_Info->i16WriteGUnit = i16ADXL345_getAccelerationUnit(spADXL345_Info->u8WriteDataFormat);
	// 読み込み成功
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: vADXL345_calibration
 *
 * DESCRIPTION:較正処理、現在の加速度が引数になる様にオフセット値を編集
 *
 * PARAMETERS:       Name            RW  Usage
 *   int8            i8AbsX          R   較正後値（X軸）
 *   int8            i8AbsY          R   較正後値（Y軸）
 *   int8            i8AbsZ          R   較正後値（Z軸）
 *
 * RETURNS:
 *      TRUE :較正成功
 *      FALSE:較正失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bADXL345_calibration(int8 i8AbsX, int8 i8AbsY, int8 i8AbsZ) {
	// オフセットの初期化
	vADXL345_editOffset(0, 0, 0);
	// 電力モード（通常）・サンプリングレート更新（200Hz）
	uint8 u8BefBWRate = spADXL345_Info->sWriteSubRgst2.u8BWRate;
	spADXL345_Info->sWriteSubRgst2.u8BWRate = 0x0B;
	// 書き込みフラグを立てる
	spADXL345_Info->bWriteSubRgst1 = TRUE;
	spADXL345_Info->bWriteSubRgst2 = TRUE;
	// 設定更新
	bADXL345_writeRegister();
	// １０サンプルの平均値を取得
	int32 i32SumX = 0;
	int32 i32SumY = 0;
	int32 i32SumZ = 0;
	tsADXL345_AxesData *spAxesData = &spADXL345_Info->sReadAxesData;
	uint8 idx;
	for (idx = 0; idx < ADXL345_CALIBRATION_CNT; idx++) {
		// サンプル値の読み込み
		if (!bADXL345_readRegister()) {
			return FALSE;
		}
		i32SumX += spAxesData->i16DataX;
		i32SumY += spAxesData->i16DataY;
		i32SumZ += spAxesData->i16DataZ;
	}
	// オフセット値の更新
	tsADXL345_SubRegister1 *spSubRegister1 = &spADXL345_Info->sWriteSubRgst1;
	// オフセットX軸（較正値）
	spSubRegister1->i8OffsetX = i8ADXL345_convOffsetVal(i32SumX / ADXL345_CALIBRATION_CNT - i8AbsX);
	// オフセットY軸（較正値）
	spSubRegister1->i8OffsetY = i8ADXL345_convOffsetVal(i32SumY / ADXL345_CALIBRATION_CNT - i8AbsY);
	// オフセットZ軸（較正値）
	spSubRegister1->i8OffsetZ = i8ADXL345_convOffsetVal(i32SumZ / ADXL345_CALIBRATION_CNT - i8AbsZ);
	// 電力モード・サンプリングレート更新を元に戻す
	spADXL345_Info->sWriteSubRgst2.u8BWRate = u8BefBWRate;
	// 書き込みフラグを立てる
	spADXL345_Info->bWriteSubRgst1 = TRUE;
	spADXL345_Info->bWriteSubRgst2 = TRUE;
	// オフセット・電力モード・サンプリングレートの更新
	return bADXL345_writeRegister();
}

/*****************************************************************************
 *
 * NAME:tsADXL345_getGData
 *
 * DESCRIPTION:加速度取得（XYZ軸）
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   tsADXL345_AxesData 読み込みした加速度データ構造体（15.6mg/LSB）
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC tsADXL345_AxesData sADXL345_getGData() {
	// 読み込みデータ返却
	return spADXL345_Info->sReadAxesData;
}

/*****************************************************************************
 *
 * NAME:bADXL345_getBitField
 *
 * DESCRIPTION:ビットステータス取得
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint16         u16Pos          R   項目アドレス（ビット数）
 *
 * RETURNS:
 *      TRUE :項目値(1)
 *      FALSE:項目値(0)
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bADXL345_getBitField(uint16 u16Pos) {
	uint8 u8BitPos = 7 - u16Pos % 8;
	return (u8ADXL345_getByteField(u16Pos) >> u8BitPos) & 0x01;
}

/*****************************************************************************
 *
 * NAME:u8ADXL345_getByteField
 *
 * DESCRIPTION:バイトステータス取得
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint16         u16Pos          R   項目アドレス（ビット数）
 *
 * RETURNS:
 *   uint8          選択項目値
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint8 u8ADXL345_getByteField(uint16 u16Pos) {
	uint8 *u8Field = (uint8*)(&spADXL345_Info->sReadRgst + (u16Pos - ADXL345_ADDR_THRESH_TAP) / 8);
	return *u8Field;
}

/*****************************************************************************
 *
 * NAME:sADXL345_getActStatus
 *
 * DESCRIPTION:アクティブイベントステータス取得
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   tsADXL345_AxesStatus アクティブイベントステータス
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC tsADXL345_AxesStatus sADXL345_getActStatus() {
	// 対象データバイト取得
	uint8 u8Data = spADXL345_Info->sReadRgst.u8ActTapSts;
	// ステータス編集
	tsADXL345_AxesStatus sStatus;
	sStatus.bStatusX = (u8Data >> 6) & 0x01;
	sStatus.bStatusY = (u8Data >> 5) & 0x01;
	sStatus.bStatusZ = (u8Data >> 4) & 0x01;
	// 編集値を返却
	return sStatus;
}

/*****************************************************************************
 *
 * NAME:u8ADXL345_getTapStatus
 *
 * DESCRIPTION:タップイベントステータス取得
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   tsADXL345_AxesStatus タップイベントステータス
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC tsADXL345_AxesStatus u8ADXL345_getTapStatus() {
	// 対象データバイト取得
	uint8 u8Data = spADXL345_Info->sReadRgst.u8ActTapSts;
	// ステータス編集
	tsADXL345_AxesStatus sStatus;
	sStatus.bStatusX = (u8Data >> 2) & 0x01;
	sStatus.bStatusY = (u8Data >> 1) & 0x01;
	sStatus.bStatusZ = u8Data & 0x01;
	// 編集値を返却
	return sStatus;
}

/*****************************************************************************
 *
 * NAME:sADXL345_getIntStatus
 *
 * DESCRIPTION:割り込みステータス取得
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 * tsADXL345_InterruptSts  割り込みイベントステータス構造体
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC tsADXL345_InterruptSts sADXL345_getIntStatus() {
	// 対象データバイト取得
	uint8 u8Data = spADXL345_Info->sReadRgst.u8IntSource;
	// ステータス編集
	tsADXL345_InterruptSts sStatus;
	sStatus.bStsDataReady  = u8Data >> 7;
	sStatus.bStsSingleTap  = (u8Data >> 6) & 0x01;
	sStatus.bStsDoubleTap  = (u8Data >> 5) & 0x01;
	sStatus.bStsActivity   = (u8Data >> 4) & 0x01;
	sStatus.bStsInActivity = (u8Data >> 3) & 0x01;
	sStatus.bStsFreeFall   = (u8Data >> 2) & 0x01;
	sStatus.bStsWatermark  = (u8Data >> 1) & 0x01;
	sStatus.bStsOverrun    = u8Data & 0x01;
	// 編集値を返却
	return sStatus;
}

/*****************************************************************************
 *
 * NAME:bADXL345_getFIFOTrigger
 *
 * DESCRIPTION:FIFOトリガーイベントステータス取得
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   bool_t         FIFOトリガーイベントステータス
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bADXL345_getFIFOTrigger() {
	return spADXL345_Info->sReadRgst.u8FIFOStatus >> 7;
}

/*****************************************************************************
 *
 * NAME:u8ADXL345_getFIFOPoolSize
 *
 * DESCRIPTION:FIFOプールサイズ取得
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   uint8          FIFOプールサイズ
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC uint8 u8ADXL345_getFIFOPoolSize() {
	return spADXL345_Info->sReadRgst.u8FIFOStatus & 0x1F;
}

/*****************************************************************************
 *
 * NAME:bADXL345_isDataReady
 *
 * DESCRIPTION:未参照データ有無判定
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      TRUE :未参照データ有り
 *      FALSE:未参照データ無し
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bADXL345_isDataReady() {
	return spADXL345_Info->sReadRgst.u8IntSource >> 7;
}

/*****************************************************************************
 *
 * NAME:bADXL345_isSleep
 *
 * DESCRIPTION:スリープ判定
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      TRUE :スリープ状態
 *      FALSE:ウェイクアップ状態
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bADXL345_isSleep() {
	return (spADXL345_Info->sReadRgst.u8ActTapSts >> 3) & 0x01;
}

/*****************************************************************************
 *
 * NAME:vADXL345_editDefault
 *
 * DESCRIPTION:デフォルト値編集
 *
 * PARAMETERS:       Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editDefault() {
	// 書き込み用レジスタ情報１編集
	tsADXL345_SubRegister1 *spSubRegister1 = &spADXL345_Info->sWriteSubRgst1;
	memset(spSubRegister1, 0, sizeof(*spSubRegister1));
	spSubRegister1->u8TapThresh = 0xFF;			// タップ閾値（16G）
	spSubRegister1->i8OffsetX = 0;				// オフセット値（X軸）
	spSubRegister1->i8OffsetY = 0;				// オフセット値（Y軸）
	spSubRegister1->i8OffsetZ = 0;				// オフセット値（Z軸）
	spSubRegister1->u8ActThresh = 0xFF;			// アクティブ閾値（16G）
	spSubRegister1->u8InactThresh = 0xFF;		// インアクティブ閾値（16G）
	spSubRegister1->u8FFThresh = 0xFF;			// 自由落下加速度閾値（16G）
	spSubRegister1->u8FFTime   = 0xFF;			// 自由落下時間閾値（1275ms）
	// 書き込み用レジスタ情報２編集
	tsADXL345_SubRegister2 *spSubRegister2 = &spADXL345_Info->sWriteSubRgst2;
	memset(spSubRegister2, 0, sizeof(*spSubRegister2));
	spSubRegister2->u8BWRate = 0x0A;			// 省電力・データレートコントロール
	spSubRegister2->u8PowerCrl = 0x08;			// リンク・スリープ無効、測定モード
	spADXL345_Info->u8WriteDataFormat = 0x0B;	// デフォルトフォーマット、±16G、最大分解能モード
	spADXL345_Info->u8WriteFIFOCtl = 0x1F;		// FIFO無効、キューサイズ閾値=31
	// 加速度単位の再編集
	spADXL345_Info->i16WriteGUnit = i16ADXL345_getAccelerationUnit(spADXL345_Info->u8WriteDataFormat);
	// 書き込みフラグを立てる
	spADXL345_Info->bWriteSubRgst1 = TRUE;
	spADXL345_Info->bWriteSubRgst2 = TRUE;
}

/*****************************************************************************
 *
 * NAME:vADXL345_editPowert
 *
 * DESCRIPTION:電力モード・データレート編集
 *
 *   Output(Hz) Bandwidth(Hz) 消費電力(uA) サンプリングレート
 *   0.10       0.05           23          0000
 *   0.20       0.10           23          0001
 *   0.39       0.20           23          0010
 *   0.78       0.39           23          0011
 *   1.56       0.78           34          0100
 *   3.13       1.56           40          0101
 *   6.25       3.13           45          0110
 *   12.5       6.25           50          0111
 *     25       12.5           60          1000
 *     50         25           90          1001
 *    100         50          140          1010
 *    200        100          140          1011
 *    400        200          140          1100
 *    800        400          140          1101
 *   1600        800           90          1110
 *   3200       1600          140          1111
 *
 * PARAMETERS:      Name            RW  Usage
 *   bool_t         bLowPWR         R   省電力モードビット
 *   uint8          u8Rate          R   サンプリングレート
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editPower(bool_t bLowPWR, uint8 u8Rate) {
	uint8 u8BWRate = (bLowPWR << 4) | (u8Rate & 0x0F);
	// 更新判定
	if (spADXL345_Info->sWriteSubRgst2.u8BWRate != u8BWRate) {
		// サンプリングレートを更新
		spADXL345_Info->sWriteSubRgst2.u8BWRate = u8BWRate;
		spADXL345_Info->bWriteSubRgst2 = TRUE;
	}
}

/*****************************************************************************
 *
 * NAME:vADXL345_editOffset
 *
 * DESCRIPTION:オフセット編集
 *
 * PARAMETERS:      Name            RW  Usage
 *   int8           i8ofsX          R   オフセット（X軸：15.6 mg/LSB）
 *   int8           i8ofsY          R   オフセット（Y軸：15.6 mg/LSB）
 *   int8           i8ofsZ          R   オフセット（Z軸：15.6 mg/LSB）
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editOffset(int8 i8ofsX, int8 i8ofsY, int8 i8ofsZ) {
	// 更新判定
	if (spADXL345_Info->sWriteSubRgst1.i8OffsetX != i8ofsX) {
		spADXL345_Info->sWriteSubRgst1.i8OffsetX = i8ofsX;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
	if (spADXL345_Info->sWriteSubRgst1.i8OffsetY != i8ofsY) {
		spADXL345_Info->sWriteSubRgst1.i8OffsetY = i8ofsY;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
	if (spADXL345_Info->sWriteSubRgst1.i8OffsetZ != i8ofsZ) {
		spADXL345_Info->sWriteSubRgst1.i8OffsetZ = i8ofsZ;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
}

/*****************************************************************************
 *
 * NAME:vADXL345_editFormat
 *
 * DESCRIPTION:出力フォーマット編集（Gレンジ、精度、左右寄せ、割り込み）
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint8          u8Range         R   Gレンジ（0～3:2,4,8,16）
 *   bool_t         bFullRes,       R   最大分解能モード（TRUE:最大分解能）
 *   bool_t         bJustify        R   左右寄せ（TRUE:左寄せ）
 *   bool_t         bIntInv         R   割り込み出力（TRUE:アクティブ・ハイ）
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editFormat(uint8 u8Range, bool_t bFullRes, bool_t bJustify, bool_t bIntInv) {
	uint8 u8Fmt = spADXL345_Info->u8WriteDataFormat;
	u8Fmt = u8Fmt | (bIntInv << 5);
	u8Fmt = u8Fmt | (bFullRes << 3);
	u8Fmt = u8Fmt | (bJustify << 2);
	u8Fmt = u8Fmt & (u8Range | 0xFC);
	spADXL345_Info->u8WriteDataFormat = u8Fmt;
	// 加速度単位の再編集
	spADXL345_Info->i16WriteGUnit = i16ADXL345_getAccelerationUnit(u8Fmt);
}

/*****************************************************************************
 *
 * NAME:vADXL345_editOutputCtl
 *
 * DESCRIPTION:出力制御編集（セルフテスト、SPI出力モード）
 *
 * PARAMETERS:      Name            RW  Usage
 *   bool_t         bSelfTest       R   セルフテスト（TRUE:有効）
 *   bool_t         bSPIMode        R   SPIモード（TRUE:3線式）
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editOutputCtl(bool_t bSelfTest, bool_t bSPIMode) {
	uint8 u8Fmt = spADXL345_Info->u8WriteDataFormat;
	u8Fmt = u8Fmt | (bSelfTest << 7);
	u8Fmt = u8Fmt | (bSPIMode << 6);
	spADXL345_Info->u8WriteDataFormat = u8Fmt;
}

/*****************************************************************************
 *
 * NAME:vADXL345_editOutputCtl
 *
 * DESCRIPTION:スリープ編集（オートスリープ、スリープ、スリープ時周波数）
 *
 * PARAMETERS:      Name            RW  Usage
 *   bool_t         bAutoSleep      R   オートスリープ（TRUE:オートスリープ有効）
 *   bool_t         bSleep          R   スリープ（TRUE:スリープモード）
 *   uint8          u8SleepRate     R   スリープ時サンプリングレート（0～3:8,4,2,1）
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editSleep(bool_t bAutoSleep, bool_t bSleep, uint8 u8SleepRate) {
	uint8 u8PowerCrl = spADXL345_Info->sWriteSubRgst2.u8PowerCrl;
	u8PowerCrl = u8PowerCrl | (bAutoSleep << 4);
	u8PowerCrl = u8PowerCrl | (bSleep << 2);
	u8PowerCrl = u8PowerCrl & (u8SleepRate | 0xFC);
	// 更新判定
	if (spADXL345_Info->sWriteSubRgst2.u8PowerCrl != u8PowerCrl) {
		spADXL345_Info->sWriteSubRgst2.u8PowerCrl = u8PowerCrl;
		spADXL345_Info->bWriteSubRgst2 = TRUE;
	}
}

/*****************************************************************************
 *
 * NAME:vADXL345_editOutputCtl
 *
 * DESCRIPTION:計測モード設定（スタンバイ、アクティブ・インアクティブリンク）
 *
 * PARAMETERS:      Name            RW  Usage
 *   bool_t         bMeasure        R   スタンバイ（TRUE:測定、FALSE:スタンバイ）
 *   bool_t         bLink           R   リンク（TRUE:イベント発生を排他的に測定）
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editMeasure(bool_t bMeasure, bool_t bLink) {
	uint8 u8PowerCrl = spADXL345_Info->sWriteSubRgst2.u8PowerCrl;
	u8PowerCrl = u8PowerCrl | (bMeasure << 3);
	u8PowerCrl = u8PowerCrl | (bLink << 5);
	// 更新判定
	if (spADXL345_Info->sWriteSubRgst2.u8PowerCrl != u8PowerCrl) {
		spADXL345_Info->sWriteSubRgst2.u8PowerCrl = u8PowerCrl;
		spADXL345_Info->bWriteSubRgst2 = TRUE;
	}
}

/*****************************************************************************
 *
 * NAME:vADXL345_editFIFO
 *
 * DESCRIPTION:FIFO設定（モード、トリガ出力先、プールサイズ閾値）
 *
 * PARAMETERS:      Name            RW  Usage
 *   bool_t         bMeasure        R   スタンバイ（TRUE:測定、FALSE:スタンバイ）
 *   bool_t         bLink           R   リンク（TRUE:イベント発生を排他的に測定）
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editFIFO(uint8 u8Mode, bool_t bTrigger, uint8 u8SampleCnt) {
	uint8 u8FIFOCrl = 0x00;
	u8FIFOCrl = u8FIFOCrl | (u8Mode << 6);
	u8FIFOCrl = u8FIFOCrl | (bTrigger << 5);
	u8FIFOCrl = u8FIFOCrl | u8SampleCnt;
	spADXL345_Info->u8WriteFIFOCtl = u8FIFOCrl;
}

/*****************************************************************************
 *
 * NAME:vADXL345_editIntEnable
 *
 * DESCRIPTION:有効割り込み編集（タップ・アクティブ・自由落下等）
 *
 * PARAMETERS:             Name     RW  Usage
 * tsADXL345_InterruptSts  sStatus  R   割り込み有効ステータス構造体
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editIntEnable(tsADXL345_InterruptSts sStatus) {
	uint8 u8IntEnable = 0x00;
	u8IntEnable = u8IntEnable | (sStatus.bStsDataReady << 7);
	u8IntEnable = u8IntEnable | (sStatus.bStsSingleTap << 6);
	u8IntEnable = u8IntEnable | (sStatus.bStsDoubleTap << 5);
	u8IntEnable = u8IntEnable | (sStatus.bStsActivity << 4);
	u8IntEnable = u8IntEnable | (sStatus.bStsInActivity << 3);
	u8IntEnable = u8IntEnable | (sStatus.bStsFreeFall << 2);
	u8IntEnable = u8IntEnable | (sStatus.bStsWatermark << 1);
	u8IntEnable = u8IntEnable | sStatus.bStsOverrun;
	// 割り込み判定
	if (spADXL345_Info->sWriteSubRgst2.u8IntEnable != u8IntEnable) {
		spADXL345_Info->sWriteSubRgst2.u8IntEnable = u8IntEnable;
		spADXL345_Info->bWriteSubRgst2 = TRUE;
	}
}

/*****************************************************************************
 *
 * NAME:vADXL345_editIntMap
 *
 * DESCRIPTION:割り込み出力先編集（タップ・アクティブ・自由落下等）
 *
 * PARAMETERS:             Name     RW  Usage
 * tsADXL345_InterruptSts  sStatus  R   割り込み先ステータス構造体
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editIntMap(tsADXL345_InterruptSts sStatus) {
	uint8 u8IntEnable = 0x00;
	u8IntEnable = u8IntEnable | (sStatus.bStsDataReady << 7);
	u8IntEnable = u8IntEnable | (sStatus.bStsSingleTap << 6);
	u8IntEnable = u8IntEnable | (sStatus.bStsDoubleTap << 5);
	u8IntEnable = u8IntEnable | (sStatus.bStsActivity << 4);
	u8IntEnable = u8IntEnable | (sStatus.bStsInActivity << 3);
	u8IntEnable = u8IntEnable | (sStatus.bStsFreeFall << 2);
	u8IntEnable = u8IntEnable | (sStatus.bStsWatermark << 1);
	u8IntEnable = u8IntEnable | sStatus.bStsOverrun;
	// 割り込み判定
	if (spADXL345_Info->sWriteSubRgst2.u8IntMap != u8IntEnable) {
		spADXL345_Info->sWriteSubRgst2.u8IntMap = u8IntEnable;
		spADXL345_Info->bWriteSubRgst2 = TRUE;
	}
}

/*****************************************************************************
 *
 * NAME:vADXL345_editTapThreshold
 *
 * DESCRIPTION:タップ閾値編集（加速度、継続時間）
 *
 * PARAMETERS:     Name         RW  Usage
 *   uint8         u8Threshold  R   タップ加速度閾値（62.5 mg/LSB）
 *   uint8         u8Duration   R   タップ継続時間閾値（625 μs/LSB）
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editTapThreshold(uint8 u8Threshold, uint8 u8Duration) {
	// 更新判定
	if (spADXL345_Info->sWriteSubRgst1.u8TapThresh != u8Threshold) {
		spADXL345_Info->sWriteSubRgst1.u8TapThresh = u8Threshold;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
	if (spADXL345_Info->sWriteSubRgst1.u8TapDuration != u8Duration) {
		spADXL345_Info->sWriteSubRgst1.u8TapDuration = u8Duration;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
}

/*****************************************************************************
 *
 * NAME:vADXL345_editDblTapThreshold
 *
 * DESCRIPTION:ダブルタップ閾値編集（間隔、測定期間）
 *
 * PARAMETERS:     Name         RW  Usage
 *   uint8         u8Latent     R   ダブルタップ間隔閾値（1.25ms/LSB）
 *   uint8         u8Window     R   ダブルタップ測定期間閾値（1.25ms/LSB）
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editDblTapThreshold(uint8 u8Latent, uint8 u8Window) {
	// 更新判定
	if (spADXL345_Info->sWriteSubRgst1.u8TapLatency != u8Latent) {
		spADXL345_Info->sWriteSubRgst1.u8TapLatency = u8Latent;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
	if (spADXL345_Info->sWriteSubRgst1.u8TapWindow != u8Window) {
		spADXL345_Info->sWriteSubRgst1.u8TapWindow = u8Window;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
}

/*****************************************************************************
 *
 * NAME:vADXL345_editTapAxes
 *
 * DESCRIPTION:タップ間のタップ有効無効、タップ有効軸
 *
 * PARAMETERS:           Name         RW  Usage
 * bool_t                bSuppress    R   タップ間のタップ有効無効
 * tsADXL345_AxesStatus  sAxesSts     R   タップ有効軸
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editTapAxes(bool_t bSuppress, tsADXL345_AxesStatus sAxesSts) {
	uint8 u8TapAxes = 0x00;
	u8TapAxes = u8TapAxes | (bSuppress << 3);
	u8TapAxes = u8TapAxes | (sAxesSts.bStatusX << 2);
	u8TapAxes = u8TapAxes | (sAxesSts.bStatusY << 1);
	u8TapAxes = u8TapAxes | sAxesSts.bStatusZ;
	// 更新判定
	if (spADXL345_Info->sWriteSubRgst1.u8TapAxes != u8TapAxes) {
		spADXL345_Info->sWriteSubRgst1.u8TapAxes = u8TapAxes;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
}

/*****************************************************************************
 *
 * NAME:vADXL345_editActiveCtl
 *
 * DESCRIPTION:アクティブ制御編集（加速度、絶対／相対、有効軸）
 *
 * PARAMETERS:           Name          RW  Usage
 * uint8                 u8ActTh       R   アクティブ加速度閾値（62.5 mg/LSB）
 * bool_t                bACDC         R   アクティブACDC（TRUE:AC）
 * tsADXL345_AxesStatus  sAxesSts      R   アクティブ判定有効軸
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editActiveCtl(uint8 u8ActTh, bool_t bACDC, tsADXL345_AxesStatus sAxesSts) {
	// 更新判定（アクティブ加速度閾値）
	if (spADXL345_Info->sWriteSubRgst1.u8ActThresh != u8ActTh) {
		spADXL345_Info->sWriteSubRgst1.u8ActThresh = u8ActTh;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
	// 更新判定（有効軸等）
	uint8 u8ActInactCtl = spADXL345_Info->sWriteSubRgst1.u8ActInactCtl & 0x0F;
	u8ActInactCtl = u8ActInactCtl | (bACDC << 3);
	u8ActInactCtl = u8ActInactCtl | (sAxesSts.bStatusX << 2);
	u8ActInactCtl = u8ActInactCtl | (sAxesSts.bStatusY << 1);
	u8ActInactCtl = u8ActInactCtl | sAxesSts.bStatusZ;
	if (spADXL345_Info->sWriteSubRgst1.u8ActInactCtl != u8ActInactCtl) {
		spADXL345_Info->sWriteSubRgst1.u8ActInactCtl = u8ActInactCtl;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
}

/*****************************************************************************
 *
 * NAME:vADXL345_editInActiveCtl
 *
 * DESCRIPTION:インアクティブ制御編集（加速度、継続時間、絶対／相対、有効軸）
 *
 * PARAMETERS:           Name          RW  Usage
 * uint8                 u8InactTh     R   インアクティブ加速度閾値（62.5 mg/LSB）
 * uint8                 u8InactTime   R   インアクティブ時間閾値（1 sec/LSB）
 * bool_t                bACDC         R   インアクティブACDC
 * tsADXL345_AxesStatus  sAxesSts      R   インアクティブ判定有効軸
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editInActiveCtl(uint8 u8InactTh, uint8 u8InactTime, bool_t bACDC, tsADXL345_AxesStatus sAxesSts) {
	// 更新判定（インアクティブ加速度閾値）
	if (spADXL345_Info->sWriteSubRgst1.u8InactThresh != u8InactTh) {
		spADXL345_Info->sWriteSubRgst1.u8InactThresh = u8InactTh;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
	// 更新判定（インアクティブ継続時間閾値）
	if (spADXL345_Info->sWriteSubRgst1.u8InactTime != u8InactTime) {
		spADXL345_Info->sWriteSubRgst1.u8InactTime = u8InactTime;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
	// 更新判定（有効軸等）
	uint8 u8InactCtl = spADXL345_Info->sWriteSubRgst1.u8ActInactCtl & 0xF0;
	u8InactCtl = u8InactCtl | (bACDC << 3);
	u8InactCtl = u8InactCtl | (sAxesSts.bStatusX << 2);
	u8InactCtl = u8InactCtl | (sAxesSts.bStatusY << 1);
	u8InactCtl = u8InactCtl | sAxesSts.bStatusZ;
	if (spADXL345_Info->sWriteSubRgst1.u8ActInactCtl != u8InactCtl) {
		spADXL345_Info->sWriteSubRgst1.u8ActInactCtl = u8InactCtl;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
}

/*****************************************************************************
 *
 * NAME:vADXL345_editFreeFall
 *
 * DESCRIPTION:自由落下閾値編集（加速度、継続時間）
 *
 * PARAMETERS:     Name          RW  Usage
 *   uint8         u8ThreshFF    R   自由落下加速度閾値（62.5 mg/LSB）
 *   uint8         u8TimeFF      R   自由落下時間閾値（5 ms/LSB）
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC void vADXL345_editFreeFall(uint8 u8FFThresh, uint8 u8FFTime) {
	// 更新判定（自由落下加速度閾値）
	if (spADXL345_Info->sWriteSubRgst1.u8FFThresh != u8FFThresh) {
		spADXL345_Info->sWriteSubRgst1.u8FFThresh = u8FFThresh;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
	// 更新判定（自由落下継続時間閾値）
	if (spADXL345_Info->sWriteSubRgst1.u8FFTime != u8FFTime) {
		spADXL345_Info->sWriteSubRgst1.u8FFTime = u8FFTime;
		spADXL345_Info->bWriteSubRgst1 = TRUE;
	}
}

/*****************************************************************************
 *
 * NAME:bADXL345_writeRegister
 *
 * DESCRIPTION:編集値書き込み
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *      TRUE :書き込み成功
 *      FALSE:書き込み失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC bool_t bADXL345_writeRegister() {
	// レジスタ情報１の書き込み判定
	if (spADXL345_Info->bWriteSubRgst1) {
		tsADXL345_SubRegister1 *sSubRgst1 = &spADXL345_Info->sWriteSubRgst1;
		if (!bADXL345_writeData(ADXL345_ADDR_THRESH_TAP / 8, (uint8*)&sSubRgst1->u8TapThresh, sizeof(*sSubRgst1))) {
			return FALSE;
		}
		// 未編集に戻す
		spADXL345_Info->bWriteSubRgst1 = FALSE;
	}
	// レジスタ情報２の書き込み判定
	if (spADXL345_Info->bWriteSubRgst2) {
		tsADXL345_SubRegister2 *sSubRgst2 = &spADXL345_Info->sWriteSubRgst2;
		if (!bADXL345_writeData(ADXL345_ADDR_BW_RATE / 8, (uint8*)&sSubRgst2->u8BWRate, sizeof(*sSubRgst2))) {
			return FALSE;
		}
		// アクセス間隔を初期化
		spADXL345_Info->u32Interval = u32ADXL345_getInterval(sSubRgst2->u8BWRate);
		// 未編集に戻す
		spADXL345_Info->bWriteSubRgst2 = FALSE;
	}
	// データフォーマットの書き込み判定
	if (spADXL345_Info->sReadRgst.u8DataFormat != spADXL345_Info->u8WriteDataFormat) {
		if (!bADXL345_writeData(ADXL345_ADDR_DATA_FORMAT / 8, (uint8*)&spADXL345_Info->u8WriteDataFormat, 1)) {
			return FALSE;
		}
	}
	// FIFOコントロールの書き込み判定
	if (spADXL345_Info->sReadRgst.u8FIFOCtl != spADXL345_Info->u8WriteFIFOCtl) {
		if (!bADXL345_writeData(ADXL345_ADDR_FIFO_CTL / 8, (uint8*)&spADXL345_Info->u8WriteFIFOCtl, 1)) {
			return FALSE;
		}
	}
	// 書き込み成功
	return TRUE;
}


/*****************************************************************************
 *
 * NAME:i16ADXL345_convGVal
 *
 * DESCRIPTION:
 *   加速度の算出処理、これだけの為に浮動小数点演算するのは煩雑なので
 *   整数値計算で概算
 *
 * PARAMETERS:           Name            RW  Usage
 * tsADXL345_AxesData    sAxesData       R
 *
 * RETURNS:
 *      int16       概算した３軸の加速度の合成値
 *
 * NOTES:
 * None.
 *****************************************************************************/
PUBLIC int16 i16ADXL345_convGVal(tsADXL345_AxesData *sAxesData) {
	// 加速度の二乗値（三軸の加速度を合成）を取得
	int32 i32Gx = sAxesData->i16DataX;
	int32 i32Gy = sAxesData->i16DataY;
	int32 i32Gz = sAxesData->i16DataZ;
	uint32 u32Gpow = i32Gx * i32Gx + i32Gy * i32Gy + i32Gz * i32Gz;
	// 概算値（加速度 = √u32Gpow）を算出
	uint32 u32Result = u32Gpow;
	uint32 u32Before = 0;
	while (u32Result != u32Before) {
		u32Before = u32Result;
		u32Result = (u32Before + u32Gpow / u32Before) / 2;
	}
	// 概算値を返す
	return (int16)u32Result;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/*****************************************************************************
 *
 * NAME: vADXL345_wait
 *
 * DESCRIPTION:アクセス待ち処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE void vADXL345_wait() {
	// 読み込み間隔制御
	u32TimerUtil_waitUntil(spADXL345_Info->u64LastRefTime + spADXL345_Info->u32Interval);
}

/*****************************************************************************
 *
 * NAME: bADXL345_readData
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
PRIVATE bool_t bADXL345_readData(uint8 address, uint8 *data, uint8 len) {
	// アクセス待ち処理
	vADXL345_wait();
	// 書き込み開始宣言
	if (!bI2C_startWrite(spADXL345_Info->u8DeviceAddress)) {
		bI2C_stopACK();
		return FALSE;
	}
	// 参照開始アドレス書き込み
	if (u8I2C_write(address) != I2CUTIL_STS_ACK) {
		bI2C_stopACK();
		return FALSE;
	}
	// 読み込み開始宣言
	if(!bI2C_startRead(spADXL345_Info->u8DeviceAddress)) {
		bI2C_stopNACK();
		return FALSE;
	}
	// データの読み込み処理
	if (!bI2C_read(data, len, FALSE)) {
		bI2C_stopNACK();
		return FALSE;
	}
	// I2C通信完了
	if (!bI2C_stopNACK()) {
		return FALSE;
	}
	// 最終アクセス時刻更新
	spADXL345_Info->u64LastRefTime = u64TimerUtil_readUsec();
	// 読み込み完了
	return TRUE;
}

/*****************************************************************************
 *
 * NAME: bADXL345_writeData
 *
 * DESCRIPTION:デバイスへのデータの書き込み
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       address         R   デバイス上のメモリアドレス
 *      uint8*      data            R   書き込みデータ配列
 *      uint8       len             R   書き込みバイト数
 *
 * RETURNS:
 *      TRUE :書き込み成功
 *      FALSE:書き込み失敗
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE bool_t bADXL345_writeData(uint8 address, uint8 *data, uint8 len) {
	// アクセス待ち処理
	vADXL345_wait();
	// 入力チェック
	if (len <= 0) return TRUE;
	// 書き込み開始宣言
	if (!bI2C_startWrite(spADXL345_Info->u8DeviceAddress)) {
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
	// 最終アクセス時刻更新
	spADXL345_Info->u64LastRefTime = u64TimerUtil_readUsec();
	return TRUE;
}

/*****************************************************************************
 *
 * NAME:iADXL345_getAccelerationUnit
 *
 * DESCRIPTION:加速度単位の取得処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   uint8          u8DataFmt       R   データフォーマット
 *
 * RETURNS:
 *   int            加速度単位
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE int16 i16ADXL345_getAccelerationUnit(uint8 u8DataFmt) {
	// 最大分解能判定
	if ((u8DataFmt & 0x08) != 0) {
		return ADXL345_UNIT_FULL;
	}
	// レンジ判定
	switch (u8DataFmt & 0x03) {
	case 0:
		return ADXL345_UNIT_2G;
	case 1:
		return ADXL345_UNIT_4G;
	case 2:
		return ADXL345_UNIT_8G;
	default:
		return ADXL345_UNIT_16G;
	}
}

/*****************************************************************************
 *
 * NAME:i8ADXL345_convOffsetVal
 *
 * DESCRIPTION:オフセット値への変換処理
 *
 * PARAMETERS:      Name            RW  Usage
 *   int32          i32GVal         R   軸毎の加速度
 *
 * RETURNS:
 *   int32          オフセット値に変換された値
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE int8 i8ADXL345_convOffsetVal(int32 i32GVal) {
	int32 i32GUnit = spADXL345_Info->i16WriteGUnit;
	return (i32GVal * i32GUnit * -1) / ADXL345_UNIT_OFFSET;
}


/*****************************************************************************
 *
 * NAME:u32ADXL345_getInterval
 *
 * DESCRIPTION:インターバルの取得処理
 *
 * PARAMETERS:      Name           RW  Usage
 *   uint8          u8Rate         R   サンプリングレート
 *
 * RETURNS:
 *   int32          インターバルタイム（usec単位）
 *
 * NOTES:
 * None.
 *****************************************************************************/
PRIVATE uint32 u32ADXL345_getInterval(uint8 u8Rate) {
	uint32 u32IntervalList[] = {
		10000000, 5000000, 2564102, 1282051, 641025, 320512, 160000, 80000,
		40000, 20000, 10000, 5000, 2500, 1250, 625, 313
	};
	return u32IntervalList[u8Rate & 0x0f];
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
