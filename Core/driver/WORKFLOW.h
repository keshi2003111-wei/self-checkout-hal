#ifndef __WORKFLOW_H
#define __WORKFLOW_H

#include "main.h"

/*
 * WORKFLOW - 结算流程业务逻辑模块
 *
 * 职责：每个状态下的实际工作（称重读取、AI识别、打印小票等）
 * KEY模块负责：按键扫描 + 状态切换 + LED/OLED更新
 * WORKFLOW负责：每个状态里"做什么"（读传感器、通信、打印）
 *
 * 状态流程：IDLE → WEIGHING → CONFIRM → SETTLING → IDLE
 *   IDLE     - 显示提示，等待用户按WK_UP开始
 *   WEIGHING - 持续读重量，显示在OLED上
 *   CONFIRM  - KEY模块处理确认/取消（WORKFLOW不做事）
 *   SETTLING - AI识别 + 打印小票 + 自动回到IDLE
 */

void WORKFLOW_Init(void);      /* 初始化（清零变量） */
void WORKFLOW_Process(void);   /* 主循环调用，根据当前状态执行对应业务 */

#endif
