#include "HX711.h"

#define HX711_READY_TIMEOUT_MS  150U

static uint8_t hx711_last_read_ok = 0U;

// HX711 driver, SCK idle LOW
void HX711_Init(void)
{
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);
}

// Read 24-bit raw ADC value from HX711
// Send 24 SCK pulses to read data (MSB first), then 1 extra pulse to reset HX711 for next 128 gain
uint32_t HX711_ReadRaw(void)
{
    uint32_t val = 0;
    uint8_t i;

    uint32_t wait_start = HAL_GetTick();

    // Wait for DOUT to go LOW (data ready), but never block the whole system.
    while (HAL_GPIO_ReadPin(HX711_DOUT_PORT, HX711_DOUT_PIN) == GPIO_PIN_SET)
    {
        if ((HAL_GetTick() - wait_start) >= HX711_READY_TIMEOUT_MS)
        {
            hx711_last_read_ok = 0U;
            return 0U;
        }
    }
    hx711_last_read_ok = 1U;

    // Read 24 bits, MSB first
    for (i = 0; i < 24; i++)
    {
        HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_SET);
        val = val << 1;
        if (HAL_GPIO_ReadPin(HX711_DOUT_PORT, HX711_DOUT_PIN) == GPIO_PIN_SET)
            val |= 1;
        HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);
    }

    // 25th pulse: reset HX711 for next 128 gain
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HX711_SCK_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);

    return val;
}

// Convert 24-bit raw to 32-bit signed (sign extend)
int32_t HX711_ReadWeight(void)
{
    uint32_t raw = HX711_ReadRaw();
    int32_t val;

    if (hx711_last_read_ok == 0U)
        return 0;

    if (raw & 0x800000)
        val = (int32_t)(raw | 0xFF000000);
    else
        val = (int32_t)raw;

    return val;
}

/* Zero offset, auto-calibrated at power on */
static int32_t zero_offset = 0;
static float scale_factor = HX711_SCALE;

// Calibrate zero: average 20 readings with no load
void HX711_CalibrateZero(void)
{
    int32_t sum = 0;
    uint8_t i;
    for (i = 0; i < 20; i++)
    {
        sum += HX711_ReadWeight();
        if (hx711_last_read_ok == 0U)
        {
            zero_offset = 0;
            return;
        }
        HAL_Delay(50);
    }
    zero_offset = sum / 20;
}

// Get weight in grams (auto subtract zero offset)
float HX711_GetWeightKg(void)
{
    int32_t sum = 0;
    uint8_t i;
    float weight;

    for (i = 0; i < 10; i++)
    {
        sum += HX711_ReadWeight();
        if (hx711_last_read_ok == 0U)
            return 0.0f;
        HAL_Delay(50);
    }

    weight = (float)(sum / 10 - zero_offset) / scale_factor / 1000.0f;
    if (weight < 0) weight = 0;
    return weight;
}

// Calibrate scale factor with known weight
// Returns the new scale_factor value
float HX711_CalibrateScale(float known_weight_grams)
{
    int32_t sum = 0;
    uint8_t i;

    for (i = 0; i < 10; i++)
    {
        sum += HX711_ReadWeight();
        if (hx711_last_read_ok == 0U)
            return scale_factor;
        HAL_Delay(50);
    }

    int32_t avg = sum / 10;
    scale_factor = (float)(avg - zero_offset) / known_weight_grams;

    extern void USART1_Printf(const char *fmt, ...);
    USART1_Printf("[CAL] New SCALE = %.2f (update HX711.h)\r\n", scale_factor);

    return scale_factor;
}
