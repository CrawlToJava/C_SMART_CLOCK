#ifndef MAIN_BUTTON_H
#define MAIN_BUTTON_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"

#define INCREASE_NUM_BUTTON_GPIO 17 // GPIO to control a button that increase timer value
#define DECREASE_NUM_BUTTON_GPIO 16 // GPIO to control a button that decrease timer value
#define START_TIMER_BUTTON_GPIO 23  // GPIO to control a button that start a timer
#define RESET_TIMER_BUTTON_GPIO 22  // GPIO to control a button that  reset a timer

extern QueueHandle_t queue_music_handle;

/**
 * Pin mask for the button gpio`s
 */
#define BUTTON_GPIO_PIN_MASK ((1ULL << INCREASE_NUM_BUTTON_GPIO) | (1ULL << DECREASE_NUM_BUTTON_GPIO) | (1ULL << START_TIMER_BUTTON_GPIO) | (1 << RESET_TIMER_BUTTON_GPIO))

/**
 *Timer delay periods
 */
#define BUTTON_CHECK_STATE_DELAY_PERIOD 100 // Timer period for checking button gpio`s status
#define START_TIMER_DELAY_PERIOD 1000

typedef enum
{
    BUTTON_NOT_PRESSED,
    BUTTON_PRESSED
} ButtonState_e;

typedef enum
{
    TIMER_STARTED,
    TIMER_NOT_STARTED
} TimerState_e;

void button_app(uint32_t *displayNum);

#endif