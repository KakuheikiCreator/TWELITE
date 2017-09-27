/****************************************************************************
 *
 * MODULE :EEPROM Driver functions header file
 *
 * CREATED:2016/05/27 00:37:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:Microchip EEPROM driver
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
#ifndef  EEPROM_24AAXX_H_INCLUDED
#define  EEPROM_24AAXX_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// I2C Address
#define I2C_ADDR_EEPROM_0     (0x50)
#define I2C_ADDR_EEPROM_1     (0x51)
#define I2C_ADDR_EEPROM_2     (0x52)
#define I2C_ADDR_EEPROM_3     (0x53)
#define I2C_ADDR_EEPROM_4     (0x54)
#define I2C_ADDR_EEPROM_5     (0x55)
#define I2C_ADDR_EEPROM_6     (0x56)
#define I2C_ADDR_EEPROM_7     (0x57)

// EEPROM Address Size
#define EEPROM_ADDR_SIZE_1B    (1)
#define EEPROM_ADDR_SIZE_2B    (1)

// EEPROM Page Size
#define EEPROM_PAGE_SIZE_8B    (8)
#define EEPROM_PAGE_SIZE_16B   (16)
#define EEPROM_PAGE_SIZE_32B   (32)
#define EEPROM_PAGE_SIZE_64B   (64)
#define EEPROM_PAGE_SIZE_128B  (128)

// EEPROM Address Size
#define EEPROM_WRITE_CYCLE     (5000)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/**
 * EEPROM用のステータス情報
 */
typedef struct {
	uint8 u8DevAddress;		// I2Cアドレス
	bool_t b2ByteAddrFlg;	// 2Byteアドレスフラグ
	uint8 u8PageSize;		// ページサイズ
	uint64 u64LastWrite;	// 最終書き込み時刻
} tsEEPROM_status;

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
PUBLIC bool_t bEEPROM_deviceSelect(tsEEPROM_status *spStatus);
/** データ読み込み */
PUBLIC bool_t bEEPROM_readData(uint16 u16Addr, uint16 u16Len, uint8 *pu8Buff);
/** データ書き込み */
PUBLIC bool_t bEEPROM_writeData(uint16 u16Addr, uint16 u16Len, uint8 *pu8Data);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* EEPROM_24AAXX_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
