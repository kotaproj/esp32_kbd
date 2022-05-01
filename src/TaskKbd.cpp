/**
 * @file TaskKbd.cpp
 * @brief マウス管理タスク
 * @author kotaproj
 */
#include "TaskKbd.h"

/// @cond
#define _TASKKBD_C_
/// @endcond

/* DECLARATIONS ***************************************************************/
#define DEBUG_KBD_TASK (1)
#define ROUTE_KBD_TASK (1)

#define KBD_TASK_STACK (8192U)                       ///< タスクに割り当てるスタック容量
#define KBD_TASK_PRIORITY (configMAX_PRIORITIES - 6) ///< タスクの優先順位
#define KBD_QUEUE_LENGTH (4)                         ///< キューの深さ

/* VARIABLES ******************************************************************/
static BleKeyboard s_xBleKbd;
static QueueHandle_t s_xQueueKbd; ///< 受信管理用キューセット

/* PRIVATE PROTOTYPES *********************************************************/
static ErType_t xInitWorkKbd(void);
static void vTaskKbd(void *pvParameters);
static ErType_t xProcKbdCode(KbdMessage_t *pxMessage);
static ErType_t xProcKbdCode_KEY_DOWN(KbdMessage_t *pxMessage);
static ErType_t xProcKbdCode_KEY_UP(KbdMessage_t *pxMessage);
static ErType_t xProcKbdCode_JS_LEFT(KbdMessage_t *pxMessage);
static ErType_t xProcKbdCode_JS_RIGHT(KbdMessage_t *pxMessage);
static ErType_t xProcKbdCode_JS_DOWN(KbdMessage_t *pxMessage);
static ErType_t xProcKbdCode_JS_UP(KbdMessage_t *pxMessage);
static ErType_t xProcKbdCode_JS_CENTER(KbdMessage_t *pxMessage);
static void vProcSw_StartKindle(void);
static void vProcSw_Home(void);
static void vProcSw_ArrowLeft(void);
static void vProcSw_ArrowRight(void);

/* TABLES ***************************************************************/
typedef struct
{
    KbdCode_e eKbdCode;
    ErType_t (*xFunc)(KbdMessage_t *);
} KbdCodeFunt_t;

const KbdCodeFunt_t c_xaKbdCodeFunt_t[] = {
    {KBD_CODE_KEY_DOWN, xProcKbdCode_KEY_DOWN},
    {KBD_CODE_KEY_UP, xProcKbdCode_KEY_UP},
    {KBD_CODE_JS_LEFT, xProcKbdCode_JS_LEFT},
    {KBD_CODE_JS_RIGHT, xProcKbdCode_JS_RIGHT},
    {KBD_CODE_JS_DOWN, xProcKbdCode_JS_DOWN},
    {KBD_CODE_JS_UP, xProcKbdCode_JS_UP},
    {KBD_CODE_JS_CENTER, xProcKbdCode_JS_CENTER},
};

typedef struct
{
    uint8_t u8No;
    void (*vFunc)();
} KbdNoFunt_t;

const KbdNoFunt_t c_xaKbdNoFunt_t_forKeyDown[] = {
    {0, vProcSw_StartKindle}, // SW_CODE_NO1
    {1, vProcSw_ArrowLeft},   // SW_CODE_NO2
    {2, vProcSw_ArrowRight},  // SW_CODE_NO3
    {3, vProcSw_Home},        // SW_CODE_NO4
    {4, NULL},                // SW_CODE_NO5
    {5, NULL},                // SW_CODE_NO6
    {6, NULL},                // SW_CODE_NO7
    {7, NULL},                // SW_CODE_NO8
};

const KbdNoFunt_t c_xaKbdNoFunt_t_forKeyUp[] = {
    {0, NULL}, // SW_CODE_NO1
    {1, NULL}, // SW_CODE_NO2
    {2, NULL}, // SW_CODE_NO3
    {3, NULL}, // SW_CODE_NO4
    {4, NULL}, // SW_CODE_NO5
    {5, NULL}, // SW_CODE_NO6
    {6, NULL}, // SW_CODE_NO7
    {7, NULL}, // SW_CODE_NO8
};

/**
 * @brief タスク初期化関数
 */
ErType_t xInitKbd(void)
{
    // begin - Kbd
    s_xBleKbd.begin();

    // キューセットの作成
    s_xQueueKbd = xQueueCreate(KBD_QUEUE_LENGTH, sizeof(MessageCtlKbd_t));
    if (NULL == s_xQueueKbd)
    {
        return ER_FAIL;
    }

    // タスクの生成
    xTaskCreatePinnedToCore(vTaskKbd, "vTaskKbd", KBD_TASK_STACK, NULL, KBD_TASK_PRIORITY, NULL, ARDUINO_RUNNING_CORE);

    return ER_OK;
}

/**
 * @brief ワークエリアの初期化(タスク生成後)
 */
static ErType_t xInitWorkKbd(void)
{
    return ER_OK;
}

/**
 * @brief タスク処理
 */
