#include "AI_COMM.h"
#include <string.h>
#include <stdio.h>
#include "PRINTER.h"
#include "usart.h"
#include "KEY.h"

extern UART_HandleTypeDef huart2;

/* One-byte interrupt reception is used during the UART link check. */
static uint8_t  uart_rx_byte;
static uint8_t  line_buf[AI_BUF_SIZE];
static uint16_t line_index = 0;
static AI_Result_t ai_result = {0, 0, 0};

#define RECEIPT_MAX_ITEMS 10
typedef struct {
    uint8_t class_id;
    uint16_t weight_g;
    uint16_t unit_cents;
    uint16_t amount_cents;
} ReceiptItem_t;

static uint8_t arm_rx_byte;
static char arm_line[64];
static uint8_t arm_line_index;
static ReceiptItem_t receipt_items[RECEIPT_MAX_ITEMS];
static volatile uint8_t receipt_count;
static volatile uint8_t receipt_pending;
static volatile uint32_t receipt_total_cents;

static const char *Receipt_ProductName(uint8_t id)
{
    /* GBK: apple, banana, grapes, lemon, tomato, chili. */
    static const char *names[] = {
        "\xC6\xBB\xB9\xFB", "\xCF\xE3\xBD\xB6", "\xC6\xCF\xCC\xD1",
        "\xC4\xFB\xC3\xCA", "\xB7\xAC\xC7\xD1", "\xC0\xB1\xBD\xB7"
    };
    return id < 6U ? names[id] : "Unknown";
}

static void Printer_PrintReceiptLine(const char *text)
{
    Printer_SendByte(0x02); /* STX: printer discards stale RX bytes. */
    Printer_SendString(text);
    Printer_SendByte(0x0A);
    Printer_SendByte(0x0A);
    /* The printer controller prints synchronously; do not overwrite its RX job. */
    HAL_Delay(700U);
}

static void ARM_COMM_ParseLine(char *line)
{
    int a, b, c, d;
    if (strcmp(line, "CMD,START") == 0) {
        if (KEY_GetSystemState() == STATE_IDLE)
            KEY_SetSystemState(STATE_WEIGHING);
    } else if (strcmp(line, "CMD,CONFIRM") == 0) {
        if (KEY_GetSystemState() == STATE_WEIGHING ||
            KEY_GetSystemState() == STATE_CONFIRM)
            KEY_SetSystemState(STATE_SETTLING);
    } else if (strcmp(line, "CMD,CANCEL") == 0) {
        KEY_SetSystemState(STATE_IDLE);
    } else if (sscanf(line, "SALE,BEGIN,%d,%d", &a, &b) == 2) {
        receipt_count = 0U;
        receipt_total_cents = (uint32_t)(b < 0 ? 0 : b);
        receipt_pending = 0U;
        USART1_SendString("SALE:BEGIN RX\r\n");
    } else if (sscanf(line, "SALE,ITEM,%d,%d,%d,%d", &a, &b, &c, &d) == 4) {
        if (receipt_count < RECEIPT_MAX_ITEMS) {
            ReceiptItem_t *item = &receipt_items[receipt_count++];
            item->class_id = (uint8_t)a;
            item->weight_g = (uint16_t)b;
            item->unit_cents = (uint16_t)c;
            item->amount_cents = (uint16_t)d;
        }
    } else if (strcmp(line, "SALE,END") == 0) {
        receipt_pending = 1U;
        USART1_SendString("SALE:END RX\r\n");
    }
}

void AI_COMM_Init(void)
{
    line_index = 0;
    uart_rx_byte = 0;
    memset(line_buf, 0, sizeof(line_buf));
    ai_result.valid = 0;

    /* Test mode: every received character immediately enters the parser. */
    HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
    arm_line_index = 0U;
    receipt_count = 0U;
    receipt_pending = 0U;
    HAL_UART_Receive_IT(&huart1, &arm_rx_byte, 1);
}

