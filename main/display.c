#include "display.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <stdio.h>
#include <stdlib.h>

static char *TAG = "DISPLAY";

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
    0b11011110,// 9
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
        .pin_bit_mask = ((1ULL << DIGIT_1_GPIO) | (1ULL << DIGIT_2_GPIO) | (1ULL << DIGIT_3_GPIO) | (1ULL << DIGIT_4_GPIO)),
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
    if (digit_num > 3 || digit_num < 0)
    {
        ESP_LOGE(TAG, "digit num is larger then 3 or less then 0");
    }
    gpio_set_level(digit_num_gpio, 0);
    gpio_set_level(LATCH_GPIO, 0);
    shiftOutRightToLeft(DATA_GPIO, CLOCK_GPIO, digits_table[digit_num]);
    gpio_set_level(LATCH_GPIO, 1);
}

static void displayTime(int number)
{

    int *digits = splitNumber(number);

    gpio_set_level(DIGIT_1_GPIO, 0);
    sendDataToDigit(DIGIT_1_GPIO, digits[0]);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(DIGIT_1_GPIO, 1);

    gpio_set_level(DIGIT_2_GPIO, 0);
    sendDataToDigit(DIGIT_2_GPIO, digits[1]);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(DIGIT_2_GPIO, 1);

    gpio_set_level(DIGIT_3_GPIO, 0);
    sendDataToDigit(DIGIT_3_GPIO, digits[2]);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(DIGIT_3_GPIO, 1);

    gpio_set_level(DIGIT_4_GPIO, 0);
    sendDataToDigit(DIGIT_4_GPIO, digits[3]);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(DIGIT_4_GPIO, 1);

    free(digits);
}

static void time_task(void)
{
    while (1)
    {
        displayTime(9111);
    }
}

void display_app(void)
{
    shift_register_gpio_init();
    display_digit_gpio_init();
    xTaskCreatePinnedToCore(&time_task, "displayTime", 4096, NULL, 5, NULL, 0);
}