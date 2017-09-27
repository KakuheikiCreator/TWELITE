/****************************************************************************
 *
 * MODULE :Framework functions header file
 *
 * CREATED:2015/03/20 08:30:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:基本的な機能と処理フローを提供するフレームワーク
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
#ifndef  FRAMEWORK_H_INCLUDED
#define  FRAMEWORK_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include "ToCoNet_use_mod.h"
#include "ToCoNet.h"
#include "ToCoNet_mod_prototype.h"
#include "ccitt8.h"
#include "utils.h"

#include "fprintf.h"
#include "serial.h"

// DEBUG options
#ifdef DEBUG
#include "serial.h"
#include "fprintf.h"
#include "sprintf.h"
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// 構造体：イベントタスク種別
typedef enum {
	E_EVENT_FWK_EMPTY = ToCoNet_EVENT_APP_BASE
} teFwkEvent;

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PUBLIC tsFILE sSerStream;
PUBLIC tsSerialPortSetup sSerPort;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/** タスク登録処理：ハードウェア割り込み処理 */
PUBLIC bool_t vRegisterHwIntTask(uint32 u32DeviceId, uint8 (*u8pFunc)(uint32 u32DeviceId, uint32 u32ItemBitmap));
/** イベントタスク登録処理 */
PUBLIC bool_t bRegisterEvtTask(teFwkEvent eEvt, void (*vpFunc)(uint32 u32EvtTimeMs));
/** イベントタスク登録解除処理 */
PUBLIC bool_t bUnregisterEvtTask(teFwkEvent eEvt);
/** スケジュール実行イベントの登録処理 */
PUBLIC int iEntryScheduleEvt(teFwkEvent eEvt, uint32 u32Interval, uint32 u32Offset, bool_t bRepeatFlg);
/** スケジュール実行イベントの登録解除処理 */
PUBLIC bool_t bCancelScheduleEvt(int iEvtID);
/** 順次実行イベントの登録処理 */
PUBLIC int iEntrySeqEvt(teFwkEvent eEvt);
/** 順次実行イベントの登録解除処理 */
PUBLIC bool_t bCancelSeqEvt(int iEvtID);


#if defined __cplusplus
}
#endif

#endif  /* MAIN_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
