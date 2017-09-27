/****************************************************************************
 *
 * MODULE :ADXL345 Driver functions header file
 *
 * CREATED:2016/05/01 04:01:00
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
#ifndef  ADXL345_H_INCLUDED
#define  ADXL345_H_INCLUDED

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
#define I2C_ADDR_ADXL345_H     (0x1D)
#define I2C_ADDR_ADXL345_L     (0x53)

// レジスタアドレス
#define ADXL345_ADDR_DEVICE_ID         (0)
#define ADXL345_ADDR_THRESH_TAP        (29 * 8)
#define ADXL345_ADDR_OFSX              (30 * 8)
#define ADXL345_ADDR_OFSY              (31 * 8)
#define ADXL345_ADDR_OFSZ              (32 * 8)
#define ADXL345_ADDR_DUR               (33 * 8)
#define ADXL345_ADDR_LATENT            (34 * 8)
#define ADXL345_ADDR_WINDOW            (35 * 8)
#define ADXL345_ADDR_THRESH_ACT        (36 * 8)
#define ADXL345_ADDR_THRESH_INACT      (37 * 8)
#define ADXL345_ADDR_TIME_INACT        (38 * 8)
#define ADXL345_ADDR_ACT_ACDC          (39 * 8 + 0)
#define ADXL345_ADDR_ACT_ENBL_X        (39 * 8 + 1)
#define ADXL345_ADDR_ACT_ENBL_Y        (39 * 8 + 2)
#define ADXL345_ADDR_ACT_ENBL_Z        (39 * 8 + 3)
#define ADXL345_ADDR_INACT_ACDC        (39 * 8 + 4)
#define ADXL345_ADDR_INACT_ENBL_X      (39 * 8 + 5)
#define ADXL345_ADDR_INACT_ENBL_Y      (39 * 8 + 6)
#define ADXL345_ADDR_INACT_ENBL_Z      (39 * 8 + 7)
#define ADXL345_ADDR_THRESH_FF         (40 * 8)
#define ADXL345_ADDR_TIME_FF           (41 * 8)
#define ADXL345_ADDR_TAP_AXES          (42 * 8)
#define ADXL345_ADDR_TAP_SUPPRES       (42 * 8 + 4)
#define ADXL345_ADDR_TAP_ENBL_X        (42 * 8 + 5)
#define ADXL345_ADDR_TAP_ENBL_Y        (42 * 8 + 6)
#define ADXL345_ADDR_TAP_ENBL_Z        (42 * 8 + 7)
#define ADXL345_ADDR_ACT_TAP_STATUS    (43 * 8)
#define ADXL345_ADDR_ACT_SOURCE_X      (43 * 8 + 1)
#define ADXL345_ADDR_ACT_SOURCE_Y      (43 * 8 + 2)
#define ADXL345_ADDR_ACT_SOURCE_Z      (43 * 8 + 3)
#define ADXL345_ADDR_ASLEEP            (43 * 8 + 4)
#define ADXL345_ADDR_TAP_SOURCE_X      (43 * 8 + 5)
#define ADXL345_ADDR_TAP_SOURCE_Y      (43 * 8 + 6)
#define ADXL345_ADDR_TAP_SOURCE_Z      (43 * 8 + 7)
#define ADXL345_ADDR_BW_RATE           (44 * 8)
#define ADXL345_ADDR_LOW_POWER         (44 * 8 + 3)
#define ADXL345_ADDR_RATE              (44 * 8 + 4)
#define ADXL345_ADDR_POWER_CTL         (45 * 8)
#define ADXL345_ADDR_LINK              (45 * 8 + 3)
#define ADXL345_ADDR_AUTO_SLEEP        (45 * 8 + 4)
#define ADXL345_ADDR_MEASURE           (45 * 8 + 5)
#define ADXL345_ADDR_SLEEP             (45 * 8 + 6)
#define ADXL345_ADDR_WAKEUP            (45 * 8 + 7)
#define ADXL345_ADDR_INT_ENABLE        (46 * 8)
#define ADXL345_ADDR_EBL_DT_READY      (46 * 8 + 0)
#define ADXL345_ADDR_EBL_SGL_TAP       (46 * 8 + 1)
#define ADXL345_ADDR_EBL_DBL_TAP       (46 * 8 + 2)
#define ADXL345_ADDR_EBL_ACT           (46 * 8 + 3)
#define ADXL345_ADDR_EBL_INACT         (46 * 8 + 4)
#define ADXL345_ADDR_EBL_FF            (46 * 8 + 5)
#define ADXL345_ADDR_EBL_WATERMARK     (46 * 8 + 6)
#define ADXL345_ADDR_EBL_OVER_RUN      (46 * 8 + 7)
#define ADXL345_ADDR_INT_MAP           (47 * 8)
#define ADXL345_ADDR_MAP_DT_READY      (47 * 8 + 0)
#define ADXL345_ADDR_MAP_SGL_TAP       (47 * 8 + 1)
#define ADXL345_ADDR_MAP_DBL_TAP       (47 * 8 + 2)
#define ADXL345_ADDR_MAP_ACT           (47 * 8 + 3)
#define ADXL345_ADDR_MAP_INACT         (47 * 8 + 4)
#define ADXL345_ADDR_MAP_FF            (47 * 8 + 5)
#define ADXL345_ADDR_MAP_WATERMARK     (47 * 8 + 6)
#define ADXL345_ADDR_MAP_OVER_RUN      (47 * 8 + 7)
#define ADXL345_ADDR_INT_SOURCE        (48 * 8)
#define ADXL345_ADDR_SRC_DT_READY      (48 * 8 + 0)
#define ADXL345_ADDR_SRC_SGL_TAP       (48 * 8 + 1)
#define ADXL345_ADDR_SRC_DBL_TAP       (48 * 8 + 2)
#define ADXL345_ADDR_SRC_ACT           (48 * 8 + 3)
#define ADXL345_ADDR_SRC_INACT         (48 * 8 + 4)
#define ADXL345_ADDR_SRC_FF            (48 * 8 + 5)
#define ADXL345_ADDR_SRC_WATERMARK     (48 * 8 + 6)
#define ADXL345_ADDR_SRC_OVER_RUN      (48 * 8 + 7)
#define ADXL345_ADDR_DATA_FORMAT       (49 * 8)
#define ADXL345_ADDR_SELF_TEST         (49 * 8 + 0)
#define ADXL345_ADDR_FMT_SPI           (49 * 8 + 1)
#define ADXL345_ADDR_FMT_INT_IVT       (49 * 8 + 2)
#define ADXL345_ADDR_FMT_FULL_RES      (49 * 8 + 4)
#define ADXL345_ADDR_FMT_JUSTIFY       (49 * 8 + 5)
#define ADXL345_ADDR_FMT_RENGE         (49 * 8 + 6)
#define ADXL345_ADDR_DATA_X            (50 * 8)
#define ADXL345_ADDR_DATA_Y            (52 * 8)
#define ADXL345_ADDR_DATA_Z            (54 * 8)
#define ADXL345_ADDR_FIFO_CTL          (56 * 8)
#define ADXL345_ADDR_FIFO_CTL_MODE     (56 * 8 + 0)
#define ADXL345_ADDR_FIFO_CTL_TRG      (56 * 8 + 2)
#define ADXL345_ADDR_FIFO_CTL_SMPL     (56 * 8 + 3)
#define ADXL345_ADDR_FIFO_STATUS       (57 * 8)
#define ADXL345_ADDR_FIFO_STS_TRG      (57 * 8 + 0)
#define ADXL345_ADDR_FIFO_STS_ENTS     (57 * 8 + 2)

// 各加速度係数
#define ADXL345_UNIT_OFFSET            (15625)	// オフセット単位（16000000ug/1024）
#define ADXL345_UNIT_2G                (4000)	// 2G加速度単位（4mg*1000）
#define ADXL345_UNIT_4G                (8000)	// 4G加速度単位（8mg*1000）
#define ADXL345_UNIT_8G                (16000)	// 8G加速度単位（16mg*1000）
#define ADXL345_UNIT_16G               (32000)	// 16G加速度単位（32mg*1000）
#define ADXL345_UNIT_FULL              (4000)	// 高分解能モード加速度単位（4000*1000）

// FIFOモード
#define ADXL345_MODE_FIFO_BYPASS       (0x00)	// FIFOモード：バイパス
#define ADXL345_MODE_FIFO_FIFO         (0x01)	// FIFOモード：FIFO
#define ADXL345_MODE_FIFO_STREAM       (0x02)	// FIFOモード：ストリーム
#define ADXL345_MODE_FIFO_TRIGGER      (0x03)	// FIFOモード：トリガー


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/**
 * ADXL345のレジスタ情報
 */