static void vTaskKbd(void *pvParameters)
{
    static MessageCtlKbd_t s_xMessage;
    ErType_t xErType = ER_OK;

    // ワークエリア初期化処理
    xErType = xInitWorkKbd();

    //メインループ
    while (1)
    {
        if (pdPASS == xQueueReceive(s_xQueueKbd, &s_xMessage, portMAX_DELAY))
        {
            if (s_xBleKbd.isConnected())
            {
                xErType = xProcKbdCode(&s_xMessage.Data_t);
                if (ER_OK != xErType)
                {
                    Serial.print(__func__);
                    Serial.print("KBD_ERROR:");
                    Serial.println(xErType);
                }
            }
        }
    }

    return;
}

/**
 * @brief マウス処理
 */
static ErType_t xProcKbdCode(KbdMessage_t *pxMessage)
{
    for (uint16_t usCnt = 0; usCnt < sizeof(c_xaKbdCodeFunt_t) / sizeof(KbdCodeFunt_t); usCnt++)
    {
        if (c_xaKbdCodeFunt_t[usCnt].eKbdCode == pxMessage->u8Code)
        {
            return c_xaKbdCodeFunt_t[usCnt].xFunc(pxMessage);
        }
    }

    return ER_FAIL;
}

/**
 * @brief コードイベント - KEY_DOWN
 */
static ErType_t xProcKbdCode_KEY_DOWN(KbdMessage_t *pxMessage)
{
#if ROUTE_KBD_TASK
    Serial.printf("%s - run - (%d, %d)\n", __func__, pxMessage->u8Code, pxMessage->u8No);
#endif

    for (int i = 0; i < (sizeof(c_xaKbdNoFunt_t_forKeyDown) / sizeof(KbdNoFunt_t)); i++)
    {
        if (c_xaKbdNoFunt_t_forKeyDown[i].u8No == pxMessage->u8No && c_xaKbdNoFunt_t_forKeyDown[i].vFunc != NULL)
        {
            c_xaKbdNoFunt_t_forKeyDown[i].vFunc();
            return ER_OK;
        }
    }

    return ER_FAIL;
}

/**
 * @brief コードイベント - KEY_UP
 */
static ErType_t xProcKbdCode_KEY_UP(KbdMessage_t *pxMessage)
{
#if ROUTE_KBD_TASK
    Serial.printf("%s - run - (%d, %d)\n", __func__, pxMessage->u8Code, pxMessage->u8No);
#endif

    for (int i = 0; i < (sizeof(c_xaKbdNoFunt_t_forKeyUp) / sizeof(KbdNoFunt_t)); i++)
    {
        if (c_xaKbdNoFunt_t_forKeyUp[i].u8No == pxMessage->u8No && c_xaKbdNoFunt_t_forKeyUp[i].vFunc != NULL)
        {
            c_xaKbdNoFunt_t_forKeyUp[i].vFunc();
            return ER_OK;
        }
    }

    return ER_FAIL;
}

/**
 * @brief コードイベント - JS_LEFT
 */
static ErType_t xProcKbdCode_JS_LEFT(KbdMessage_t *pxMessage)
{
#if ROUTE_KBD_TASK
    Serial.printf("%s - run - (%d, %d)\n", __func__, pxMessage->u8Code, pxMessage->u8No);
#endif
    s_xBleKbd.write(KEY_LEFT_ARROW);
    return ER_OK;
}

/**
 * @brief コードイベント - JS_RIGHT
 */
static ErType_t xProcKbdCode_JS_RIGHT(KbdMessage_t *pxMessage)
{
#if ROUTE_KBD_TASK
    Serial.printf("%s - run - (%d, %d)\n", __func__, pxMessage->u8Code, pxMessage->u8No);
#endif
    s_xBleKbd.write(KEY_RIGHT_ARROW);
    return ER_OK;
}

/**
 * @brief コードイベント - JS_DOWN
 */
static ErType_t xProcKbdCode_JS_DOWN(KbdMessage_t *pxMessage)
{
#if ROUTE_KBD_TASK
    Serial.printf("%s - run - (%d, %d)\n", __func__, pxMessage->u8Code, pxMessage->u8No);
#endif
    s_xBleKbd.write(KEY_DOWN_ARROW);
    return ER_OK;
}

/**
 * @brief コードイベント - JS_UP
 */
static ErType_t xProcKbdCode_JS_UP(KbdMessage_t *pxMessage)
{
#if ROUTE_KBD_TASK
    Serial.printf("%s - run - (%d, %d)\n", __func__, pxMessage->u8Code, pxMessage->u8No);
#endif
    s_xBleKbd.write(KEY_UP_ARROW);
    return ER_OK;
}

/**
 * @brief コードイベント - JS_CENTER
 */
static ErType_t xProcKbdCode_JS_CENTER(KbdMessage_t *pxMessage)
{
#if ROUTE_KBD_TASK
    Serial.printf("%s - run - (%d, %d)\n", __func__, pxMessage->u8Code, pxMessage->u8No);
#endif
    return ER_OK;
}

/**
 * @brief キュー送信処理(Ctl->Kbd)
 */
