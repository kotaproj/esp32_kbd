/**
 * @file CtlTaskJs.c
 * @brief 全体管理タスク - ジョイスティック処理
 */
#include "CtlTaskJs.h"
#include "TaskKbd.h"

/// @cond
#define _CTLTASK_JS_C_
/// @endcond

/* DECLARATIONS ***************************************************************/
#define DEBUG_CTL_TASK_JS 0
#if DEBUG_CTL_TASK_JS
void vPrintCtlJs(const char *msg, const char *func, const char *file) { Serial.printf("%s - %s() : %s\n", file, func, msg); }
#else                            // DEBUG_CTL_TASK
void vPrintCtlJs(const char *msg, const char *func, const char *file) {}
#endif                           // DEBUG_CTL_TASK_JS
#define CTL_QUEUE_LENGTH_JS (1U) ///< フロントジョイスティックキューバッファ数

/* VARIABLES ******************************************************************/
static xQueueHandle s_xQueueJs; ///< フロントジョイスティックキューハンドラ

/* PRIVATE PROTOTYPES *********************************************************/
static void vProcCtlCmdJs(JsMessage_t *pxData_t);

/* CODE ***********************************************************************/

/**
 * @brief 初期化-タスク生成前
 */
ErType_t xInitCtlJs(QueueSetHandle_t *pxQueSetHandle)
{
    s_xQueueJs = xQueueCreate(CTL_QUEUE_LENGTH_JS, sizeof(MessageCtlJs_t));
    if (NULL == s_xQueueJs)
    {
        return ER_FAIL;
    }

    if (pdPASS != xQueueAddToSet(s_xQueueJs, *pxQueSetHandle))
    {
        return ER_FAIL;
    }

    return ER_OK;
}

/**
 * @brief ワークエリア初期化-タスク生成後
 */
ErType_t xInitWorkCtlJs(void)
{
    return ER_OK;
}

/**
 * @brief キューの深さ取得
 */
uint32_t ulGetQueLengthCtlJs(void)
{
    return CTL_QUEUE_LENGTH_JS;
}

/**
 * @brief キュー送信処理(Js->Ctl)
 */
void vCallbackCtlMsgJs(UniId_t xSrcId, UniId_t xDstId, void *pvParam)
{
    MessageCtlJs_t xMessage;

    //ヘッダ情報
    xMessage.Head_t.xSrcId = xSrcId;
    xMessage.Head_t.xDstId = xDstId;

    //データ情報
    memcpy(&xMessage.Data_t, (JsMessage_t *)pvParam, sizeof(JsMessage_t));

    //キューの送信
    if (pdTRUE != xQueueSend(s_xQueueJs, &xMessage, 0))
    {
        MessageCtlJs_t xDummy;

        // 送信できないときは、古いデータを削除
        // キューの空読み
        xQueueReceive(s_xQueueJs, &xDummy, 0);
        // キューへ再送信
        xQueueSend(s_xQueueJs, &xMessage, 0);
    }

    return;
}

/**
 * @brief キュー受信処理(Js->Ctl)
 */
ErType_t xReciveQueJs(QueueSetMemberHandle_t *pxQueSetMember, MessageCtlJs_t *pxMessage)
{
    if (s_xQueueJs == *pxQueSetMember)
    {
        xQueueReceive(*pxQueSetMember, pxMessage, 0);
        if (UID_CTL == pxMessage->Head_t.xDstId)
        {
            vProcCtlCmdJs(&(pxMessage->Data_t));
            return ER_OK;
        }
        else
        {
            return ER_PARAM;
        }
    }

    return ER_STATUS;
}

/**
 * @brief ジョイスティック処理(Ctl->Mouse)
 */
static void vProcCtlCmdJs(JsMessage_t *pxData_t)
{
    vPrintCtlJs("run", __func__, __FILE__);

    switch (pxData_t->ulCode)
    {
    case JS_CODE_LEFT:
        xSendKbdQueue_Code(UID_CTL, KBD_CODE_JS_LEFT, 0);
        break;

    case JS_CODE_RIGHT:
        xSendKbdQueue_Code(UID_CTL, KBD_CODE_JS_RIGHT, 0);
        break;

    case JS_CODE_DOWN:
        xSendKbdQueue_Code(UID_CTL, KBD_CODE_JS_DOWN, 0);
        break;

    case JS_CODE_UP:
        xSendKbdQueue_Code(UID_CTL, KBD_CODE_JS_UP, 0);
        break;

    case JS_CODE_CENTER:
    case JS_CODE_AXIS:
    default:
        return;
    }

    return;
}

#undef _CTLTASK_JS_C_