typedef struct {
	uint8 u8TapThresh;		// タップ閾値（加速度）
	int8  i8OffsetX;		// オフセット（X）
	int8  i8OffsetY;		// オフセット（Y）
	int8  i8OffsetZ;		// オフセット（Z）
	uint8 u8TapDuration;	// タップ閾値（時間）
	uint8 u8TapLatency;		// ダブルタップ閾値（最小間隔）
	uint8 u8TapWindow;		// ダブルタップ閾値（最大間隔）
	uint8 u8ActThresh;		// アクティブ閾値（加速度）
	uint8 u8InactThresh;	// インアクティブ閾値（加速度）
	uint8 u8InactTime;		// インアクティブ閾値（時間）：0-255秒
	uint8 u8ActInactCtl;	// アクティブ・インアクティブ軸
	uint8 u8FFThresh;		// 自由落下閾値（加速度）
	uint8 u8FFTime;			// 自由落下閾値（時間）
	uint8 u8TapAxes;		// タップ軸（シングルタップ・ダブルタップ）
	uint8 u8ActTapSts;		// アクティブタップステータス
	uint8 u8BWRate;			// 省電力・データレートコントロール
	uint8 u8PowerCrl;		// アクティブ・インアクティブ連携・スリープモード等
	uint8 u8IntEnable;		// イベント割り込みの有効無効
	uint8 u8IntMap;			// 各イベント発生時のIntピンマップ
	uint8 u8IntSource;		// イベント発生のステータス
	uint8 u8DataFormat;		// SPIやI2C等の接続方式を指定
	uint8 u8DataX0;			// X軸加速度下位桁（FIFOモード時は一番古いデータ）
	uint8 u8DataX1;			// X軸加速度上位桁（FIFOモード時は一番古いデータ）
	uint8 u8DataY0;			// Y軸加速度下位桁（FIFOモード時は一番古いデータ）
	uint8 u8DataY1;			// Y軸加速度上位桁（FIFOモード時は一番古いデータ）
	uint8 u8DataZ0;			// Z軸加速度下位桁（FIFOモード時は一番古いデータ）
	uint8 u8DataZ1;			// Z軸加速度上位桁（FIFOモード時は一番古いデータ）
	uint8 u8FIFOCtl;		// FIFOモード設定や各イベントに関するデータ数
	uint8 u8FIFOStatus;		// FIFOトリガ・キューサイズ
} tsADXL345_Register;

