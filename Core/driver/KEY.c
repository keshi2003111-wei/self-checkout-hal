#include "KEY.h"
#include "HX711.h"
#include "usart.h"
#include "OLED.h"
#include <stdio.h>

static KeyState_t sys_state = STATE_IDLE;

void KEY_Init(void)
{
    sys_state = STATE_IDLE;
}

uint8_t KEY_Scan(void)
{
    uint8_t key1 = HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);
    uint8_t key0 = HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin);
    uint8_t wkup = HAL_GPIO_ReadPin(WK_UP_GPIO_Port, WK_UP_Pin);

    if (key1 == GPIO_PIN_RESET || key0 == GPIO_PIN_RESET || wkup == GPIO_PIN_SET)
    {
        HAL_Delay(10);
        key1 = HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);
        key0 = HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin);
        wkup = HAL_GPIO_ReadPin(WK_UP_GPIO_Port, WK_UP_Pin);

        if (key1 == GPIO_PIN_RESET) return KEY1_PRES;
        if (key0 == GPIO_PIN_RESET) return KEY0_PRES;
        if (wkup == GPIO_PIN_SET)   return WKUP_PRES;
    }
    return KEY_NONE;
}

KeyState_t KEY_GetSystemState(void)
{
    return sys_state;
}

void KEY_SetSystemState(KeyState_t state)
{
    sys_state = state;
}

void KEY_Process(void)
{
    /* ==== WK_UP long-press detection (for calibration) ==== */
    /* WK_UP is handled on RELEASE: short press = start weighing, long press(3s) = calibrate */
    static uint32_t wkup_press_time = 0;
    static uint8_t  wkup_long_fired = 0;

    if (HAL_GPIO_ReadPin(WK_UP_GPIO_Port, WK_UP_Pin) == GPIO_PIN_SET)
    {
        /* Button is physically held */
        if (wkup_press_time == 0)
            wkup_press_time = HAL_GetTick();

        if (!wkup_long_fired && (HAL_GetTick() - wkup_press_time >= 3000))
        {
            wkup_long_fired = 1;
            /* Enter calibration mode */
            if (sys_state == STATE_IDLE)
            {
                OLED_Clear();
                OLED_ShowString(0, 0, (char *)"Calibrate Mode");
                printf("[CAL] Waiting for 100g weight...\r\n");
                while (1)
                {
                    /* Show raw ADC value on line 2 */
                    int32_t raw_val = HX711_ReadWeight();
                    OLED_ShowString(2, 0, (char *)"R=");
                    OLED_ShowNum(2, 16, raw_val, 8);

                    /* Show calculated weight on line 4 */
                    int weight_g = (int)(HX711_GetWeightKg() * 1000.0f + 0.5f);
                    OLED_ShowString(4, 0, (char *)"W= ");
                    OLED_ShowNum(4, 24, weight_g, 4);
                    OLED_ShowString(4, 56, (char *)"g     ");

                    uint8_t k = KEY_Scan();
                    if (k == KEY1_PRES)
                    {
                        float new_scale = HX711_CalibrateScale(100.0f);
                        int sx = (int)(new_scale * 100.0f + 0.5f);
                        if (sx < 0) sx = -sx;
                        OLED_Clear();
                        OLED_ShowString(0, 0, (char *)"SCALE=");
                        OLED_ShowNum(0, 48, sx / 100, 4);
                        OLED_ShowNum(0, 96, sx % 100, 2);
                        HAL_Delay(5000);
                        OLED_Clear();
                        OLED_ShowString(0, 0, "Place item");
                        OLED_ShowString(2, 0, "WK_UP=start");
                        break;
                    }
                    HAL_Delay(50);
                }
            }
        }
        /* Don't process state machine while WK_UP is held */
        return;
    }
    else
    {
        /* Button released */
        if (wkup_press_time != 0)
        {
            uint32_t held = HAL_GetTick() - wkup_press_time;
            if (held < 3000 && !wkup_long_fired && sys_state == STATE_IDLE)
            {
                /* Short press: start weighing */
                sys_state = STATE_WEIGHING;
                LED_ShowWeighing();
                OLED_Clear();
                OLED_ShowString(0, 0, "Weighing...");
            }
            wkup_press_time = 0;
            wkup_long_fired = 0;
        }
    }

    /* ==== Other keys (KEY1, KEY0) ==== */
    uint8_t key = KEY_Scan();
    if (key == KEY_NONE) return;

    switch (sys_state)
    {
    case STATE_IDLE:
        break;

    case STATE_WEIGHING:
        if (key == KEY1_PRES)
        {
            sys_state = STATE_CONFIRM;
            LED_ShowReady();
            OLED_Clear();
            OLED_ShowString(0, 0, "Confirm?");
            OLED_ShowString(0, 2, "KEY1=OK KEY0=Cancel");
        }
        else if (key == KEY0_PRES)
        {
            sys_state = STATE_IDLE;
            LED_ShowReady();
            OLED_Clear();
            OLED_ShowString(0, 0, "Ready");
        }
        break;

    case STATE_CONFIRM:
        if (key == KEY1_PRES)
        {
            sys_state = STATE_SETTLING;
        }
        else if (key == KEY0_PRES)
        {
            sys_state = STATE_WEIGHING;
            OLED_Clear();
            OLED_ShowString(0, 0, "Weighing...");
        }
        break;

    case STATE_SETTLING:
        break;
    }
}
