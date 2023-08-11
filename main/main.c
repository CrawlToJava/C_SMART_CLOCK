#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "button.h"

uint32_t num = 5;

void app_main()
{
    button_app(&num);
    display_app(0, 0, &num, 5);
    while (1)
    {
        printf("app_main: num %ld\n", num);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}