/**
 * @file System_Kbd.h
 * @brief Kbd定義ヘッダ
 */

#ifndef _SYSTEM_KBD_H_
#define _SYSTEM_KBD_H_

#include "System.h"

/* DECLARATIONS ***************************************************************/

/* DECLARATIONS - enum ********************************************************/

/**
 * @enum KbdCode_e
 * @brief Kbd識別子
 * @note -
 */
typedef enum
{
    KBD_CODE_KEY_DOWN = (0),
    KBD_CODE_KEY_UP,
    KBD_CODE_JS_LEFT,
    KBD_CODE_JS_RIGHT,
    KBD_CODE_JS_DOWN,
    KBD_CODE_JS_UP,
    KBD_CODE_JS_CENTER,
    KBD_CODE_NUM,
    KBD_CODE_MIN = KBD_CODE_KEY_DOWN,
    KBD_CODE_MAX = KBD_CODE_JS_CENTER,
} KbdCode_e;

/* DECLARATIONS - struct ******************************************************/

/**
 * @struct KbdMessage_t
 * @brief メッセージ - Kbd用メッセージデータ
 * @note
 */
typedef struct
{
    uint8_t u8Code;
    uint8_t u8No;
    uint8_t ucReserve[30];
} KbdMessage_t;

/**
 * @struct MessageCtlKbd_t
 * @brief メッセージ - Kbd用メッセージ
 * @note
 */
typedef struct
{
    MessageHead_t Head_t;
    KbdMessage_t Data_t;
} MessageCtlKbd_t;

#endif /* _SYSTEM_KBD_H_ */
