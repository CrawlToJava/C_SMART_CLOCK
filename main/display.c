#include "display.h"

#include "driver/gpio.h"
#include "driver/timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_log.h"

#include "button.h"

#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "DISPLAY"; // Log tag

static uint8_t digit_count = 1; // Increment value for isr_display_num_func logic

static timer_for_display_config_params_t timer_config_params; // Timer config

static uint8_t digitOne = 0;   // first digit value of seven segment display
static uint8_t digitTwo = 0;   // second digit value of seven segment display
static uint8_t digitThree = 0; // third digit value of seven segment display
static uint8_t digitFour = 0;  // fourth digit value of seven segment display

// Digits
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

// Shitf register DATA, CLOCK, LATCH gpio`s initialization
static void shift_register_gpio_init(void)
{
    gpio_config_t shift_register_gpio = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = ((1ULL << DATA_GPIO) | (1ULL << LATCH_GPIO) | (1ULL << CLOCK_GPIO)),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE};

    ESP_ERROR_CHECK(gpio_config(&shift_register_gpio));

    ESP_LOGI(TAG, "Shift register gpio`s initialized successfully");
}

// Seven segment digits gpio`s initialization
static void display_digit_gpio_init(void)
{
    gpio_config_t display_digit_gpio = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = ((1ULL << DIGIT_1_GPIO) | (1ULL << DIGIT_2_GPIO) | (1ULL << DIGIT_3_GPIO) | (1ULL << DIGIT_4_GPIO)),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE};

    ESP_ERROR_CHECK(gpio_config(&display_digit_gpio));

    ESP_LOGI(TAG, "Display gpio`s initialized successfully");
}

// Func send bit`s data to the shift register
static void shiftOutRightToLeft(uint8_t dataPin, uint8_t clockPin, uint8_t value)
{
    for (int i = 7; i >= 0; i--)
    {
        gpio_set_level(clockPin, 0);
        gpio_set_level(dataPin, (value >> i) & 1);
        gpio_set_level(clockPin, 1);
    }
}

/**
 * Func split the number
 * @param number to split
 */
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

/**
 * Func send data from shift register to seven segment display
 * @param digit_num that`ll show on the seven segment digit
 */
static void sendDataToDigit(uint8_t digit_num)
{
    gpio_set_level(LATCH_GPIO, 0);
    shiftOutRightToLeft(DATA_GPIO, CLOCK_GPIO, digits_table[digit_num]);
    gpio_set_level(LATCH_GPIO, 1);
}

/**
 * Select digit func
 * @param digit_index index of digit on the seven-segment display
 */
static void selectDigit(uint8_t digit_index)
{
    gpio_set_level(DIGIT_4_GPIO, digit_index == 1 ? 0 : 1);
    gpio_set_level(DIGIT_3_GPIO, digit_index == 2 ? 0 : 1);
    gpio_set_level(DIGIT_2_GPIO, digit_index == 3 ? 0 : 1);
    gpio_set_level(DIGIT_1_GPIO, digit_index == 4 ? 0 : 1);
}

static bool IRAM_ATTR isr_display_digits_nums_func(void *arg)
{
    uint32_t numToDisplay = *((uint32_t *)arg);
    int *digits = splitNumber(numToDisplay);
    digitOne = digits[0];
    digitTwo = digits[1];
    digitThree = digits[2];
    digitFour = digits[3];
    BaseType_t high_task_awoken = pdFALSE;
    switch (digit_count)
    {
    case 1:
        selectDigit(4);
        sendDataToDigit(digitOne);
        gpio_set_level(DIGIT_4_GPIO, 1);
        break;
    case 2:
        selectDigit(3);
        sendDataToDigit(digitTwo);
        gpio_set_level(DIGIT_3_GPIO, 1);
        break;
    case 3:
        selectDigit(2);
        sendDataToDigit(digitThree);
        gpio_set_level(DIGIT_2_GPIO, 1);
        break;
    case 4:
        selectDigit(1);
        sendDataToDigit(digitFour);
        gpio_set_level(DIGIT_1_GPIO, 1);
        break;
    default:
        break;
    }

    digit_count++;

    if (digit_count > 4)
    {
        digit_count = 1;
    }
    free(digits);
    return high_task_awoken == pdTRUE;
}

/**
 * Timer init func for seven segmnet display delay generation
 * @param timer_group timer group index
 * @param timer timer index
 * @param auto_reload allow/disallow autoreload
 * @param timer_interval_ms interrupt interval
 */
static void timer_init_for_display_num(uint8_t timer_group, uint8_t timer, bool auto_reload, uint32_t timer_interval_ms, uint32_t *numToDisplay)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = auto_reload,
    }; // default clock source is APB
    timer_init(timer_group, timer, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(timer_group, timer, 0);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(timer_group, timer, timer_interval_ms * TIMER_SCALE_MS);
    timer_enable_intr(timer_group, timer);

    timer_isr_callback_add(timer_group, timer, isr_display_digits_nums_func, numToDisplay, 0);

    timer_start(timer_group, timer);
}

void display_app(timer_for_display_config_params_t *pTimerConfig, uint32_t *displayNum, uint32_t delayPeriod)
{

    shift_register_gpio_init();
    display_digit_gpio_init();
    timer_init_for_display_num(pTimerConfig->timerGroup, pTimerConfig->timerIndex, pTimerConfig->autoReload, delayPeriod, displayNum);

    ESP_LOGI(TAG, "display_app has started");
}