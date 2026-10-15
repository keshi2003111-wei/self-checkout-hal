#ifndef __LED_H
#define __LED_H

#include "main.h"

// LED状态控制
void LED_Init(void); 
void LED_Green_On(void);
void LED_Green_Off(void);
void LED_Blue_On(void);
void LED_Blue_Off(void);
void LED_All_Off(void);

// 场景模式
void LED_ShowReady(void);      // 等待状态：绿灯亮
void LED_ShowWeighing(void);   // 称重中：蓝灯亮
void LED_ShowError(void);      // 错误：两个都灭

#endif