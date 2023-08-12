#include "button.h"
#include "task_common.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#include "esp_log.h"

static bool button1_pressed = false; // Button gpio state
static bool button2_pressed = false; // Button gpio state

static const char *TAG = "Button";

// Button gpio`s initialization
static void button_gpio_init(void)
{
    gpio_config_t button_gpio_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = ((1ULL << BUTTON_1_GPIO) | (1ULL << BUTTON_2_GPIO)),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE};

    ESP_ERROR_CHECK(gpio_config(&button_gpio_config));

    ESP_LOGI(TAG, "Button gpio`s initialized successfully");
}

/**
 * Func read the state of button gpio`s once per 300 ms
 * @param xTimer freertos timer
 */
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

/**
 * Func allow to change display`s num using buttons
 * @param pvParameters pointer on display num
 */
static void update_display_num_task(void *pvParameters)
{
    uint32_t *numToDisplay = (uint32_t *)pvParameters;
    while (1)
    {
        if (button1_pressed)
        {
            button1_pressed = false;
            (*numToDisplay)++;
        }

        if (button2_pressed && numToDisplay > 0)
        {
            button2_pressed = false;
            (*numToDisplay)--;
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

/**
 * Starts update display num task
 * @param displayNum pointer to the display num
 */
void button_app(uint32_t *displayNum)
{
    button_gpio_init();

    xTaskCreatePinnedToCore(update_display_num_task, "update_display_num_task", BUTTON_APP_TASK_STACK_SIZE, displayNum, BUTTON_APP_TASK_PRIORITY, NULL, BUTTON_APP_TASK_CORE_ID);

    TimerHandle_t button_timer = xTimerCreate("button_timer", pdMS_TO_TICKS(DELAY_PERIOD), pdTRUE, NULL, button_handler);
    xTimerStart(button_timer, 0);

    ESP_LOGI(TAG, "button_app has started");
}