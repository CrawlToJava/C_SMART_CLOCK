#ifndef MAIN_BUTTON_H
#define MAIN_BUTTON_H

#include <stdint.h>

#define BUTTON_1_GPIO 17 // GPIO to control a button
#define BUTTON_2_GPIO 16 // GPIO to control a button

#define DELAY_PERIOD 300 // Timer period

void button_app(uint32_t *displayNum);

#endif