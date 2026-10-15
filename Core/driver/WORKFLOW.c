#include "WORKFLOW.h"
#include "usart.h"
#include "OLED.h"
#include "HX711.h"
#include "PRINTER.h"
#include "BUZZER.h"
#include "PRICE.h"
#include "AI_COMM.h"
#include "KEY.h"
#include "LED.h"
#include <stdio.h>
#include <math.h>

/* private variables */
static float current_weight = 0.0f;
static uint8_t weight_stable = 0;
/* Use a value far below any real weight so 000 g is rendered on first entry. */
static float displayed_weight = -1000.0f;

/* state enter flags: 1=just entered, cleared after first execution */
static uint8_t idle_enter = 1;
static uint8_t weighing_enter = 1;
static uint8_t settling_step = 0;

/* previous system state, for detecting state transitions */
static KeyState_t prev_state = STATE_IDLE;

/* forward declarations */
static void State_Idle(void);
static void State_Weighing(void);
static void State_Settling(void);

/*
 * ARM display protocol on USART1 (PA9 TX, PA10 RX through USB-to-TTL).
 * All monetary values use cents so the Qt side does not need to parse floats.
 * Example: ITEM,1,100,1000,1000\r\n
 */
static void ARM_SendItem(uint8_t class_id, float weight_g, float unit_price)
{
    char frame[64];
    int weight_int = (int)(weight_g + 0.5f);
    int price_cents = (int)(unit_price * 100.0f + 0.5f);

    if (weight_int < 0) weight_int = 0;
    if (price_cents < 0) price_cents = 0;

    sprintf(frame, "ITEM,%d,%d,%d,%d\r\n",
            class_id, weight_int, price_cents, price_cents);
    UART4_SendString(frame);
}

/**
 * @brief  Workflow init, reset all variables
 */
void WORKFLOW_Init(void)
{
    current_weight = 0.0f;
    weight_stable = 0;
    displayed_weight = -1000.0f;
    prev_state = STATE_IDLE;
    idle_enter = 1;
    weighing_enter = 1;
    settling_step = 0;
    UART4_SendString("ARM,READY\r\n");
}

/**
 * @brief  Called in main loop, dispatch to current state handler
 *         Check state transition first, reset enter flags
 */
void WORKFLOW_Process(void)
{
    KeyState_t state = KEY_GetSystemState();

    /* ---- detect state transition, reset enter flags ---- */
    if (state != prev_state)
    {
        printf("[STATE] %d -> %d\r\n", prev_state, state);
        switch (state)
        {
            case STATE_IDLE:
                idle_enter = 1;
                UART4_SendString("STATUS,IDLE\r\n");
                break;
            case STATE_WEIGHING:
                weighing_enter = 1;
                UART4_SendString("STATUS,WEIGHING\r\n");
                break;
            case STATE_SETTLING:
                settling_step = 0;
                UART4_SendString("STATUS,RECOGNIZING\r\n");
                break;
            default: break;
        }
        prev_state = state;
    }

    /* ---- dispatch to current state handler ---- */
    switch (state)
    {
        case STATE_IDLE:
            State_Idle();
            break;

        case STATE_WEIGHING:
            State_Weighing();
            break;

        case STATE_SETTLING:
            State_Settling();
            break;

        default:
            break;
    }
}

/* ========== state handler implementations ========== */

/**
 * @brief  Idle state - show welcome message, wait for user to place item and press WK_UP
 */
static void State_Idle(void)
{
    if (idle_enter)
    {
        OLED_Clear();
        OLED_ShowString(0, 0, "Place item");
        OLED_ShowString(2, 0, "WK_UP=start");
        LED_ShowReady();
        idle_enter = 0;
    }
}

/**
 * @brief  Weighing state - read weight, display on OLED
 *         Use displayed_weight (old value) for stability check to avoid noise
 *         Only refresh OLED when weight changes > 5g to reduce flicker
 *         Use OLED_ShowNum instead of sprintf(%.0f) because Keil sprintf
 *         does NOT support %f without MicroLIB
 */
static void State_Weighing(void)
{
    static uint8_t stable_printed = 0;
    static float stable_weight_value = 0;

    if (weighing_enter)
    {
        OLED_Clear();
        OLED_ShowString(0, 0, "Weight:");
        OLED_ShowString(0, 72, "g");
        LED_ShowWeighing();
        displayed_weight = -1000.0f;
        weighing_enter = 0;
    }

    /* read weight: kg -> g */
    current_weight = HX711_GetWeightKg() * 1000.0f;

    /* stability check: compare with displayed value, <2g change = stable */
    if (displayed_weight >= 0)
        weight_stable = (fabs(current_weight - displayed_weight) < 2.0f);
    else
        weight_stable = 0;

    /* only refresh OLED when weight changes > 5g */
    if (fabs(current_weight - displayed_weight) > 5.0f)
    {
        /* convert float to integer grams, use OLED_ShowNum */
        int weight_g = (int)(current_weight + 0.5f);
        if (weight_g < 0) weight_g = 0;
        OLED_ShowNum(0, 48, weight_g, 3);
        {
            char frame[40];
            sprintf(frame, "WEIGHT,%d,%d\r\n", weight_g, weight_stable ? 1 : 0);
            UART4_SendString(frame);
        }
        displayed_weight = current_weight;
    }

    /* print stable weight once */
    if (weight_stable && current_weight > 10.0f)
    {
        if (!stable_printed)
        {
            char frame[40];
            printf("[WEIGHT] %d g STABLE\r\n", (int)(current_weight + 0.5f));
            sprintf(frame, "WEIGHT,%d,1\r\n", (int)(current_weight + 0.5f));
            UART4_SendString(frame);
            stable_printed = 1;
            stable_weight_value = current_weight;
        }
    }
    else if (fabs(current_weight - stable_weight_value) > 10.0f)
    {
        stable_printed = 0;
    }
}

