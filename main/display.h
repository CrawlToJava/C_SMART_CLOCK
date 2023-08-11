#ifndef MAIN_DISPLAY_H
#define MAIN_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#define DATA_GPIO 21  // GPIO to control shift register data pin
#define LATCH_GPIO 19 // GPIO to control shift register latch pin
#define CLOCK_GPIO 18 // GPIO to control shift register clock pin

#define DIGIT_1_GPIO 32 // GPIO to control fisrt digit
#define DIGIT_2_GPIO 33 // GPIO to control second digit
#define DIGIT_3_GPIO 25 // GPIO to control third digit
#define DIGIT_4_GPIO 26 // GPIO to control fourth digit

// Timer config structure
typedef struct timer_config_params
{
    uint8_t timerGroup; // Timer group
    uint8_t timerIndex; // Timer index
    bool autoReload;    // allow/disallow autoreload
} timer_config_params_t;

// Shift register pin`s config structure
typedef struct shift_register_config_gpio
{
    uint8_t dataGpio;  // Shift register data pin gpio
    uint8_t clocKGpio; // Shift register clock pin gpio
    uint8_t latchGpio; // Shift register latch gpio
} shift_register_config_gpio_t;

// Seven segment display digits gpio`s config structure
typedef struct display_digit_config_gpio
{
    uint8_t digitOneGpio;   // Display digit one gpio
    uint8_t digitTwoGpio;   // Display digit two gpio
    uint8_t digitThreeGpio; // Display digit three gpio
    uint8_t digitFourGpio;  // Display digit four gpio
} display_digit_config_gpio_t;

#define TIMER_DIVIDER (16)                                     //  Hardware timer clock divider
#define TIMER_SCALE_MS ((APB_CLK_FREQ / TIMER_DIVIDER) / 1000) // convert counter value to milliseconds

void display_app(uint8_t timerGroup, uint8_t timerIndex, uint32_t *displayNum, uint32_t delayPeriod);

#endif