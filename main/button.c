#include "button.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#include "esp_log.h"

static bool button1_pressed = false;
static bool button2_pressed = false;

static const char *TAG = "Button";

static void button_gpio_init(void)
{
    gpio_config_t button_gpio_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = ((1ULL << BUTTON_1_GPIO) | (1ULL << BUTTON_2_GPIO)),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE};

    ESP_ERROR_CHECK(gpio_config(&button_gpio_config));
}

static void button_handler(TimerHandle_t xTimer)
{
    if (gpio_get_level(BUTTON_1_GPIO) == 0)
    {
        button1_pressed = true;
    }

    if (gpio_get_level(BUTTON_2_GPIO) == 0)
    {
        button2_pressed = true;
    }
}

static void update_display_task(void *pvParameters)
{
    uint32_t *numToDisplay = (uint32_t *)pvParameters;
    while (1)
    {
        if (button1_pressed)
        {
            button1_pressed = false;
            (*numToDisplay)++;
            printf("update_display_task: button 16 is pressed, numToDisplay: %ld\n", *numToDisplay);
        }

        if (button2_pressed && numToDisplay > 0)
        {
            button2_pressed = false;
            (*numToDisplay)--;
            printf("update_display_task: button 17 is pressed, numToDisplay: %ld\n", *numToDisplay);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void button_app(uint32_t *displayNum)
{
    button_gpio_init();
    xTaskCreate(update_display_task, "update_display_task", 2048, displayNum, 1, NULL);
    TimerHandle_t button_timer = xTimerCreate("button_timer", pdMS_TO_TICKS(DELAY_PERIOD), pdTRUE, NULL, button_handler);
    xTimerStart(button_timer, 0);
}