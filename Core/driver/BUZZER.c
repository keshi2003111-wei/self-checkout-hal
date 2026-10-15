#include "BUZZER.h"

extern TIM_HandleTypeDef htim3;

/*
 * TIM3 CH2 PWM 参数:
 *   PSC = 83, ARR = 999
 *   PWM频率 = 168MHz / (83+1) / (999+1) = 2000 Hz
 *   CCR = 0    → 0% 占空比（无声）
 *   CCR = 500  → 50% 占空比（最大音量）
 */

void BUZZER_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0); // 初始关闭
}

void BUZZER_On(void)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 500); // 50% 占空比
}

void BUZZER_Off(void)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
}

void BUZZER_SetDuty(uint16_t duty)
{
    if (duty > 999) duty = 999;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, duty);
}

void BUZZER_Beep(void)
{
    BUZZER_On();
    HAL_Delay(100);
    BUZZER_Off();
}

void BUZZER_BeepLong(void)
{
    BUZZER_On();
    HAL_Delay(500);
    BUZZER_Off();
}

void BUZZER_BeepDouble(void)
{
    BUZZER_On();
    HAL_Delay(100);
    BUZZER_Off();
    HAL_Delay(100);
    BUZZER_On();
    HAL_Delay(100);
    BUZZER_Off();
}

void BUZZER_BeepError(void)
{
    uint8_t i;
    for (i = 0; i < 3; i++)
    {
        BUZZER_On();
        HAL_Delay(80);
        BUZZER_Off();
        HAL_Delay(80);
    }
}
