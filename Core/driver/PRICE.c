#include "PRICE.h"

static PRICE_Item_t price_table[PRICE_MAX_ITEMS];

void PRICE_Init(void)
{
    uint8_t i;
    for (i = 0; i < PRICE_MAX_ITEMS; i++)
    {
        price_table[i].id = i + 1;
        price_table[i].price = 0.0f;
    }
    // 默认价格表（示例）
    price_table[0].price = 5.0f;   // 商品1: 5元
    price_table[1].price = 10.0f;  // 商品2: 10元
    price_table[2].price = 15.0f;  // 商品3: 15元
    price_table[3].price = 20.0f;  // 商品4: 2 of 20元
    price_table[4].price = 25.0f;  // 商品5: 25元
    price_table[5].price = 30.0f;  // 商品6: 30元
    price_table[6].price = 30.0f;  // 商品7: 30元
    price_table[7].price = 35.0f;  // 商品8: 35元
    price_table[8].price = 40.0f;  // 商品9: 40元
    price_table[9].price = 45.0f;  // 商品10: 45元
}

float PRICE_GetPrice(uint8_t id)
{
    uint8_t i;
    for (i = 0; i < PRICE_MAX_ITEMS; i++)
    {
        if (price_table[i].id == id)
            return price_table[i].price;
    }
    return 0.0f; // 未找到
}

void PRICE_SetPrice(uint8_t id, float price)
{
    uint8_t i;
    for (i = 0; i < PRICE_MAX_ITEMS; i++)
    {
        if (price_table[i].id == id)
        {
            price_table[i].price = (price < 0) ? 0.0f : price;
            return;
        }
    }
}
