#ifndef __OLED_H
#define __OLED_H

#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define OLED_ADDR       0x78  // OLED I2C从机地址
#define OLED_CMD        0x00  // 控制字节：命令
#define OLED_DATA       0x40  // 控制字节：显示数据
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_PAGES      8  // 64/8=8页 SSD1306页模式
extern I2C_HandleTypeDef hi2c1;

void OLED_Init(void);
void OLED_Clear(void);
void OLED_SetCursor(uint8_t page, uint8_t column);
void OLED_ShowChar(uint8_t page, uint8_t column, char ch);
void OLED_ShowString(uint8_t page, uint8_t column, char *str);
void OLED_ShowNum(uint8_t page, uint8_t column, int32_t num, uint8_t len);
void OLED_Update(void);
void OLED_Showf(uint8_t page, uint8_t column, const char *fmt, ...);

#endif