/**
 * @brief  Settling state - AI recognition + print receipt + return to idle
 *         Use settling_step for non-blocking step-by-step execution
 *         step 0: send AI detect request
 *         step 1: wait for AI result (timeout 3s)
 *         step 2: print receipt
 *         step 3: cleanup and return to idle
 */
static void State_Settling(void)
{
    static AI_Result_t ai_result;
    static float item_price = 0.0f;

    switch (settling_step)
    {
        case 0: /* send AI detect request */
            OLED_Clear();
            OLED_ShowString(0, 0, "Recognizing...");
            AI_COMM_SendDetect();
            BUZZER_Beep();
            settling_step = 1;
            break;

        case 1: /* wait for AI result */
            if (AI_COMM_IsReady())
            {
                ai_result = AI_COMM_GetResult();
                printf("[AI] class=%d, conf=%d, valid=%d\r\n",
                       ai_result.class_id, ai_result.confidence, ai_result.valid);

                if (ai_result.valid && ai_result.confidence > 50)
                {
                    item_price = PRICE_GetPrice(ai_result.class_id);
                    /* Notify the ARM UI as soon as recognition is confirmed. */
                    ARM_SendItem(ai_result.class_id, current_weight, item_price);

                    /* display result on OLED using OLED_ShowNum (avoid %f) */
                    OLED_Clear();
                    OLED_ShowString(0, 0, "Detected!");
                    OLED_ShowString(2, 0, "ID:");
                    OLED_ShowNum(2, 24, ai_result.class_id, 3);

                    /* show price as integer cents to avoid %f */
                    int price_x10 = (int)(item_price * 10.0f + 0.5f);
                    OLED_ShowString(4, 0, "P:");
                    OLED_ShowNum(4, 16, price_x10 / 10, 4);
                    OLED_ShowString(4, 48, ".");
                    OLED_ShowNum(4, 56, price_x10 % 10, 1);

                    BUZZER_BeepDouble();
                    /* Receipt is printed only after Qt reports payment complete. */
                    settling_step = 3;
                }
                else
                {
                    OLED_Clear();
                    OLED_ShowString(0, 0, "Recognize Fail");
                    BUZZER_BeepError();
                    UART4_SendString("STATUS,FAIL\r\n");
                    settling_step = 3;
                }
            }
            else
            {
                /* timeout check (3s) */
                static uint32_t wait_start = 0;
                if (wait_start == 0) wait_start = HAL_GetTick();
                if (HAL_GetTick() - wait_start > 3000)
                {
                    OLED_Clear();
                    OLED_ShowString(0, 0, "Timeout");
                    BUZZER_BeepError();
                    UART4_SendString("STATUS,FAIL\r\n");
                    wait_start = 0;
                    settling_step = 3;
                }
            }
            break;

        case 2: /* print receipt */
            LED_ShowReady();
            printf("[PRINT] ID:%d Weight:%dg Price:%d\r\n",
                   ai_result.class_id, (int)(current_weight + 0.5f), (int)(item_price * 100 + 0.5f));
            {
                char buf[64];

                Printer_PrintDouble("Self Checkout");
                Printer_PrintLine("----------------");

                sprintf(buf, "Item ID:%d", ai_result.class_id);
                Printer_PrintLine(buf);

                sprintf(buf, "Weight:%d g", (int)(current_weight + 0.5f));
                Printer_PrintLine(buf);

                /* price with 2 decimal places for receipt */
                {
                    int price_int = (int)(item_price * 100.0f + 0.5f);
                    sprintf(buf, "Price:%d.%02d RMB", price_int / 100, price_int % 100);
                    Printer_PrintLine(buf);
                }

                Printer_PrintLine("----------------");
                {
                    int price_int = (int)(item_price * 100.0f + 0.5f);
                    sprintf(buf, "Total:%d.%02d RMB", price_int / 100, price_int % 100);
                    Printer_PrintBold(buf);
                }

                Printer_FeedLines(3);
            }
            BUZZER_BeepLong();
            OLED_Clear();
            OLED_ShowString(0, 0, "Done!");
            OLED_ShowString(2, 0, "Total:");
            {
                int price_int = (int)(item_price * 100.0f + 0.5f);
                OLED_ShowNum(2, 48, price_int / 100, 3);
                OLED_ShowString(2, 72, ".");
                OLED_ShowNum(2, 80, price_int % 100, 2);
            }
            settling_step = 3;
            break;

        case 3: /* cleanup and return to idle */
            settling_step = 0;
            current_weight = 0.0f;
            weight_stable = 0;
            displayed_weight = -1000.0f;
            item_price = 0.0f;
            LED_ShowReady();
            KEY_SetSystemState(STATE_IDLE);
            break;
    }
}
