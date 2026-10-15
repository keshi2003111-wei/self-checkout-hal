#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

/*
 * 蜂鸣器驱动 - 使用 TIM3 CH2 (PB5) PWM 控制
 * TIM3 配置: PSC=83, ARR=999 → PWM 频率 ≈ 2kHz
 * 通过修改 ARR 可改变音调频率
 */

void BUZZER_Init(void);
void BUZZER_On(void);           // 蜂鸣器响（50%占空比）
void BUZZER_Off(void);          // 蜂鸣器停
void BUZZER_SetDuty(uint16_t duty); // 设置占空比 0~ARR
void BUZZER_Beep(void);         // 短促"滴"一声
void BUZZER_BeepLong(void);     // 长"滴——"
void BUZZER_BeepDouble(void);   // 双"滴滴"
void BUZZER_BeepError(void);    // 错误提示音（三短）

#endif
