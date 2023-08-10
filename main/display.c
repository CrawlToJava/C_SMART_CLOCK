#include "display.h"

#include "driver/gpio.h"
#include "driver/timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <stdio.h>
#include <stdlib.h>

#define TIMER_DIVIDER (16) //  Hardware timer clock divider
#define TIMER_SCALE (APB_CLK_FREQ / TIMER_DIVIDER)

bool ss = false;

static char *TAG = "DISPLAY";

static uint8_t digit_count = 1;

// static const uint8_t digits[4] = {
//     DIGIT_1_GPIO,
//     DIGIT_2_GPIO,
//     DIGIT_3_GPIO,
//     DIGIT_4_GPIO};

static const uint8_t digits_table[11] = {
    0b01111110, // 0
    0b00001100, // 1
    0b10110110, // 2
    0b10011110, // 3
    0b11001100, // 4
    0b11011010, // 5
    0b11111010, // 6
    0b00001110, // 7
    0b11111110, // 8
    0b11011110, // 9
};

static void shift_register_gpio_init(void)
{
    gpio_config_t shift_register_gpio = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = ((1ULL << DATA_GPIO) | (1ULL << LATCH_GPIO) | (1ULL << CLOCK_GPIO)),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE};

    ESP_ERROR_CHECK(gpio_config(&shift_register_gpio));
}

static void display_digit_gpio_init(void)
{
    gpio_config_t display_digit_gpio = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = ((1ULL << DIGIT_1_GPIO) | (1ULL << DIGIT_2_GPIO) | (1ULL << DIGIT_3_GPIO) | (1ULL << DIGIT_4_GPIO) | (1ULL << GPIO_NUM_16)),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE};

    ESP_ERROR_CHECK(gpio_config(&display_digit_gpio));
}

static void shiftOutRightToLeft(uint8_t dataPin, uint8_t clockPin, uint8_t value)
{
    for (int i = 7; i >= 0; i--)
    {
        gpio_set_level(clockPin, 0);
        gpio_set_level(dataPin, (value >> i) & 1);
        gpio_set_level(clockPin, 1);
    }
}

static int *splitNumber(int number)
{
    int *digits = (int *)malloc(4 * sizeof(int));

    if (!digits)
    {
        ESP_LOGE(TAG, "Memory allocation failed");
        return NULL;
    }

    for (int i = 0; i < 4; i++)
    {
        digits[i] = number % 10;
        number /= 10;
    }

    return digits;
}

static void sendDataToDigit(uint8_t digit_num_gpio, uint8_t digit_num)
{
    gpio_set_level(LATCH_GPIO, 0);
    shiftOutRightToLeft(DATA_GPIO, CLOCK_GPIO, digits_table[digit_num]);
    gpio_set_level(LATCH_GPIO, 1);
}

static void selectDigit(uint8_t digit)
{
    gpio_set_level(DIGIT_4_GPIO, digit == 1 ? 0 : 1);
    gpio_set_level(DIGIT_3_GPIO, digit == 2 ? 0 : 1);
    gpio_set_level(DIGIT_2_GPIO, digit == 3 ? 0 : 1);
    gpio_set_level(DIGIT_1_GPIO, digit == 4 ? 0 : 1);
}

static bool IRAM_ATTR your_isr_handler(void *arg)
{
    BaseType_t high_task_awoken = pdFALSE;
    switch (digit_count)
    {
    case 1:
        selectDigit(1);
        sendDataToDigit(DIGIT_1_GPIO, 2);
        gpio_set_level(DIGIT_1_GPIO, 1);
        break;
    case 2:
        selectDigit(2);
        sendDataToDigit(DIGIT_2_GPIO, 0);
        gpio_set_level(DIGIT_2_GPIO, 1);
        break;
    case 3:
        selectDigit(3);
        sendDataToDigit(DIGIT_3_GPIO, 0);
        gpio_set_level(DIGIT_3_GPIO, 1);
        break;
    case 4:
        selectDigit(4);
        sendDataToDigit(DIGIT_4_GPIO, 4);
        gpio_set_level(DIGIT_4_GPIO, 1);
        break;
    default:
        break;
    }

    digit_count++;

    if (digit_count > 4)
    {
        digit_count = 1;
    }

    return high_task_awoken == pdTRUE;
}

static void example_tg_timer_init(int group, int timer, bool auto_reload, int timer_interval_ms)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = 16,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true,
    }; // default clock source is APB
    timer_init(group, timer, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(group, timer, 0);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(group, timer, timer_interval_ms * (TIMER_SCALE / 1000));
    timer_enable_intr(group, timer);

    timer_isr_callback_add(group, timer, your_isr_handler, NULL, 0);

    timer_start(group, timer);
}

void display_app(int time)
{
    shift_register_gpio_init();
    display_digit_gpio_init();
    example_tg_timer_init(TIMER_GROUP_0, TIMER_0, true, 5);
}