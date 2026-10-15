#include "LED.h"

// LED2(PA6) 绿灯, LED3(PA7) 蓝灯, 低电平亮


void LED_Init(void)
{
    LED_All_Off();  // 初始化时全部关闭
}

void LED_Green_On(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
}

void LED_Green_Off(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
}

void LED_Blue_On(void)
{
    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
}

void LED_Blue_Off(void)
{
    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
}

void LED_All_Off(void)
{
    LED_Green_Off();
    LED_Blue_Off();
}

void LED_ShowReady(void)
{
    LED_Green_On();
    LED_Blue_Off();
}

void LED_ShowWeighing(void)
{
    LED_Blue_On();
    LED_Green_Off();
}

void LED_ShowError(void)
{
    LED_All_Off();
}