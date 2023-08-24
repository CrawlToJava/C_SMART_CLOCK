#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "button.h"
#include "buzzer.h"

uint32_t num = 0;
timer_for_display_config_params_t timer_config = {
    .autoReload = true,
    .timerGroup = 0,
    .timerIndex = 0};

void app_main()
{
    button_app(&num);
    display_app(&timer_config, &num, 5);
    buzzer_app();
}