#ifndef __KEY_H
#define __KEY_H

#include "main.h"
#include "OLED.h"
#include "LED.h"

/* 按键返回值 */
#define KEY_NONE   0
#define KEY1_PRES  1   // PE3, 低电平有效 — 确认结算
#define KEY0_PRES  2   // PE4, 低电平有效 — 取消/删除
#define WKUP_PRES  3   // PA0, 高电平有效 — 开始称重

/* 系统状态 */
typedef enum {
    STATE_IDLE = 0,      // 待机，等待放商品
    STATE_WEIGHING,      // 称重中（稳定读数）
    STATE_CONFIRM,       // 等待确认结算
    STATE_SETTLING       // 结算完成，短暂提示
} KeyState_t;

void         KEY_Init(void);
uint8_t      KEY_Scan(void);          // 返回按下的键，无按键返回KEY_NONE
KeyState_t   KEY_GetSystemState(void); // 获取当前系统状态
void         KEY_SetSystemState(KeyState_t state);
void         KEY_Process(void);       // 核心：扫描按键 + 状态切换，放while(1)里调用

#endif