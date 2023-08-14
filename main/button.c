#include "button.h"
#include "task_common.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_log.h"

static ButtonState_e increase_num_button_pressed = BUTTON_NOT_PRESSED; // Button gpio state
static ButtonState_e decrease_num_button_pressed = BUTTON_NOT_PRESSED; // Button gpio state
static ButtonState_e start_timer_button_pressed = BUTTON_NOT_PRESSED;  // Button gpio state
static ButtonState_e reset_timer_button_pressed = BUTTON_NOT_PRESSED;  // Button gpio state

static TimerState_e is_timer_has_started = TIMER_NOT_STARTED; // Timer state

static TimerHandle_t timer_start_xTimer = NULL;

static const char *TAG = "Button";

/**
 * Func starts a timer
 *@param xTimer timer to start
 */
static void start_timer(TimerHandle_t xTimer)
{
    if (xTimerStart(xTimer, portMAX_DELAY) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to start timer");
    }
    else
    {
        printf("Timer has started\n");
    }
}

/**
 * Func stops a timer
 *@param xTimer timer to stop
 */
static void stop_timer(TimerHandle_t xTimer)
{
    if (xTimerStop(xTimer, portMAX_DELAY) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to stop timer");
    }
    else
    {
        is_timer_has_started = TIMER_NOT_STARTED;
        printf("Timer has stoped\n");
    }
}

// Button gpio`s initialization
static void button_gpio_init(void)
{
    gpio_config_t button_gpio_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = BUTTON_GPIO_PIN_MASK,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE};

    ESP_ERROR_CHECK(gpio_config(&button_gpio_config));

    ESP_LOGI(TAG, "Button gpio`s initialized successfully");
}

/**
 * Func decrease display num
 * @param xTimer freertos timer
 */
static void start_timer_handler(TimerHandle_t xTimer)
{
    uint32_t *numToDisplay = (uint32_t *)pvTimerGetTimerID(xTimer);
    if ((*numToDisplay) > 0 && is_timer_has_started == TIMER_STARTED)
    {
        (*numToDisplay)--;
    }
    else
    {
        stop_timer(xTimer);
    }
}

/**
 * Func read the state of button gpio`s once per 300 ms
 * @param xTimer freertos timer
 */
static void button_handler(TimerHandle_t xTimer)
{
    if (gpio_get_level(INCREASE_NUM_BUTTON_GPIO) == 0)
    {
        increase_num_button_pressed = BUTTON_PRESSED;
    }

    if (gpio_get_level(DECREASE_NUM_BUTTON_GPIO) == 0)
    {
        decrease_num_button_pressed = BUTTON_PRESSED;
    }
    if (gpio_get_level(START_TIMER_BUTTON_GPIO) == 0 && is_timer_has_started == TIMER_NOT_STARTED)
    {
        start_timer_button_pressed = BUTTON_PRESSED;
    }

    if (gpio_get_level(RESET_TIMER_BUTTON_GPIO) == 0)
    {
        reset_timer_button_pressed = BUTTON_PRESSED;
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
        if (increase_num_button_pressed == BUTTON_PRESSED && (*numToDisplay) < 9999 && is_timer_has_started == TIMER_NOT_STARTED)
        {
            increase_num_button_pressed = BUTTON_NOT_PRESSED;
            (*numToDisplay)++;
        }

        if (decrease_num_button_pressed == BUTTON_PRESSED && (*numToDisplay) > 0 && is_timer_has_started == TIMER_NOT_STARTED)
        {
            decrease_num_button_pressed = BUTTON_NOT_PRESSED;
            (*numToDisplay)--;
        }

        if (start_timer_button_pressed == BUTTON_PRESSED && (*numToDisplay) > 0)
        {
            start_timer_button_pressed = BUTTON_NOT_PRESSED;
            is_timer_has_started = TIMER_STARTED;
            start_timer(timer_start_xTimer);
            printf("Start timer button is pressed\n");
        }

        if (reset_timer_button_pressed == BUTTON_PRESSED)
        {
            reset_timer_button_pressed = BUTTON_NOT_PRESSED;
            printf("Reset timer button is pressed\n");
            (*numToDisplay) = 0;
            if (is_timer_has_started == TIMER_STARTED)
            {
                is_timer_has_started = TIMER_NOT_STARTED;
            }
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

    TimerHandle_t button_timer = xTimerCreate("button_timer", pdMS_TO_TICKS(BUTTON_CHECK_STATE_DELAY_PERIOD), pdTRUE, NULL, button_handler);
    timer_start_xTimer = xTimerCreate("Timer xTimer", pdMS_TO_TICKS(START_TIMER_DELAY_PERIOD), pdTRUE, displayNum, start_timer_handler);
    start_timer(button_timer);
    ESP_LOGI(TAG, "button_app has started");
}