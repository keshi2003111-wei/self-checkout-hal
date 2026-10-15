#include "Printer.h"

extern UART_HandleTypeDef huart3;

// 发送 \x1B\x40，这是ESC/POS的复位命令，让打印机回到初始状态，清除之前的设置
void Printer_Init(void)
{
    /* The printer controller's UART path accepts plain text, not ESC/POS. */
}

//往串口发一个字节，最底层的发送函数
void Printer_SendByte(uint8_t data)
{
    HAL_UART_Transmit(&huart3, &data, 1, 100);
    HAL_Delay(1);
}

//发一串字节，比如发ESC/POS命令（命令都是几个字节一组）
void Printer_SendData(const uint8_t *data, uint16_t len)
{
    while (len-- > 0U)
    {
        Printer_SendByte(*data++);
    }
}

//发一个字符串，就是要打印的文字内容
void Printer_SendString(const char *str)
{
    uint16_t len = strlen(str);
    Printer_SendData((const uint8_t *)str, len);
}

//发字符串 + 换行（0x0A），相当于打印一行后自动换到下一行
void Printer_PrintLine(const char *str)
{
    Printer_SendString(str);
    Printer_SendByte(0x0A);
}

//先发居中对齐命令 \x1B\x61\x01，打印文字，再恢复左对齐。这样这行文字就在纸的中间
void Printer_PrintCenter(const char *str)
{
    Printer_SendData((const uint8_t *)P_CMD_ALIGN_C, 3);
    Printer_PrintLine(str);
    Printer_SendData((const uint8_t *)P_CMD_ALIGN_L, 3);
}

//先发加粗命令 \x1B\x45\x01，打印文字，再关加粗。用于突出显示，比如"总计"那行
void Printer_PrintBold(const char *str)
{
    Printer_SendData((const uint8_t *)P_CMD_BOLD_ON, 3);
    Printer_PrintLine(str);
    Printer_SendData((const uint8_t *)P_CMD_BOLD_OFF, 3);
}

//发双倍大小命令 \x1B\x21\x30，文字宽高都翻倍，然后居中打印，再恢复正常大小。一般用在标题上，比如店名
void Printer_PrintDouble(const char *str)
{
    Printer_SendData((const uint8_t *)P_CMD_DBL_HW, 3);
    Printer_PrintCenter(str);
    Printer_SendData((const uint8_t *)P_CMD_NORMAL, 3);
}

//发走纸命令 \x1B\x64 n，让打印机空走n行，用在打印结束后把纸推出来方便撕
void Printer_FeedLines(uint8_t n)
{
    uint8_t cmd[] = {0x1B, 0x64, n};
    Printer_SendData(cmd, 3);
}

//把上面这些组合起来，一键打印完整小票：标题（双倍大）→ 分割线 → 商品列表 → 分割线 → 总价（加粗）→ 走纸
void Printer_PrintReceipt(const char *title, const char *items, float total)
{
    char buf[64];

    Printer_Init();
    HAL_Delay(100);

    Printer_PrintDouble(title);
    Printer_PrintLine("----------------");
    Printer_SendString(items);
    Printer_PrintLine("----------------");

    Printer_PrintBold("Total:");
    sprintf(buf, "Total: %.2f RMB", total);
    Printer_PrintLine(buf);

    Printer_FeedLines(3);
}