/**
 * ADXL345のレジスタ情報（書き込み用）
 */
typedef struct {
	uint8 u8TapThresh;		// タップ閾値（加速度）
	int8  i8OffsetX;		// オフセット（X）
	int8  i8OffsetY;		// オフセット（Y）
	int8  i8OffsetZ;		// オフセット（Z）
	uint8 u8TapDuration;	// タップ閾値（時間）
	uint8 u8TapLatency;		// ダブルタップ閾値（最小間隔）
	uint8 u8TapWindow;		// ダブルタップ閾値（最大間隔）
	uint8 u8ActThresh;		// アクティブ閾値（加速度）
	uint8 u8InactThresh;	// インアクティブ閾値（加速度）
	uint8 u8InactTime;		// インアクティブ閾値（時間）：0-255秒
	uint8 u8ActInactCtl;	// アクティブ・インアクティブ軸
	uint8 u8FFThresh;		// 自由落下閾値（加速度）
	uint8 u8FFTime;			// 自由落下閾値（時間）
	uint8 u8TapAxes;		// タップ軸（シングルタップ・ダブルタップ）
} tsADXL345_SubRegister1;

/**
 * ADXL345のレジスタ情報（書き込み用）
 */
typedef struct {
	uint8 u8BWRate;			// 省電力・データレートコントロール
	uint8 u8PowerCrl;		// アクティブ・インアクティブ連携・スリープモード等
	uint8 u8IntEnable;		// イベント割り込みの有効無効
	uint8 u8IntMap;			// 各イベント発生時のIntピンマップ
} tsADXL345_SubRegister2;