void ARM_COMM_Process(void)
{
    uint8_t i;
    char text[64];
    if (receipt_pending == 0U)
        return;
    receipt_pending = 0U;

    /* Send one short job per line; long UART jobs are unreliable in its firmware. */
    Printer_PrintReceiptLine("SELF CHECKOUT RECEIPT");
    Printer_PrintReceiptLine("------------------------");
    for (i = 0U; i < receipt_count; ++i) {
        ReceiptItem_t *item = &receipt_items[i];
        sprintf(text, "\xC9\xCC\xC6\xB7:%s", Receipt_ProductName(item->class_id));
        Printer_PrintReceiptLine(text);
        sprintf(text, "\xD6\xD8\xC1\xBF:%ug", item->weight_g);
        Printer_PrintReceiptLine(text);
        sprintf(text, "\xB5\xA5\xBC\xDB:%u.%02u\xD4\xAA/kg",
                item->unit_cents / 100U, item->unit_cents % 100U);
        Printer_PrintReceiptLine(text);
        sprintf(text, "\xBD\xF0\xB6\xEE:%u.%02u\xD4\xAA",
                item->amount_cents / 100U, item->amount_cents % 100U);
        Printer_PrintReceiptLine(text);
    }
    Printer_PrintReceiptLine("------------------------");
    sprintf(text, "\xD7\xDC\xBC\xC6:%lu.%02lu\xD4\xAA",
            receipt_total_cents / 100U, receipt_total_cents % 100U);
    Printer_PrintReceiptLine(text);
    Printer_PrintReceiptLine("\xD0\xBB\xD0\xBB\xBB\xDD\xB9\xCB");
    Printer_PrintReceiptLine("");
    HAL_Delay(100U);
    UART4_SendString("PRINT,DONE\r\n");
}

void AI_COMM_SendDetect(void)
{
    /* Ignore a reply that arrived before this recognition request. */
    ai_result.valid = 0;
    HAL_UART_Transmit(&huart2, (uint8_t *)AI_CMD_DETECT,
                      strlen(AI_CMD_DETECT), 100);
}

uint8_t AI_COMM_IsReady(void)
{
    return ai_result.valid;
}

AI_Result_t AI_COMM_GetResult(void)
{
    AI_Result_t res = ai_result;
    ai_result.valid = 0;
    return res;
}

/* 鍦?USART2 涓柇涓皟鐢紝瑙ｆ瀽 K230 杩斿洖鐨勬暟鎹?*/
void AI_COMM_ParseData(uint8_t *data, uint16_t len)
{
    uint16_t i;
    char *p;
    int class_id = 0, conf = 0;

    for (i = 0; i < len; i++)
    {
        if (data[i] == '\n')
        {
            line_buf[line_index] = '\0';
            // 瑙ｆ瀽 "CLASS:3,CONF:95"
            p = strstr((char *)line_buf, "CLASS:");
            if (p)
            {
                if (sscanf(p, "CLASS:%d,CONF:%d", &class_id, &conf) == 2 &&
                    class_id >= 0 && class_id <= 255 &&
                    conf >= 0 && conf <= 100)
                {
                    ai_result.class_id = (uint8_t)class_id;
                    ai_result.confidence = (uint8_t)conf;
                    ai_result.valid = 1;
                }
            }
            line_index = 0;
        }
        else if (data[i] == '\r')
        {
            /* 鍚屾椂鍏煎 \n 鍜?\r\n 琛岀粨灏俱€?*/
        }
        else
        {
            if (line_index < AI_BUF_SIZE - 1)
            {
                line_buf[line_index++] = data[i];
            }
            else
            {
                /* 寮傚父闀挎暟鎹抚锛氫涪寮冨綋鍓嶈锛岄槻姝㈢紦鍐插尯婧㈠嚭銆?*/
                line_index = 0;
            }
        }
    }
}

/* USART2 receives one byte at a time during the link test. */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        AI_COMM_ParseData(&uart_rx_byte, 1);
        HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
    }
    else if (huart->Instance == USART1)
    {
        if (arm_rx_byte == '\n' || arm_rx_byte == '\r') {
            if (arm_line_index > 0U) {
                arm_line[arm_line_index] = '\0';
                ARM_COMM_ParseLine(arm_line);
                arm_line_index = 0U;
            }
        } else if (arm_line_index < sizeof(arm_line) - 1U) {
            arm_line[arm_line_index++] = (char)arm_rx_byte;
        } else {
            arm_line_index = 0U;
        }
        HAL_UART_Receive_IT(&huart1, &arm_rx_byte, 1);
    }
}
