#ifndef __PRICE_H
#define __PRICE_H

#include "main.h"

/*
 * 价格表模块 - 存储商品编码与对应价格
 * 商品编码由 K230 识别后返回
 */

#define PRICE_MAX_ITEMS  10  // 最大商品种类数

typedef struct
{
    uint8_t id;           // 商品编码 (1~10)
    float price;          // 商品价格 (元)
} PRICE_Item_t;

void PRICE_Init(void);
float PRICE_GetPrice(uint8_t id);
void PRICE_SetPrice(uint8_t id, float price);

#endif