/** ３軸データ */
typedef struct {
	int16 i16DataX;				// データ（X軸）
	int16 i16DataY;				// データ（Y軸）
	int16 i16DataZ;				// データ（Z軸）
} tsADXL345_AxesData;

/** ３軸ステータス */
typedef struct {
	bool_t bStatusX;			// ステータス（X軸）
	bool_t bStatusY;			// ステータス（Y軸）
	bool_t bStatusZ;			// ステータス（Z軸）
} tsADXL345_AxesStatus;

/** 割り込みステータス */
typedef struct {
	bool_t bStsDataReady;		// ステータス（データ読み込み）
	bool_t bStsSingleTap;		// ステータス（シングルタップ）
	bool_t bStsDoubleTap;		// ステータス（ダブルタップ）
	bool_t bStsActivity;		// ステータス（アクティブ）
	bool_t bStsInActivity;		// ステータス（インアクティブ）
	bool_t bStsFreeFall;		// ステータス（自由落下）
	bool_t bStsWatermark;		// ステータス（FIFOプールサイズ閾値超え）
	bool_t bStsOverrun;			// ステータス（オーバーラン）
} tsADXL345_InterruptSts;

/**
 * ADXL345デバイス情報
 */
typedef struct {
	uint8 u8DeviceAddress;						// I2Cアドレス
	uint64 u64LastRefTime;						// 最終読み込み時間（マイクロ秒）
	uint32 u32Interval;							// 読み込み間隔
	tsADXL345_Register sReadRgst;				// 読み込みレジスタ情報
	tsADXL345_AxesData sReadAxesData;			// 読み込み加速度情報
	tsADXL345_SubRegister1 sWriteSubRgst1;		// 書き込みレジスタ情報１
	tsADXL345_SubRegister2 sWriteSubRgst2;		// 書き込みレジスタ情報２
	uint8 u8WriteDataFormat;					// 書き込みレジスタ情報（データフォーマット）
	uint8 u8WriteFIFOCtl;						// 書き込みレジスタ情報（FIFOコントロール）
	bool_t bWriteSubRgst1;						// レジスタ情報１書き込みフラグ
	bool_t bWriteSubRgst2;						// レジスタ情報２書き込みフラグ
	int16 i16WriteGUnit;						// 編集値の加速度単位
} tsADXL345_Info;

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
PUBLIC bool_t bADXL345_deviceSelect(tsADXL345_Info *spInfo);
/** センサー初期化処理 */
PUBLIC bool_t bADXL345_init(uint8 u8Rate);
/** レジスタ情報読み込み */
PUBLIC bool_t bADXL345_readRegister();
/** 較正処理 */
PUBLIC bool_t bADXL345_calibration(int8 i8AbsX, int8 i8AbsY, int8 i8AbsZ);
/** 加速度取得（XYZ軸） */
PUBLIC tsADXL345_AxesData sADXL345_getGData();
/** ビットステータス取得 */
PUBLIC bool_t tsADXL345_getBitField(uint16 u16Pos);
/** バイトステータス取得 */
PUBLIC uint8 u8ADXL345_getByteField(uint16 u16Pos);
/** アクティブイベントステータス取得 */
PUBLIC tsADXL345_AxesStatus sADXL345_getActStatus();
/** タップベントステータス取得 */
PUBLIC tsADXL345_AxesStatus sADXL345_getTapStatus();
/** 割り込みステータス取得 */
PUBLIC tsADXL345_InterruptSts sADXL345_getIntStatus();
/** FIFOトリガーステータス取得 */
PUBLIC bool_t bADXL345_getFIFOTrigger();
/** FIFOプールサイズ取得 */
PUBLIC uint8 u8ADXL345_getFIFOPoolSize();
/** 未参照データ有無判定 */
PUBLIC bool_t bADXL345_isDataReady();
/** スリープ判定 */
PUBLIC bool_t bADXL345_isSleep();
/** デフォルト値編集 */
PUBLIC void vADXL345_editDefault();
/** 電力モード・データレート編集 */
PUBLIC void vADXL345_editPower(bool_t bLowPWR, uint8 u8Rate);
/** オフセット編集 */
PUBLIC void vADXL345_editOffset(int8 i8ofsX, int8 i8ofsY, int8 i8ofsZ);
/** 出力フォーマット編集（Gレンジ、精度、左右寄せ、割り込み） */
PUBLIC void vADXL345_editFormat(uint8 u8Range, bool_t bFullRes, bool_t bJustify, bool_t bIntInv);
/** 出力制御編集（セルフテスト、SPI出力モード） */
PUBLIC void vADXL345_editOutputCtl(bool_t bSelfTest, bool_t bSPIMode);
/** スリープ編集（オートスリープ、スリープ、スリープ時周波数） */
PUBLIC void vADXL345_editSleep(bool_t bAutoSleep, bool_t bSleep, uint8 u8SleepRate);
/** 計測モード編集（スタンバイ、アクティブ・インアクティブリンク） */
PUBLIC void vADXL345_editMeasure(bool_t bMeasure, bool_t bLink);
/** FIFO編集（モード、トリガ出力先、プールサイズ閾値） */
PUBLIC void vADXL345_editFIFO(uint8 u8Mode, bool_t bTrigger, uint8 u8SampleCnt);
/** 有効割り込み編集（タップ・アクティブ・自由落下等） */
PUBLIC void vADXL345_editIntEnable(tsADXL345_InterruptSts sStatus);
/** 割り込み出力先編集（タップ・アクティブ・自由落下等） */
PUBLIC void vADXL345_editIntMap(tsADXL345_InterruptSts sStatus);
/** タップ閾値編集（加速度、継続時間） */
PUBLIC void vADXL345_editTapThreshold(uint8 u8Threshold, uint8 u8Duration);
/** ダブルタップ閾値編集（間隔、測定期間） */
PUBLIC void vADXL345_editDblTapThreshold(uint8 u8Latent, uint8 u8Window);
/** タップ設定編集（タップ間のタップ有効無効、タップ有効軸） */
PUBLIC void vADXL345_editTapAxes(bool_t bSuppress, tsADXL345_AxesStatus sAxesSts);
/** アクティブ制御編集（加速度、絶対／相対、有効軸） */
PUBLIC void vADXL345_editActiveCtl(uint8 u8ActTh, bool_t bACDC, tsADXL345_AxesStatus sAxesSts);
/** インアクティブ制御編集（加速度、継続時間、絶対／相対、有効軸） */
PUBLIC void vADXL345_editInActiveCtl(uint8 u8InactTh, uint8 u8InactTime, bool_t bACDC, tsADXL345_AxesStatus sAxesSts);
/** 自由落下閾値編集（加速度、継続時間） */
PUBLIC void vADXL345_editFreeFall(uint8 u8FFThresh, uint8 u8FFTime);
/** 編集値書き込み */
PUBLIC bool_t bADXL345_writeRegister();
/** 加速度算出処理（ニュートン法で概算したXYZ軸の合成値） */
PUBLIC int16 i16ADXL345_convGVal(tsADXL345_AxesData *sAxesData);


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* ADXL345_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
