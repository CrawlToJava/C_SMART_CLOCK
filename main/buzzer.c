#include "buzzer.h"
#include "driver/ledc.h"
#include "driver/timer.h"

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static ledc_info_t ledc_ch[BUZZER_LED_CHANNEL_NUM];

// handle for rgb_led_pwm_init
bool g_pwm_init_handle = false;

// Music`s notes frq
static const uint16_t buzzer_led_notes_frq[] = {
    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
    NOTE_C5, NOTE_D5, NOTE_B4, NOTE_B4, REST,
    NOTE_A4, NOTE_G4, NOTE_A4, REST,

    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
    NOTE_C5, NOTE_D5, NOTE_B4, NOTE_B4, REST,
    NOTE_A4, NOTE_G4, NOTE_A4, REST,

    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
    NOTE_A4, NOTE_C5, NOTE_D5, NOTE_D5, REST,
    NOTE_D5, NOTE_E5, NOTE_F5, NOTE_F5, REST,
    NOTE_E5, NOTE_D5, NOTE_E5, NOTE_A4, REST,

    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
    NOTE_D5, NOTE_E5, NOTE_A4, REST,
    NOTE_A4, NOTE_C5, NOTE_B4, NOTE_B4, REST,
    NOTE_C5, NOTE_A4, NOTE_B4, REST,

    NOTE_A4, NOTE_A4,
    // Repeat of first part
    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
    NOTE_C5, NOTE_D5, NOTE_B4, NOTE_B4, REST,
    NOTE_A4, NOTE_G4, NOTE_A4, REST,

    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
    NOTE_C5, NOTE_D5, NOTE_B4, NOTE_B4, REST,
    NOTE_A4, NOTE_G4, NOTE_A4, REST,

    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
    NOTE_A4, NOTE_C5, NOTE_D5, NOTE_D5, REST,
    NOTE_D5, NOTE_E5, NOTE_F5, NOTE_F5, REST,
    NOTE_E5, NOTE_D5, NOTE_E5, NOTE_A4, REST,

    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
    NOTE_D5, NOTE_E5, NOTE_A4, REST,
    NOTE_A4, NOTE_C5, NOTE_B4, NOTE_B4, REST,
    NOTE_C5, NOTE_A4, NOTE_B4, REST,
    // End of Repeat

    NOTE_E5, REST, REST, NOTE_F5, REST, REST,
    NOTE_E5, NOTE_E5, REST, NOTE_G5, REST, NOTE_E5, NOTE_D5, REST, REST,
    NOTE_D5, REST, REST, NOTE_C5, REST, REST,
    NOTE_B4, NOTE_C5, REST, NOTE_B4, REST, NOTE_A4,

    NOTE_E5, REST, REST, NOTE_F5, REST, REST,
    NOTE_E5, NOTE_E5, REST, NOTE_G5, REST, NOTE_E5, NOTE_D5, REST, REST,
    NOTE_D5, REST, REST, NOTE_C5, REST, REST,
    NOTE_B4, NOTE_C5, REST, NOTE_B4, REST, NOTE_A4};

// Duty duration
static const uint8_t buzzer_led_duty_duration[] = {
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8,

    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8,

    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 8, 4, 8,

    8, 8, 4, 8, 8,
    4, 8, 4, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 4,

    4, 8,
    // Repeat of First Part
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8,

    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8,

    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 8, 4, 8,

    8, 8, 4, 8, 8,
    4, 8, 4, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 4,
    // End of Repeat

    4, 8, 4, 4, 8, 4,
    8, 8, 8, 8, 8, 8, 8, 8, 4,
    4, 8, 4, 4, 8, 4,
    8, 8, 8, 8, 8, 2,

    4, 8, 4, 4, 8, 4,
    8, 8, 8, 8, 8, 8, 8, 8, 4,
    4, 8, 4, 4, 8, 4,
    8, 8, 8, 8, 8, 2};

// PWM initialization
static void buzzer_led_pwm_init(void)
{
    uint8_t buzzer_ch;

    ledc_ch[0].channel = LEDC_CHANNEL_0;
    ledc_ch[0].gpio = BUZZER_LED_GPIO;
    ledc_ch[0].mode = LEDC_HIGH_SPEED_MODE;
    ledc_ch[0].timer_index = LEDC_TIMER_1;
    ledc_ch[0].duty_resolution = LEDC_TIMER_10_BIT;

    ledc_timer_config_t timer_config_info = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = NOTE_CS4};

    ledc_timer_config(&timer_config_info);

    // Configure chanells
    for (buzzer_ch = 0; buzzer_ch < BUZZER_LED_CHANNEL_NUM; buzzer_ch++)
    {
        ledc_channel_config_t channel_config = {
            .channel = ledc_ch[buzzer_ch].channel,
            .duty = 0,
            .hpoint = 0,
            .gpio_num = ledc_ch[buzzer_ch].gpio,
            .intr_type = LEDC_INTR_DISABLE,
            .speed_mode = ledc_ch[buzzer_ch].mode,
            .timer_sel = ledc_ch[buzzer_ch].timer_index};

        ledc_channel_config(&channel_config);
    }

    g_pwm_init_handle = true;
}

// Changes the frq and duty duration params
static void update_channel_params(uint16_t buzzer_led_note_frq, uint16_t buzzer_led_duty_duration)
{
    for (int i = 0; i < BUZZER_LED_CHANNEL_NUM; i++)
    {
        ledc_set_duty(ledc_ch[i].mode, ledc_ch[i].channel, buzzer_led_duty_duration);
        ledc_update_duty(ledc_ch[i].mode, ledc_ch[i].channel);
        ledc_set_freq(ledc_ch[i].mode, ledc_ch[i].timer_index, buzzer_led_note_frq);
    }
}

void play_music(void)
{
    while (true)
    {
        uint8_t size = sizeof(buzzer_led_duty_duration) / sizeof(uint8_t);
        if (g_pwm_init_handle != true)
        {
            buzzer_led_pwm_init();
        }

        for (int note = 0; note < size; note++)
        {
            uint32_t duration = 1000 / buzzer_led_duty_duration[note];
            update_channel_params(buzzer_led_notes_frq[note], duration);
            uint32_t pauseBetweenNotes = duration * 1.30;
            vTaskDelay(pdMS_TO_TICKS(pauseBetweenNotes));
        }
    }
}