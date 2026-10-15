#ifndef __AI_COMM_H
#define __AI_COMM_H

#include "main.h"

/*
 * AI通信模块 - 与K230通过USART2通信
 * USART2: PA2(TX), PA3(RX), 115200 baud, DMA RX
 *
 * 通信协议（简化版）:
 *   STM32 → K230: "DETECT\n"          （触发识别）
 *   K230 → STM32: "CLASS:3,CONF:95\n"  （返回结果）
 */

#define AI_CMD_DETECT   "DETECT\n"
#define AI_BUF_SIZE     64

typedef struct
{
    uint8_t class_id;     // 识别到的商品类别
    uint8_t confidence;   // 置信度 (0~100)
    uint8_t valid;        // 结果是否有效
} AI_Result_t;

void         AI_COMM_Init(void);
void         AI_COMM_SendDetect(void);
AI_Result_t  AI_COMM_GetResult(void);
uint8_t      AI_COMM_IsReady(void);
void         ARM_COMM_Process(void);

#endif
