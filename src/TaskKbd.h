/**
 * @file TaskKbd.h
 * @brief Kbdタスク
 * @author kotaproj
 */

#ifndef _TASKKBD_H_
#define _TASKKBD_H_

/* INCLUDES *******************************************************************/
#include "System.h"
#include "System_Kbd.h"
#include <BleKeyboard.h>

/******************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

///	@cond
#ifdef _TASKKBD_C_
#define EXTERN
#else
#define EXTERN extern
#endif
    ///	@endcond

    /* DECLARATIONS ***************************************************************/

    /* VARIABLES ******************************************************************/

    /* PROTOTYPES *****************************************************************/
    EXTERN ErType_t xInitKbd(void);
    EXTERN ErType_t xSendKbdQueue_Code(UniId_t xSrcId, KbdCode_e eCode, uint8_t u8No);
    EXTERN ErType_t xSendKbdQueue(UniId_t xSrcId, UniId_t xDstId, void *pvParam);
    EXTERN void setup_for_TaskKbd(void);

/******************************************************************************/
#undef EXTERN

#ifdef __cplusplus
}
#endif
#endif /* _TASKKBD_H_ */
