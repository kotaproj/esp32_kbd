/**
 * @file ApiSw.c
 * @brief スイッチ管理
 */

#include "ApiSw.h"
#include "CtlTaskSw.h"

/// @cond
#define _API_SW_C_
/// @endcond

/* DECLARATIONS ***************************************************************/
#define DEBUG_API_SW (1)   // 0:release, 1:debug
#define SW_CHECK_TIME (25) //チャダリング除去時間:300msec

/* VARIABLES ******************************************************************/
static TimerHandle_t s_xTimerSw = NULL;

/* PRIVATE PROTOTYPES *********************************************************/
static void vTimerCallbackSw(TimerHandle_t xTimer);
static void vSendSw(SwCode_t xCode, SwType_t xType);
static bool bReadApiSwIdx(int idx);

/* TABLES ***************************************************************/
typedef struct
{
    SwCode_t xSwCode;
    uint8_t ucPin;
    int input;       // INPUT, INPUT_PULLUP, INPUT_PULLDOWN
    int mode;        // LOW, CHANGE, RISING,FALLING
    bool pushed;     // false:pushなし, true:pushあり
    int push_cnt;    // pushしている連続値
    int release_cnt; // releaseしている連続値
} SwGpioAsgn_t;

SwGpioAsgn_t s_xaSwGpioAsgn_t[] = {
#if USE_SW_NO1
    {SW_CODE_NO1, SW_GPIO_ASGN_NO1, INPUT_PULLUP, FALLING, false, 0, 0},
#endif // USE_SW_NO1
#if USE_SW_NO2
    {SW_CODE_NO2, SW_GPIO_ASGN_NO2, INPUT_PULLUP, FALLING, false, 0, 0},
#endif // USE_SW_NO2
#if USE_SW_NO3
    {SW_CODE_NO3, SW_GPIO_ASGN_NO3, INPUT_PULLUP, FALLING, false, 0, 0},
#endif // USE_SW_NO3
#if USE_SW_NO4
    {SW_CODE_NO4, SW_GPIO_ASGN_NO4, INPUT_PULLUP, FALLING, false, 0, 0},
#endif // USE_SW_NO4
#if USE_SW_NO5
    {SW_CODE_NO5, SW_GPIO_ASGN_NO5, INPUT_PULLUP, FALLING, false, 0, 0},
#endif // USE_SW_NO5
};

/**
 * @brief スイッチ処理初期化関数
 */
ErType_t xInitSw(void)
{
    Serial.printf("%s : run\n", __func__);

    ErType_t xErType = ER_OK;

    // GPIO PIN設定
    for (uint16_t usCnt = 0; usCnt < sizeof(s_xaSwGpioAsgn_t) / sizeof(SwGpioAsgn_t); usCnt++)
    {
        // GPIO Input Mode、プルアップで設定
        // ->MPU内部のプルアップ抵抗を使用する
        pinMode(s_xaSwGpioAsgn_t[usCnt].ucPin, s_xaSwGpioAsgn_t[usCnt].input);
    }

    // 監視タイマーの作成
    //               [0],[1],[2],[3],[4],[5],[6],[7],
    char cName[8] = {'T', 'm', 'r', 'S', 'w', '0', 0x00, 0x00};

    // text name, timer period, auto-reload, number of times, callback
    s_xTimerSw = xTimerCreate(cName, SW_CHECK_TIME, pdTRUE, (void *)0, vTimerCallbackSw);
    if (NULL == s_xTimerSw)
    {
        Serial.printf("%s : error - xTimerCreate\n", __func__);
        return ER_FAIL;
    }

    if (pdTRUE != xTimerReset(s_xTimerSw, SW_CHECK_TIME))
    {
        Serial.printf("%s : error - xTimerReset\n", __func__);
        return ER_FAIL;
    }

    Serial.printf("%s : over\n", __func__);
    return xErType;
}

/**
 * @brief スイッチのタイマーコールバック関数
 */
static void vTimerCallbackSw(TimerHandle_t xTimer)
{
    bool cur_push;

    for (uint16_t usCnt = 0; usCnt < sizeof(s_xaSwGpioAsgn_t) / sizeof(SwGpioAsgn_t); usCnt++)
    {
        cur_push = bReadApiSwIdx(usCnt);
        if (s_xaSwGpioAsgn_t[usCnt].pushed)
        {
            // 押されている状態での判定
            if (cur_push)
            {
                s_xaSwGpioAsgn_t[usCnt].release_cnt = 0;
                continue;
            }
            else
            {
                s_xaSwGpioAsgn_t[usCnt].release_cnt += 1;
                if (s_xaSwGpioAsgn_t[usCnt].release_cnt > 3)
                {
                    s_xaSwGpioAsgn_t[usCnt].push_cnt = 0;
                    s_xaSwGpioAsgn_t[usCnt].release_cnt = 0;
                    // up event送信
                    vSendSw(s_xaSwGpioAsgn_t[usCnt].xSwCode, SW_TYPE_UP);
                    s_xaSwGpioAsgn_t[usCnt].pushed = false;
                }
            }
        }
        else
        {
            // 離している状態での判定
            if (cur_push)
            {
                s_xaSwGpioAsgn_t[usCnt].push_cnt += 1;
                if (s_xaSwGpioAsgn_t[usCnt].push_cnt > 3)
                {
                    s_xaSwGpioAsgn_t[usCnt].push_cnt = 0;
                    s_xaSwGpioAsgn_t[usCnt].release_cnt = 0;
                    // down event送信
                    vSendSw(s_xaSwGpioAsgn_t[usCnt].xSwCode, SW_TYPE_DOWN);
                    s_xaSwGpioAsgn_t[usCnt].pushed = true;
                }
            }
            else
            {
                s_xaSwGpioAsgn_t[usCnt].push_cnt = 0;
                continue;
            }
        }
    }
}

/**
 * @brief 管理タスクへスイッチイベントを通知
 */
static void vSendSw(SwCode_t xCode, SwType_t xType)
{
#if DEBUG_API_SW
    static int32_t s_count = 0;
    s_count++;
    Serial.printf("%s : xCode:%d, xType:%d, count:%d\n", __func__, xCode, xType, s_count);
#endif

    static SwMessage_t s_xMessage;

    s_xMessage.ulCode = (uint32_t)xCode;
    s_xMessage.ulType = (uint32_t)xType;

    vCallbackCtlMsgSw(UID_SW, UID_CTL, &s_xMessage);

    return;
}

/**
 * @brief スイッチ状態の取得
 */
static bool bReadApiSwIdx(int idx)
{
    int sw_sts;

    sw_sts = digitalRead(s_xaSwGpioAsgn_t[idx].ucPin);
    if (SW_STS_PRESSED == sw_sts && FALLING == s_xaSwGpioAsgn_t[idx].mode)
    {
        return true;
    }
    if (SW_STS_RELEASED == sw_sts && RISING == s_xaSwGpioAsgn_t[idx].mode)
    {
        return true;
    }

    return false;
}

#undef _API_SW_C_