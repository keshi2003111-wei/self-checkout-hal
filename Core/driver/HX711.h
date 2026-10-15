#ifndef __HX711_H
#define __HX711_H

#include "main.h"

#define HX711_DOUT_PORT     GPIOD
#define HX711_DOUT_PIN      GPIO_PIN_2
#define HX711_SCK_PORT      GPIOD
#define HX711_SCK_PIN       GPIO_PIN_3

/* �궨����ϵ����ÿ�˶�Ӧ��ԭʼADC�仯�� */
#define HX711_SCALE         217.0f

void HX711_Init(void);
uint32_t HX711_ReadRaw(void);
int32_t HX711_ReadWeight(void);
void HX711_CalibrateZero(void);
float HX711_GetWeightKg(void);

float HX711_CalibrateScale(float known_weight_grams);
#endif