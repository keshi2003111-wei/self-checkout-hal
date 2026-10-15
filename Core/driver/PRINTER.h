#ifndef __PRINTER_H
#define __PRINTER_H

#include "main.h"
#include <string.h>
#include <stdio.h>

#define P_CMD_INIT       "\x1B\x40"
#define P_CMD_ALIGN_L    "\x1B\x61\x00"
#define P_CMD_ALIGN_C    "\x1B\x61\x01"
#define P_CMD_ALIGN_R    "\x1B\x61\x02"
#define P_CMD_BOLD_ON    "\x1B\x45\x01"
#define P_CMD_BOLD_OFF   "\x1B\x45\x00"
#define P_CMD_DBL_H      "\x1B\x21\x10"
#define P_CMD_DBL_W      "\x1B\x21\x20"
#define P_CMD_DBL_HW     "\x1B\x21\x30"
#define P_CMD_NORMAL     "\x1B\x21\x00"
#define P_CMD_LINE_FEED  "\x0A"
#define P_CMD_FEED_N(n)  {0x1B, 0x64, (n)}
#define P_CMD_CUT        "\x1D\x56\x00"

void Printer_Init(void);
void Printer_SendByte(uint8_t data);
void Printer_SendData(const uint8_t *data, uint16_t len);
void Printer_SendString(const char *str);
void Printer_PrintLine(const char *str);
void Printer_PrintCenter(const char *str);
void Printer_PrintBold(const char *str);
void Printer_PrintDouble(const char *str);
void Printer_FeedLines(uint8_t n);
void Printer_PrintReceipt(const char *title, const char *items, float total);

#endif