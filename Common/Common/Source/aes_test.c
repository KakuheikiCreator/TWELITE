/******************************************************************************
 *
 * MODULE :AES Test functions source file
 *
 * CREATED:2018/01/27 22:56:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:AES Test functions (source file)
 *             AES暗号の暗号化と復号化を行う関数群です。
 *             対応するキーサイズは128bit、192bit、256bit
 *             ブロック暗号なので、余白分の対応としてPKCS#7の機能を提供する。
 *             仕様書はNISTのサイトを参照
 *             FIPS-197(http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf)
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 ******************************************************************************
 * Copyright (c) 2018, Nakanohito
 * This software is released under the BSD 2-Clause License.
 * http://opensource.org/licenses/BSD-2-Clause
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "aes_test.h"
#include "framework.h"
#include "aes.h"
#include "timer_util.h"

static void test_128();
static void test_192();
static void test_256();
static void dispBytes(char* pPrefix, uint8* bytes, uint8 u8Len);

PUBLIC void vAES_test() {
	// AES 128bit
	test_128();
	// AES 192bit
	test_192();
	// AES 256bit
	test_256();
}

/**
 * 128bit AES Test
 */
static void test_128() {
	//**********************************************************************
	// Case No.1
	//**********************************************************************
	vfPrintf(&sSerStream, "\n");
	SERIAL_vFlush(sSerStream.u8Device);
	vfPrintf(&sSerStream, "================================================================================\n");
	vfPrintf(&sSerStream, "= No.1\n");
	vfPrintf(&sSerStream, "================================================================================\n");
	SERIAL_vFlush(sSerStream.u8Device);
	// キー
	uint8 u8Key01[] =
		{(uint8)0xbc, (uint8)0x94, (uint8)0x61, (uint8)0x67, (uint8)0xa7, (uint8)0x16, (uint8)0xf7, (uint8)0xf1
	   , (uint8)0xb6, (uint8)0x46, (uint8)0x46, (uint8)0x92, (uint8)0xde, (uint8)0xb5, (uint8)0xfb, (uint8)0x0f};
	dispBytes("key    :", u8Key01, 16);
	// 元パスワードからラウンドキー（拡張キー）を初期化
	tsAES_state sState = vAES_newECBState(AES_KEY_LEN_128, u8Key01);
	// データ
	uint8 u8Data01[16];
	memset(u8Data01, 0x00, 16);
	dispBytes("data   :", u8Data01, 16);
	// ECBモードでブロック（128bit）単位でデータを暗号化
	vAES_encryptPad(&sState, u8Data01, 0);
	dispBytes("crypt  :", u8Data01, 16);
	// 暗号文答え合わせ
	uint8 u8TestResult01[] = {
		(uint8)0x2F, (uint8)0x79, (uint8)0x49, (uint8)0x38, (uint8)0x58, (uint8)0x2C, (uint8)0x00, (uint8)0xD0
	  , (uint8)0xA6, (uint8)0xEB, (uint8)0x52, (uint8)0x8F, (uint8)0x66, (uint8)0x0E, (uint8)0x86, (uint8)0x8B
	};
	if (memcmp(u8TestResult01, u8Data01, 16) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.1 Ciphertext OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.1 Ciphertext NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// ECBモードでブロック（128bit）単位でデータを復号化
	vAES_decrypt(&sState, u8Data01, 16);
	dispBytes("decrypt:", u8Data01, 16);

	//**********************************************************************
	// Case No.2
	//**********************************************************************
	vfPrintf(&sSerStream, "\n");
	SERIAL_vFlush(sSerStream.u8Device);
	vfPrintf(&sSerStream, "================================================================================\n");
	vfPrintf(&sSerStream, "= No.2\n");
	vfPrintf(&sSerStream, "================================================================================\n");
	SERIAL_vFlush(sSerStream.u8Device);
	// キー
	uint8 u8Key02[] =
		{(uint8)0xBC, (uint8)0x94, (uint8)0x61, (uint8)0x67, (uint8)0xA7, (uint8)0x16, (uint8)0xF7, (uint8)0xF1
	   , (uint8)0xB6, (uint8)0x46, (uint8)0x46, (uint8)0x92, (uint8)0xDE, (uint8)0xB5, (uint8)0xFB, (uint8)0x0F};
	dispBytes("key    :", u8Key02, 16);
	// 元パスワードからラウンドキー（拡張キー）を初期化
	sState = vAES_newECBState(AES_KEY_LEN_128, u8Key02);
	// データ
	uint8 u8Data02[] =
		{(uint8)0x6A, (uint8)0x18, (uint8)0x0D, (uint8)0x0E, (uint8)0xF0, (uint8)0x86, (uint8)0x56, (uint8)0x57
	   , (uint8)0x9F, (uint8)0x54, (uint8)0xA9, (uint8)0x96, (uint8)0x93, (uint8)0x8F, (uint8)0x1D, (uint8)0xE2};
	uint8 u8TestData02[32];
	memset(u8TestData02, 0x00, 32);
	memcpy(u8TestData02, u8Data02, 16);
	dispBytes("data   :", u8TestData02, 16);
	// ECBモードでブロック（128bit）単位でデータを暗号化
	vAES_encrypt(&sState, u8TestData02, 16);
	dispBytes("crypt  :", u8TestData02, 16);
	// 暗号文答え合わせ
	uint8 u8TestResult02[] = {
		(uint8)0xE2, (uint8)0xCA, (uint8)0x1B, (uint8)0x92, (uint8)0xBA, (uint8)0x6F, (uint8)0x48, (uint8)0xB7
	  , (uint8)0x0A, (uint8)0xDD, (uint8)0x36, (uint8)0xA1, (uint8)0x84, (uint8)0x6C, (uint8)0x36, (uint8)0xCF
	};
	if (memcmp(u8TestResult02, u8TestData02, 16) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.2 Ciphertext OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.2 Ciphertext NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// ECBモードでブロック（128bit）単位でデータを復号化
	vAES_decrypt(&sState, u8TestData02, 16);
	dispBytes("decrypt:", u8TestData02, 16);
	// 平文答え合わせ
	if (memcmp(u8Data02, u8TestData02, 16) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.2 Plaintext  OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.2 Plaintext  NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}

	//**********************************************************************
	// Case No.3
	//**********************************************************************
	vfPrintf(&sSerStream, "\n");
	SERIAL_vFlush(sSerStream.u8Device);
	vfPrintf(&sSerStream, "================================================================================\n");
	vfPrintf(&sSerStream, "= No.3\n");
	vfPrintf(&sSerStream, "================================================================================\n");
	SERIAL_vFlush(sSerStream.u8Device);
	// キー
	uint8 u8Key03[] =
		{(uint8)0xBC, (uint8)0x94, (uint8)0x61, (uint8)0x67, (uint8)0xA7, (uint8)0x16, (uint8)0xF7, (uint8)0xF1
	   , (uint8)0xB6, (uint8)0x46, (uint8)0x46, (uint8)0x92, (uint8)0xDE, (uint8)0xB5, (uint8)0xFB, (uint8)0x0F};
	dispBytes("key    :", u8Key03, 16);
	// 初期ベクトル
	uint8 u8IV03[] =
		{(uint8)0x70, (uint8)0x55, (uint8)0x28, (uint8)0x42, (uint8)0xC8, (uint8)0xAE, (uint8)0xE2, (uint8)0x8D
	   , (uint8)0x08, (uint8)0xD6, (uint8)0x24, (uint8)0x80, (uint8)0x17, (uint8)0x78, (uint8)0x1A, (uint8)0x03};
	dispBytes("iv     :", u8IV03, 16);
	// 元パスワードからラウンドキー（拡張キー）を初期化
	sState = vAES_newCBCState(AES_KEY_LEN_128, u8Key03, u8IV03);
	// データ
	uint8 u8Data03[] =
		{(uint8)0x85, (uint8)0xD5, (uint8)0xFF, (uint8)0xE3, (uint8)0x7F, (uint8)0x8C, (uint8)0x2E, (uint8)0xDC
	   , (uint8)0x24, (uint8)0x58, (uint8)0xF9, (uint8)0x8A, (uint8)0xEE, (uint8)0x1D, (uint8)0x4A, (uint8)0x05
	   , (uint8)0x94, (uint8)0x9D, (uint8)0xF1, (uint8)0x43, (uint8)0xE9, (uint8)0xAF, (uint8)0xA1, (uint8)0xD5
	   , (uint8)0x25, (uint8)0x3D, (uint8)0x8C, (uint8)0x8F, (uint8)0x72, (uint8)0x09, (uint8)0x68, (uint8)0xD0};
	uint8 u8TestData03[32];
	memcpy(u8TestData03, u8Data03, 32);
	dispBytes("data   :", u8Data03, 32);
	// CBCモードでブロック（16byte）単位でデータを暗号化
	vAES_encrypt(&sState, u8TestData03, 32);
	dispBytes("crypt  :", u8TestData03, 32);
	// 暗号文答え合わせ
	uint8 u8TestResult03[] = {
		(uint8)0xD1, (uint8)0xC5, (uint8)0x08, (uint8)0x1E, (uint8)0xD5, (uint8)0x5F, (uint8)0x07, (uint8)0x8F
	  , (uint8)0x95, (uint8)0xC2, (uint8)0xF7, (uint8)0xF7, (uint8)0x58, (uint8)0x37, (uint8)0x36, (uint8)0x66
	  , (uint8)0xD7, (uint8)0x8D, (uint8)0x22, (uint8)0x85, (uint8)0xD0, (uint8)0x68, (uint8)0xB3, (uint8)0x79
	  , (uint8)0x47, (uint8)0xE7, (uint8)0x29, (uint8)0xA6, (uint8)0x50, (uint8)0xAE, (uint8)0x7D, (uint8)0x84
	};
	if (memcmp(u8TestResult03, u8TestData03, 32) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.3 Ciphertext OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.3 Ciphertext NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// CBCモードでブロック（16byte）単位でデータを復号化
	sState = vAES_newCBCState(AES_KEY_LEN_128, u8Key03, u8IV03);
	vAES_decrypt(&sState, u8TestData03, 32);
	dispBytes("decrypt:", u8TestData03, 32);
	// 平文答え合わせ
	if (memcmp(u8Data03, u8TestData03, 32) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.3 Plaintext  OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.3 Plaintext  NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}

	//**********************************************************************
	// Case No.4
	//**********************************************************************
	vfPrintf(&sSerStream, "\n");
	SERIAL_vFlush(sSerStream.u8Device);
	vfPrintf(&sSerStream, "================================================================================\n");
	vfPrintf(&sSerStream, "= No.4\n");
	vfPrintf(&sSerStream, "================================================================================\n");
	SERIAL_vFlush(sSerStream.u8Device);
	// キー
	uint8 u8Key04[] =
		{(uint8)0xBC, (uint8)0x94, (uint8)0x61, (uint8)0x67, (uint8)0xA7, (uint8)0x16, (uint8)0xF7, (uint8)0xF1
	   , (uint8)0xB6, (uint8)0x46, (uint8)0x46, (uint8)0x92, (uint8)0xDE, (uint8)0xB5, (uint8)0xFB, (uint8)0x0F};
	dispBytes("key    :", u8Key04, 16);
	// 初期ベクトル
	uint8 u8IV04[] =
		{(uint8)0x70, (uint8)0x55, (uint8)0x28, (uint8)0x42, (uint8)0xC8, (uint8)0xAE, (uint8)0xE2, (uint8)0x8D
	   , (uint8)0x08, (uint8)0xD6, (uint8)0x24, (uint8)0x80, (uint8)0x17, (uint8)0x78, (uint8)0x1A, (uint8)0x03};
	dispBytes("iv     :", u8IV04, 16);
	// 元パスワードからラウンドキー（拡張キー）を初期化
	sState = vAES_newCBCState(AES_KEY_LEN_128, u8Key04, u8IV04);
	// データ
	uint8 u8Data04[] =
		{(uint8)0xBB, (uint8)0xC6, (uint8)0xDA, (uint8)0x0C, (uint8)0xD1, (uint8)0xB9, (uint8)0x15, (uint8)0x93};
	uint8 u8TestData04[16];
	memset(u8TestData04, 0x00, 16);
	memcpy(u8TestData04, u8Data04, 8);
	dispBytes("data   :", u8Data04, 8);
	// CBCモードでブロック（128bit）単位でデータを暗号化
	vAES_encryptPad(&sState, u8TestData04, 8);
	dispBytes("crypt  :", u8TestData04, 16);
	// 暗号文答え合わせ
	uint8 u8TestResult04[] = {
		(uint8)0x71, (uint8)0x73, (uint8)0xA9, (uint8)0xB2, (uint8)0x8E, (uint8)0x62, (uint8)0xF6, (uint8)0x61
	  , (uint8)0x90, (uint8)0x63, (uint8)0x78, (uint8)0x58, (uint8)0xB2, (uint8)0x60, (uint8)0x72, (uint8)0x99
	};
	if (memcmp(u8TestResult04, u8TestData04, 16) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.4 Ciphertext OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.4 Ciphertext NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// CBCモードでブロック（128bit）単位でデータを復号化
	sState = vAES_newCBCState(AES_KEY_LEN_128, u8Key04, u8IV04);
	vAES_decrypt(&sState, u8TestData04, 8);
	dispBytes("decrypt:", u8TestData04, 16);
	// 答え合わせ
	if (memcmp(u8Data04, u8TestData04, 8) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.4 Plaintext  OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.4 Plaintext  NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
}

/**
 * 192bit AES Test
 */
static void test_192() {
	//**********************************************************************
	// Case No.5
	//**********************************************************************
	vfPrintf(&sSerStream, "\n");
	SERIAL_vFlush(sSerStream.u8Device);
	vfPrintf(&sSerStream, "================================================================================\n");
	vfPrintf(&sSerStream, "= No.5\n");
	vfPrintf(&sSerStream, "================================================================================\n");
	SERIAL_vFlush(sSerStream.u8Device);
	// キー
	uint8 u8Key05[] = {
	    (uint8)0xC5, (uint8)0xBC, (uint8)0xA9, (uint8)0x11, (uint8)0x27, (uint8)0xC0, (uint8)0x6D, (uint8)0x73
	  , (uint8)0x41, (uint8)0x14, (uint8)0x7C, (uint8)0xE9, (uint8)0x88, (uint8)0x1B, (uint8)0xC6, (uint8)0xCB
	  , (uint8)0x24, (uint8)0xC3, (uint8)0xA0, (uint8)0xDE, (uint8)0x54, (uint8)0x89, (uint8)0x41, (uint8)0xC7
	};
	dispBytes("key    :", u8Key05, 24);
	// 元パスワードからラウンドキー（拡張キー）を初期化
	tsAES_state sState = vAES_newECBState(AES_KEY_LEN_192, u8Key05);
	// データ
	uint8 u8Data05[] = {
		(uint8)0x2C, (uint8)0x7F, (uint8)0x23, (uint8)0x95, (uint8)0x0C, (uint8)0x1B, (uint8)0x80, (uint8)0x00
	  , (uint8)0xD4, (uint8)0xCF, (uint8)0x11, (uint8)0xB0, (uint8)0xC8, (uint8)0x11, (uint8)0xCB, (uint8)0x3C
	  , (uint8)0x60, (uint8)0xA0, (uint8)0xDF, (uint8)0xB7, (uint8)0xB7, (uint8)0xBD, (uint8)0x50, (uint8)0x02
	  , (uint8)0x48, (uint8)0xD7, (uint8)0x29, (uint8)0x81, (uint8)0xD4, (uint8)0x3E, (uint8)0x2B, (uint8)0xE3
	};
	uint8 u8TestData05[48];
	memset(u8TestData05, 0x00, 48);
	memcpy(u8TestData05, u8Data05, 32);
	dispBytes("data   :", u8Data05, 32);
	// ECBモードでブロック（16byte）単位でデータを暗号化
	vAES_encryptPad(&sState, u8TestData05, 32);
	dispBytes("crypt  :", u8TestData05, 48);
	// 暗号文答え合わせ
	uint8 u8TestResult05[] = {
		(uint8)0x5B, (uint8)0x90, (uint8)0x66, (uint8)0x37, (uint8)0x27, (uint8)0x1A, (uint8)0x9D, (uint8)0x56
	  , (uint8)0x17, (uint8)0x44, (uint8)0x34, (uint8)0xBB, (uint8)0xB6, (uint8)0xFD, (uint8)0x6B, (uint8)0x24
	  , (uint8)0x2F, (uint8)0x89, (uint8)0x1B, (uint8)0xA4, (uint8)0x1F, (uint8)0x7A, (uint8)0x4E, (uint8)0xC9
	  , (uint8)0xCA, (uint8)0x77, (uint8)0xFA, (uint8)0xD4, (uint8)0x8D, (uint8)0x47, (uint8)0x06, (uint8)0xFD
	  , (uint8)0x98, (uint8)0x0A, (uint8)0x8B, (uint8)0x77, (uint8)0xBE, (uint8)0xD7, (uint8)0x18, (uint8)0x42
	  , (uint8)0x9F, (uint8)0x53, (uint8)0x9C, (uint8)0x81, (uint8)0xFF, (uint8)0x1C, (uint8)0x2F, (uint8)0x02
	};
	if (memcmp(u8TestResult05, u8TestData05, 48) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.5 Ciphertext OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.5 Ciphertext NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// ECBモードでブロック（16byte）単位でデータを復号化
	vAES_decrypt(&sState, u8TestData05, 32);
	dispBytes("decrypt:", u8TestData05, 32);
	// 平文答え合わせ
	if (memcmp(u8Data05, u8TestData05, 32) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.5 Plaintext  OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.5 Plaintext  NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}

	//**********************************************************************
	// Case No.6
	//**********************************************************************
	vfPrintf(&sSerStream, "\n");
	SERIAL_vFlush(sSerStream.u8Device);
	vfPrintf(&sSerStream, "================================================================================\n");
	vfPrintf(&sSerStream, "= No.6\n");
	vfPrintf(&sSerStream, "================================================================================\n");
	SERIAL_vFlush(sSerStream.u8Device);
	// キー
	uint8 u8Key06[] = {
		(uint8)0xF7, (uint8)0x1F, (uint8)0xCF, (uint8)0x6A, (uint8)0x9A, (uint8)0x4C, (uint8)0x3E, (uint8)0xBE
	  , (uint8)0xB0, (uint8)0x9D, (uint8)0xD3, (uint8)0x96, (uint8)0x88, (uint8)0x5B, (uint8)0x2D, (uint8)0xE1
	  , (uint8)0x34, (uint8)0x9C, (uint8)0x22, (uint8)0x73, (uint8)0x27, (uint8)0x04, (uint8)0x54, (uint8)0x57
	};
	dispBytes("key    :", u8Key06, 24);
	// 初期ベクトル
	uint8 u8IV06[] = {
		(uint8)0x7D, (uint8)0xD1, (uint8)0x5A, (uint8)0x21, (uint8)0x76, (uint8)0x5A, (uint8)0x7B, (uint8)0x74
	  , (uint8)0x1D, (uint8)0x04, (uint8)0x7C, (uint8)0x39, (uint8)0xA3, (uint8)0x62, (uint8)0x9E, (uint8)0xE5
	};
	dispBytes("iv     :", u8IV06, 16);
	// 元パスワードからラウンドキー（拡張キー）を初期化
	sState = vAES_newCBCState(AES_KEY_LEN_192, u8Key06, u8IV06);
	// データ
	uint8 u8Data06[] = {
		(uint8)0x2A, (uint8)0xED, (uint8)0xFC, (uint8)0xA3, (uint8)0x00, (uint8)0x55, (uint8)0xC2, (uint8)0x92
	  , (uint8)0x51, (uint8)0x5D, (uint8)0x9A, (uint8)0xBD, (uint8)0x00, (uint8)0xD1, (uint8)0xD7, (uint8)0xDD
	  , (uint8)0xAB, (uint8)0x76, (uint8)0xA1, (uint8)0xCC, (uint8)0xD4, (uint8)0x12, (uint8)0x5C, (uint8)0xEC
	  , (uint8)0xCB, (uint8)0x62, (uint8)0x13, (uint8)0x2A, (uint8)0xF5, (uint8)0x5C, (uint8)0x99
	};
	uint8 u8TestData06[32];
	memset(u8TestData06, 0x00, 32);
	memcpy(u8TestData06, u8Data06, 31);
	dispBytes("data   :", u8TestData06, 16);
	// CBCモードでブロック（16byte）単位でデータを暗号化
	vAES_encryptPad(&sState, u8TestData06, 31);
	dispBytes("crypt  :", u8TestData06, 32);
	// 暗号文答え合わせ
	uint8 u8TestResult06[] = {
		(uint8)0x59, (uint8)0xFF, (uint8)0x0F, (uint8)0xA6, (uint8)0xD9, (uint8)0xDF, (uint8)0x04, (uint8)0x01
	  , (uint8)0x57, (uint8)0x0C, (uint8)0xFA, (uint8)0x0A, (uint8)0xE6, (uint8)0xB4, (uint8)0x05, (uint8)0xCC
	  , (uint8)0x2A, (uint8)0x27, (uint8)0x89, (uint8)0x73, (uint8)0xF3, (uint8)0x1C, (uint8)0x89, (uint8)0xC9
	  , (uint8)0xDC, (uint8)0x68, (uint8)0x39, (uint8)0xA5, (uint8)0xF7, (uint8)0x3D, (uint8)0x66, (uint8)0xFD
	};
	if (memcmp(u8TestResult06, u8TestData06, 32) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.6 Ciphertext OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.6 Ciphertext NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// ECBモードでブロック（16byte）単位でデータを復号化
	sState = vAES_newCBCState(AES_KEY_LEN_192, u8Key06, u8IV06);
	vAES_decrypt(&sState, u8TestData06, 31);
	dispBytes("decrypt:", u8TestData06, 32);
	// 平文答え合わせ
	if (memcmp(u8Data06, u8TestData06, 31) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.6 Plaintext  OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.6 Plaintext  NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}

	//**********************************************************************
	// Case No.7
	//**********************************************************************
	vfPrintf(&sSerStream, "\n");
	SERIAL_vFlush(sSerStream.u8Device);
	vfPrintf(&sSerStream, "================================================================================\n");
	vfPrintf(&sSerStream, "= No.7\n");
	vfPrintf(&sSerStream, "================================================================================\n");
	SERIAL_vFlush(sSerStream.u8Device);
	// キー
	uint8 u8Key07[] = {
		(uint8)0xA6, (uint8)0x89, (uint8)0xE7, (uint8)0xE3, (uint8)0xDB, (uint8)0xE6, (uint8)0x75, (uint8)0x3D
	  , (uint8)0xFF, (uint8)0x7C, (uint8)0x9A, (uint8)0x70, (uint8)0x79, (uint8)0x4D, (uint8)0x20, (uint8)0x74
	  , (uint8)0x85, (uint8)0xE6, (uint8)0x48, (uint8)0xA1, (uint8)0x48, (uint8)0x94, (uint8)0xEC, (uint8)0x0B
	};
	dispBytes("key    :", u8Key07, 24);
	// 初期ベクトル
	uint8 u8IV07[] = {
		(uint8)0x17, (uint8)0x1B, (uint8)0xD6, (uint8)0xC9, (uint8)0x25, (uint8)0x11, (uint8)0x5F, (uint8)0x18
	  , (uint8)0x40, (uint8)0x47, (uint8)0xC1, (uint8)0x34, (uint8)0xEA, (uint8)0xDA, (uint8)0x1C, (uint8)0x32
	};
	dispBytes("iv     :", u8IV07, 16);
	// 元パスワードからラウンドキー（拡張キー）を初期化
	sState = vAES_newCBCState(AES_KEY_LEN_192, u8Key07, u8IV07);
	// データ
	uint8 u8Data07[] = {
		(uint8)0xB1, (uint8)0xCB, (uint8)0x00, (uint8)0xBC, (uint8)0xAE, (uint8)0x25, (uint8)0xBA, (uint8)0x42
	  , (uint8)0xAA, (uint8)0x82, (uint8)0x9E, (uint8)0xD5, (uint8)0x5C, (uint8)0x10, (uint8)0x5F, (uint8)0xE8
	  , (uint8)0x34, (uint8)0x77, (uint8)0xED, (uint8)0x12, (uint8)0x0D, (uint8)0x1E, (uint8)0x5B, (uint8)0xB6
	  , (uint8)0x6D, (uint8)0x16, (uint8)0xF8, (uint8)0x32, (uint8)0xE5, (uint8)0xF8, (uint8)0x75, (uint8)0xDC
	  , (uint8)0xAA, (uint8)0x45, (uint8)0xE1, (uint8)0xDD, (uint8)0x7B, (uint8)0xEE, (uint8)0x72, (uint8)0x94
	  , (uint8)0xA4, (uint8)0x89, (uint8)0xEF, (uint8)0xDB, (uint8)0xC1, (uint8)0xC7, (uint8)0xBE, (uint8)0xE9
	  , (uint8)0x35
	};
	uint8 u8TestData07[64];
	memset(u8TestData07, 0x00, 64);
	memcpy(u8TestData07, u8Data07, 49);
	dispBytes("data   :", u8Data07, 49);
	// CBCモードでブロック（128bit）単位でデータを暗号化
	vAES_encryptPad(&sState, u8TestData07, 49);
	dispBytes("crypt  :", u8TestData07, 64);
	// 暗号文答え合わせ
	uint8 u8TestResult07[] = {
		(uint8)0x37, (uint8)0xF4, (uint8)0x43, (uint8)0x6B, (uint8)0xEA, (uint8)0x4F, (uint8)0xAD, (uint8)0x17
	  , (uint8)0xFE, (uint8)0x19, (uint8)0x9A, (uint8)0xDB, (uint8)0xFA, (uint8)0xC3, (uint8)0xE2, (uint8)0xBB
	  , (uint8)0x3A, (uint8)0xB6, (uint8)0xFA, (uint8)0xE3, (uint8)0xE4, (uint8)0xF1, (uint8)0x2D, (uint8)0x60
	  , (uint8)0x69, (uint8)0xBF, (uint8)0xDD, (uint8)0x20, (uint8)0x20, (uint8)0x08, (uint8)0x16, (uint8)0xEB
	  , (uint8)0x7F, (uint8)0xDC, (uint8)0xF1, (uint8)0x7F, (uint8)0xA7, (uint8)0xA6, (uint8)0x6F, (uint8)0x23
	  , (uint8)0x30, (uint8)0x92, (uint8)0x51, (uint8)0xAE, (uint8)0xFC, (uint8)0x1D, (uint8)0x5A, (uint8)0x31
	  , (uint8)0x8A, (uint8)0xD1, (uint8)0x49, (uint8)0x3F, (uint8)0x05, (uint8)0x03, (uint8)0x77, (uint8)0xD1
	  , (uint8)0x06, (uint8)0x70, (uint8)0xA2, (uint8)0x59, (uint8)0xD8, (uint8)0x4F, (uint8)0x72, (uint8)0xC3
	};
	if (memcmp(u8TestResult07, u8TestData07, 64) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.7 Ciphertext OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.7 Ciphertext NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// CBCモードでブロック（16byte）単位でデータを復号化
	sState = vAES_newCBCState(AES_KEY_LEN_192, u8Key07, u8IV07);
	vAES_decrypt(&sState, u8TestData07, 64);
	dispBytes("decrypt:", u8TestData07, 64);
	// 平文答え合わせ
	if (memcmp(u8Data07, u8TestData07, 49) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.7 Plaintext  OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.7 Plaintext  NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
}

/**
 * 256bit AES Test
 */
static void test_256() {
	//**********************************************************************
	// Case No.8
	//**********************************************************************
	vfPrintf(&sSerStream, "\n");
	SERIAL_vFlush(sSerStream.u8Device);
	vfPrintf(&sSerStream, "================================================================================\n");
	vfPrintf(&sSerStream, "= No.8\n");
	vfPrintf(&sSerStream, "================================================================================\n");
	SERIAL_vFlush(sSerStream.u8Device);
	// キー
	uint8 u8Key08[] = {
		(uint8)0xC5, (uint8)0xBC, (uint8)0xA9, (uint8)0x11, (uint8)0x27, (uint8)0xC0, (uint8)0x6D, (uint8)0x73
	  , (uint8)0x77, (uint8)0x18, (uint8)0x47, (uint8)0x3D, (uint8)0xB2, (uint8)0x23, (uint8)0x39, (uint8)0xB9
	  , (uint8)0xEE, (uint8)0xA1, (uint8)0xD4, (uint8)0x91, (uint8)0xBE, (uint8)0xAD, (uint8)0x6A, (uint8)0x22
	  , (uint8)0x24, (uint8)0xC3, (uint8)0xA0, (uint8)0xDE, (uint8)0x54, (uint8)0x89, (uint8)0x41, (uint8)0xC7
	};
	dispBytes("key    :", u8Key08, 32);
	// 元パスワードからラウンドキー（拡張キー）を初期化
	tsAES_state sState = vAES_newECBState(AES_KEY_LEN_256, u8Key08);
	// データ
	uint8 u8Data08[] = {
		(uint8)0x2C, (uint8)0x7F, (uint8)0x23, (uint8)0x95, (uint8)0x0C, (uint8)0x1B, (uint8)0x80, (uint8)0x00
	  , (uint8)0xD4, (uint8)0xCF, (uint8)0x11, (uint8)0xB0, (uint8)0xC8, (uint8)0x11, (uint8)0xCB, (uint8)0x3C
	  , (uint8)0x41, (uint8)0x14, (uint8)0x7C, (uint8)0xE9, (uint8)0x88, (uint8)0x1B, (uint8)0xC6, (uint8)0xCB
	  , (uint8)0x7C, (uint8)0xD6, (uint8)0x0A, (uint8)0x26, (uint8)0x14, (uint8)0xDF, (uint8)0x8E, (uint8)0xA2
	  , (uint8)0x60, (uint8)0xA0, (uint8)0xDF, (uint8)0xB7, (uint8)0xB7, (uint8)0xBD, (uint8)0x50, (uint8)0x02
	  , (uint8)0x00, (uint8)0x2A, (uint8)0xA6, (uint8)0x1B, (uint8)0xEB, (uint8)0xE6, (uint8)0x89, (uint8)0x7F
	  , (uint8)0xBA, (uint8)0xDB, (uint8)0x4F, (uint8)0xA2, (uint8)0x9C, (uint8)0x6A, (uint8)0xFF, (uint8)0x9A
	  , (uint8)0x48, (uint8)0xD7, (uint8)0x29, (uint8)0x81, (uint8)0xD4, (uint8)0x3E, (uint8)0x2B, (uint8)0xE3
	};
	uint8 u8TestData08[80];
	memset(u8TestData08, 0x00, 64);
	memcpy(u8TestData08, u8Data08, 64);
	dispBytes("data   :", u8Data08, 64);
	// ECBモードでブロック（16byte）単位でデータを暗号化
	vAES_encryptPad(&sState, u8TestData08, 64);
	dispBytes("crypt  :", u8TestData08, 80);
	// 暗号文答え合わせ
	uint8 u8TestResult08[] = {
		(uint8)0xE7, (uint8)0x99, (uint8)0xEF, (uint8)0x1C, (uint8)0x9F, (uint8)0xD6, (uint8)0x78, (uint8)0x8B
	  , (uint8)0x35, (uint8)0xB7, (uint8)0x55, (uint8)0x1E, (uint8)0xDD, (uint8)0xA0, (uint8)0xB8, (uint8)0x06
	  , (uint8)0x7C, (uint8)0x5F, (uint8)0x7D, (uint8)0x9D, (uint8)0xB4, (uint8)0x02, (uint8)0x0B, (uint8)0x5E
	  , (uint8)0x06, (uint8)0x13, (uint8)0x47, (uint8)0xE2, (uint8)0x59, (uint8)0xD7, (uint8)0x25, (uint8)0xEB
	  , (uint8)0xDA, (uint8)0x5E, (uint8)0x93, (uint8)0x83, (uint8)0xE6, (uint8)0xBA, (uint8)0x0D, (uint8)0x80
	  , (uint8)0x62, (uint8)0x28, (uint8)0x02, (uint8)0x85, (uint8)0x8D, (uint8)0x9B, (uint8)0x4E, (uint8)0x81
	  , (uint8)0x4D, (uint8)0xBC, (uint8)0x73, (uint8)0xB0, (uint8)0x12, (uint8)0x12, (uint8)0xBC, (uint8)0x59
	  , (uint8)0x9B, (uint8)0xDF, (uint8)0xE4, (uint8)0xA6, (uint8)0x4A, (uint8)0x0C, (uint8)0x2C, (uint8)0x7A
	  , (uint8)0x88, (uint8)0x04, (uint8)0xFA, (uint8)0xB6, (uint8)0xC9, (uint8)0xFD, (uint8)0x98, (uint8)0x8A
	  , (uint8)0x85, (uint8)0x32, (uint8)0x59, (uint8)0x44, (uint8)0xD4, (uint8)0x10, (uint8)0xDF, (uint8)0x01
	};
	if (memcmp(u8TestResult08, u8TestData08, 80) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.8 Ciphertext OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.8 Ciphertext NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// ECBモードでブロック（16byte）単位でデータを復号化
	vAES_decrypt(&sState, u8TestData08, 64);
	dispBytes("decrypt:", u8TestData08, 64);
	// 平文答え合わせ
	if (memcmp(u8Data08, u8TestData08, 64) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.8 Plaintext  OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.8 Plaintext  NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}

	//**********************************************************************
	// Case No.9
	//**********************************************************************
	vfPrintf(&sSerStream, "\n");
	SERIAL_vFlush(sSerStream.u8Device);
	vfPrintf(&sSerStream, "================================================================================\n");
	vfPrintf(&sSerStream, "= No.9\n");
	vfPrintf(&sSerStream, "================================================================================\n");
	SERIAL_vFlush(sSerStream.u8Device);
	// キー
	uint8 u8Key09[] = {
		(uint8)0xF7, (uint8)0x1F, (uint8)0xCF, (uint8)0x6A, (uint8)0x9A, (uint8)0x4C, (uint8)0x3E, (uint8)0xBE
	  , (uint8)0x23, (uint8)0x3D, (uint8)0x64, (uint8)0x92, (uint8)0xFE, (uint8)0x79, (uint8)0xF4, (uint8)0xDA
	  , (uint8)0xB0, (uint8)0x9D, (uint8)0xD3, (uint8)0x96, (uint8)0x88, (uint8)0x5B, (uint8)0x2D, (uint8)0xE1
	  , (uint8)0x34, (uint8)0x9C, (uint8)0x22, (uint8)0x73, (uint8)0x27, (uint8)0x04, (uint8)0x54, (uint8)0x57
	};
	dispBytes("key    :", u8Key09, 32);
	// 初期ベクトル
	uint8 u8IV09[] = {
		(uint8)0x7D, (uint8)0xD1, (uint8)0x5A, (uint8)0x21, (uint8)0x76, (uint8)0x5A, (uint8)0x7B, (uint8)0x74
	  , (uint8)0x1D, (uint8)0x04, (uint8)0x7C, (uint8)0x39, (uint8)0xA3, (uint8)0x62, (uint8)0x9E, (uint8)0xE5
	};
	dispBytes("iv     :", u8IV09, 16);
	// 元パスワードからラウンドキー（拡張キー）を初期化
	sState = vAES_newCBCState(AES_KEY_LEN_256, u8Key09, u8IV09);
	// データ
	uint8 u8Data09[] = {};
	uint8 u8TestData09[16];
	memset(u8TestData09, 0x00, 16);
	dispBytes("data   :", u8TestData09, 0);
	// CBCモードでブロック（16byte）単位でデータを暗号化
	vAES_encryptPad(&sState, u8TestData09, 0);
	dispBytes("crypt  :", u8TestData09, 16);
	// 暗号文答え合わせ
	uint8 u8TestResult09[] = {
		(uint8)0x40, (uint8)0x57, (uint8)0x83, (uint8)0x03, (uint8)0x94, (uint8)0x48, (uint8)0xD9, (uint8)0xC0
	  , (uint8)0xEA, (uint8)0x9C, (uint8)0xC3, (uint8)0x07, (uint8)0x69, (uint8)0x0D, (uint8)0x7E, (uint8)0x4D
	};
	if (memcmp(u8TestResult09, u8TestData09, 16) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.9 Ciphertext OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.9 Ciphertext NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// ECBモードでブロック（16byte）単位でデータを復号化
	sState = vAES_newCBCState(AES_KEY_LEN_256, u8Key09, u8IV09);
	vAES_decrypt(&sState, u8TestData09, 0);
	dispBytes("decrypt:", u8TestData09, 16);

	//**********************************************************************
	// Case No.10
	//**********************************************************************
	vfPrintf(&sSerStream, "\n");
	SERIAL_vFlush(sSerStream.u8Device);
	vfPrintf(&sSerStream, "================================================================================\n");
	vfPrintf(&sSerStream, "= No.10\n");
	vfPrintf(&sSerStream, "================================================================================\n");
	SERIAL_vFlush(sSerStream.u8Device);
	// キー
	uint8 u8Key10[] = {
		(uint8)0xA6, (uint8)0x89, (uint8)0xE7, (uint8)0xE3, (uint8)0xDB, (uint8)0xE6, (uint8)0x75, (uint8)0x3D
	  , (uint8)0xFF, (uint8)0x7C, (uint8)0x9A, (uint8)0x70, (uint8)0x79, (uint8)0x4D, (uint8)0x20, (uint8)0x74
	  , (uint8)0x95, (uint8)0xF8, (uint8)0x1A, (uint8)0x75, (uint8)0x2F, (uint8)0xB7, (uint8)0xC5, (uint8)0x9C
	  , (uint8)0x85, (uint8)0xE6, (uint8)0x48, (uint8)0xA1, (uint8)0x48, (uint8)0x94, (uint8)0xEC, (uint8)0x0B
	};
	dispBytes("key    :", u8Key10, 32);
	// 初期ベクトル
	uint8 u8IV10[] = {
		(uint8)0x17, (uint8)0x1B, (uint8)0xD6, (uint8)0xC9, (uint8)0x25, (uint8)0x11, (uint8)0x5F, (uint8)0x18
	  , (uint8)0x40, (uint8)0x47, (uint8)0xC1, (uint8)0x34, (uint8)0xEA, (uint8)0xDA, (uint8)0x1C, (uint8)0x32
	};
	dispBytes("iv     :", u8IV10, 16);
	// 元パスワードからラウンドキー（拡張キー）を初期化
	sState = vAES_newCBCState(AES_KEY_LEN_256, u8Key10, u8IV10);
	// データ
	uint8 u8Data10[] = {
		(uint8)0xB1, (uint8)0xCB, (uint8)0x00, (uint8)0xBC, (uint8)0xAE, (uint8)0x25, (uint8)0xBA, (uint8)0x42
	  , (uint8)0x2D, (uint8)0x08, (uint8)0x07, (uint8)0xAC, (uint8)0xCE, (uint8)0x26, (uint8)0x2D, (uint8)0x24
	  , (uint8)0xAE, (uint8)0x5F, (uint8)0xA6, (uint8)0x87, (uint8)0x9E, (uint8)0x38, (uint8)0x88, (uint8)0xFD
	  , (uint8)0x1B, (uint8)0x2E, (uint8)0xCC, (uint8)0x4D, (uint8)0xCE, (uint8)0x0C, (uint8)0x5C, (uint8)0x80
	  , (uint8)0x52, (uint8)0xB5, (uint8)0xED, (uint8)0xAD, (uint8)0x71, (uint8)0x3A, (uint8)0xA9, (uint8)0x01
	  , (uint8)0xAA, (uint8)0x82, (uint8)0x9E, (uint8)0xD5, (uint8)0x5C, (uint8)0x10, (uint8)0x5F, (uint8)0xE8
	  , (uint8)0x19, (uint8)0xE2, (uint8)0x9A, (uint8)0xE6, (uint8)0xC4, (uint8)0xA0, (uint8)0xB9, (uint8)0x84
	  , (uint8)0xED, (uint8)0x13, (uint8)0xBD, (uint8)0xD9, (uint8)0x29, (uint8)0xFA, (uint8)0xD5, (uint8)0x94
	  , (uint8)0x34, (uint8)0x77, (uint8)0xED, (uint8)0x12, (uint8)0x0D, (uint8)0x1E, (uint8)0x5B, (uint8)0xB6
	  , (uint8)0x06, (uint8)0xE4, (uint8)0x78, (uint8)0x21, (uint8)0xC8, (uint8)0x23, (uint8)0xC3, (uint8)0x64
	  , (uint8)0xBB, (uint8)0x17, (uint8)0xE1, (uint8)0x3F, (uint8)0x24, (uint8)0x26, (uint8)0xA8, (uint8)0x4F
	  , (uint8)0x6D, (uint8)0x16, (uint8)0xF8, (uint8)0x32, (uint8)0xE5, (uint8)0xF8, (uint8)0x75, (uint8)0xDC
	  , (uint8)0x6E, (uint8)0x7D, (uint8)0xF2, (uint8)0xD2, (uint8)0x19, (uint8)0x6D, (uint8)0x4E, (uint8)0xA6
	  , (uint8)0x25, (uint8)0xF8, (uint8)0x72, (uint8)0x47, (uint8)0x56, (uint8)0xE8, (uint8)0x20, (uint8)0x95
	  , (uint8)0xAA, (uint8)0x45, (uint8)0xE1, (uint8)0xDD, (uint8)0x7B, (uint8)0xEE, (uint8)0x72, (uint8)0x94
	  , (uint8)0xA4, (uint8)0x89, (uint8)0xEF, (uint8)0xDB, (uint8)0xC1, (uint8)0xC7, (uint8)0xBE, (uint8)0xE9
	};
	uint8 u8TestData10[144];
	memset(u8TestData10, 0x00, 144);
	memcpy(u8TestData10, u8Data10, 128);
	dispBytes("data   :", u8Data10, 128);
	// CBCモードでブロック（16byte）単位でデータを暗号化
	uint32 u32EncBefore = u64TimerUtil_readUsec();
	vAES_encryptPad(&sState, u8TestData10, 128);
	uint32 u32EncAfter = u64TimerUtil_readUsec();
	vfPrintf(&sSerStream, "\nMS:%08d No.10 vAES_encryptPad %d usec", u32TickCount_ms, u32EncAfter - u32EncBefore);
	SERIAL_vFlush(sSerStream.u8Device);
	dispBytes("crypt  :", u8TestData10, 144);
	// 暗号文答え合わせ
	uint8 u8TestResult10[] = {
		(uint8)0x11, (uint8)0x74, (uint8)0x3F, (uint8)0xEF, (uint8)0x21, (uint8)0x11, (uint8)0xEF, (uint8)0xCE
	  , (uint8)0xAD, (uint8)0x1F, (uint8)0xB2, (uint8)0x2F, (uint8)0xBF, (uint8)0xD4, (uint8)0x88, (uint8)0x61
	  , (uint8)0x70, (uint8)0x28, (uint8)0xAB, (uint8)0x66, (uint8)0x56, (uint8)0x2F, (uint8)0x50, (uint8)0xCF
	  , (uint8)0x79, (uint8)0xD2, (uint8)0x3B, (uint8)0x06, (uint8)0x1B, (uint8)0xFF, (uint8)0x4B, (uint8)0xB5
	  , (uint8)0x18, (uint8)0xE4, (uint8)0xEA, (uint8)0xF1, (uint8)0x0A, (uint8)0xA8, (uint8)0x3B, (uint8)0xC3
	  , (uint8)0xA2, (uint8)0xC4, (uint8)0x0E, (uint8)0x61, (uint8)0xB4, (uint8)0x41, (uint8)0x9F, (uint8)0x83
	  , (uint8)0x0C, (uint8)0x8D, (uint8)0xDC, (uint8)0x78, (uint8)0x9B, (uint8)0xDB, (uint8)0xF7, (uint8)0x40
	  , (uint8)0xB2, (uint8)0x90, (uint8)0x3A, (uint8)0x4A, (uint8)0x3E, (uint8)0x12, (uint8)0x50, (uint8)0x74
	  , (uint8)0xA2, (uint8)0x35, (uint8)0x97, (uint8)0x10, (uint8)0x7F, (uint8)0xA8, (uint8)0x64, (uint8)0xED
	  , (uint8)0x04, (uint8)0xB5, (uint8)0x35, (uint8)0x5C, (uint8)0x08, (uint8)0x85, (uint8)0x13, (uint8)0x1C
	  , (uint8)0xC9, (uint8)0x0C, (uint8)0x81, (uint8)0xF5, (uint8)0xDA, (uint8)0xDC, (uint8)0xA4, (uint8)0xDF
	  , (uint8)0x43, (uint8)0xF5, (uint8)0xF6, (uint8)0x50, (uint8)0x3F, (uint8)0x91, (uint8)0x52, (uint8)0xF5
	  , (uint8)0x63, (uint8)0x93, (uint8)0x6C, (uint8)0x82, (uint8)0x8C, (uint8)0xEE, (uint8)0x78, (uint8)0xE8
	  , (uint8)0xD1, (uint8)0xA0, (uint8)0x8A, (uint8)0x82, (uint8)0x47, (uint8)0x5E, (uint8)0x73, (uint8)0x9B
	  , (uint8)0x5A, (uint8)0xD5, (uint8)0x62, (uint8)0xC6, (uint8)0xB1, (uint8)0xDE, (uint8)0xCD, (uint8)0x28
	  , (uint8)0x3B, (uint8)0x11, (uint8)0x7C, (uint8)0x4E, (uint8)0x4D, (uint8)0x29, (uint8)0xAF, (uint8)0x66
	  , (uint8)0x4F, (uint8)0x08, (uint8)0x88, (uint8)0xE3, (uint8)0x23, (uint8)0x4E, (uint8)0xAF, (uint8)0x4E
	  , (uint8)0x1B, (uint8)0x20, (uint8)0x26, (uint8)0x80, (uint8)0x8A, (uint8)0xB3, (uint8)0x85, (uint8)0xE4
	};
	if (memcmp(u8TestResult10, u8TestData10, 144) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.10 Ciphertext OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.10 Ciphertext NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
	// CBCモードでブロック（16byte）単位でデータを復号化
	sState = vAES_newCBCState(AES_KEY_LEN_256, u8Key10, u8IV10);
	uint32 u32DecBefore = u64TimerUtil_readUsec();
	vAES_decrypt(&sState, u8TestData10, 128);
	uint32 u32DecAfter = u64TimerUtil_readUsec();
	vfPrintf(&sSerStream, "\nMS:%08d No.10 vAES_decrypt %d usec", u32TickCount_ms, u32DecAfter - u32DecBefore);
	SERIAL_vFlush(sSerStream.u8Device);
	dispBytes("decrypt:", u8TestData10, 144);
	// 平文答え合わせ
	if (memcmp(u8Data10, u8TestData10, 128) == 0) {
		vfPrintf(&sSerStream, "\nMS:%08d No.10 Plaintext  OK", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	} else {
		vfPrintf(&sSerStream, "\nMS:%08d No.10 Plaintext  NG", u32TickCount_ms);
		SERIAL_vFlush(sSerStream.u8Device);
	}
}

/**
 * 配列表示
 */
static void dispBytes(char* pPrefix, uint8* bytes, uint8 u8Len) {
	char* wkPrefix = pPrefix;
	uint8 u8Idx = 0;
	while (u8Idx < u8Len) {
		if ((u8Idx % 8) == 0) {
			vfPrintf(&sSerStream, "\nMS:%08d %08s0x%02X", u32TickCount_ms, wkPrefix, bytes[u8Idx]);
			SERIAL_vFlush(sSerStream.u8Device);
			wkPrefix = "        ";
		} else {
			vfPrintf(&sSerStream, ", 0x%02X", bytes[u8Idx]);
			SERIAL_vFlush(sSerStream.u8Device);
		}
		u8Idx++;
	}
}