ErType_t xSendKbdQueue_Code(UniId_t xSrcId, KbdCode_e eCode, uint8_t u8No)
{
    KbdMessage_t xData;

    xData.u8Code = (uint8_t)eCode;
    xData.u8No = u8No;

    return xSendKbdQueue(xSrcId, UID_KBD, &xData);
}

/**
 * @brief キュー送信処理(Ctl->Kbd)
 */
ErType_t xSendKbdQueue(UniId_t xSrcId, UniId_t xDstId, void *pvParam)
{
    MessageCtlKbd_t xMessage;

    xMessage.Head_t.xSrcId = xSrcId;
    xMessage.Head_t.xDstId = xDstId;

    memcpy(&xMessage.Data_t, pvParam, sizeof(xMessage.Data_t));

    if (pdPASS == xQueueSend(s_xQueueKbd, (void *)&xMessage, 0))
    {
        return ER_OK;
    }

    return ER_FAIL;
}

/**
 * @brief kindle appの起動
 */
static void vProcSw_StartKindle(void)
{
#if ROUTE_KBD_TASK
    Serial.printf("%s - run\n", __func__);
#endif
    // ->home (Cmd + h)
    s_xBleKbd.press(KEY_LEFT_GUI);
    delay(10);
    s_xBleKbd.print("h"); //文字を送信
    delay(10);
    s_xBleKbd.release(KEY_LEFT_GUI);
    delay(50);

    // ->command (Cmd + Space)
    s_xBleKbd.press(KEY_LEFT_GUI);
    delay(50);
    s_xBleKbd.print(" "); // space
    delay(50);
    s_xBleKbd.release(KEY_LEFT_GUI);
    delay(50);

    // Erase the string with backspace
    for (int i = 0; i < 10; i++)
    {
        s_xBleKbd.write(KEY_BACKSPACE);
        delay(50);
    }

    // input - "kindle"
    delay(500);
    s_xBleKbd.print("kindle");
    delay(50);

    // input - "enter"
    s_xBleKbd.write(KEY_RETURN);
    delay(50);

    return;
}

/**
 * @brief lockの起動
 */
static void vProcSw_Home(void)
{
#if ROUTE_KBD_TASK
    Serial.printf("%s - run\n", __func__);
#endif
    // ->home (Cmd + S)
    s_xBleKbd.press(KEY_LEFT_GUI);
    delay(50);
    s_xBleKbd.print("h"); //文字を送信
    delay(10);
    s_xBleKbd.release(KEY_LEFT_GUI);
    delay(50);
}

/**
 * @brief ArrowLeft
 */
static void vProcSw_ArrowLeft(void)
{
#if ROUTE_KBD_TASK
    Serial.printf("%s - run\n", __func__);
#endif
    s_xBleKbd.write(KEY_LEFT_ARROW);
}

/**
 * @brief ArrowRight
 */
static void vProcSw_ArrowRight(void)
{
#if ROUTE_KBD_TASK
    Serial.printf("%s - run\n", __func__);
#endif
    s_xBleKbd.write(KEY_RIGHT_ARROW);
}

#if DEBUG_KBD_TASK
static void vDummyTask(void *pvParameters)
{
    Serial.println("run");
    vTaskDelay(1000 / portTICK_RATE_MS);

    while (1)
    {
        xSendKbdQueue_Code(UID_CTL, KBD_CODE_KEY_DOWN, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);
        xSendKbdQueue_Code(UID_CTL, KBD_CODE_KEY_UP, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);

        xSendKbdQueue_Code(UID_CTL, KBD_CODE_KEY_DOWN, 1);
        vTaskDelay(1000 / portTICK_RATE_MS);
        xSendKbdQueue_Code(UID_CTL, KBD_CODE_KEY_UP, 1);
        vTaskDelay(1000 / portTICK_RATE_MS);

        xSendKbdQueue_Code(UID_CTL, KBD_CODE_KEY_DOWN, 2);
        vTaskDelay(1000 / portTICK_RATE_MS);
        xSendKbdQueue_Code(UID_CTL, KBD_CODE_KEY_UP, 2);
        vTaskDelay(1000 / portTICK_RATE_MS);

        xSendKbdQueue_Code(UID_CTL, KBD_CODE_JS_LEFT, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);

        xSendKbdQueue_Code(UID_CTL, KBD_CODE_JS_RIGHT, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);

        xSendKbdQueue_Code(UID_CTL, KBD_CODE_JS_DOWN, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);

        xSendKbdQueue_Code(UID_CTL, KBD_CODE_JS_UP, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);

        xSendKbdQueue_Code(UID_CTL, KBD_CODE_JS_CENTER, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    return;
}

/**
 * @brief テスト処理
 */
void setup_for_TaskKbd(void)
{
    // Serial.begin(115200);
    Serial.printf("%s - run\n", __func__);
    xTaskCreatePinnedToCore(vDummyTask, "vDummyTask", 8192, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
    xInitKbd();
}
#endif // DEBUG_KBD_TASK

#undef _TASKKBD_C_
