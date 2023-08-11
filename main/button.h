#ifndef MAIN_BUTTON_H
#define MAIN_BUTTON_H

#include <stdint.h>

#define BUTTON_1_GPIO 17
#define BUTTON_2_GPIO 16

#define DELAY_PERIOD 300

void button_app(uint32_t *displayNum);

#endif