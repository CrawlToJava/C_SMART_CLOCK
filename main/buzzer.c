#include "driver/ledc.h"
#include "driver/timer.h"

#include <stdbool.h>

#include "button.h"
#include "task_common.h"
#include "buzzer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "esp_log.h"

static const char *TAG = "BUZZER";

static ledc_info_t ledc_ch[BUZZER_LED_CHANNEL_NUM];

static TimerHandle_t music_notes_timer = NULL;

// handle for rgb_led_pwm_init
bool g_pwm_init_handle = false;

bool is_timer_paused = false;

bool is_music_start = false;

static uint8_t note_order = 1;

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

#define FIRST_TIMER_DELAY (1000 / buzzer_led_duty_duration[0]) * 1.30

#define NUM_OF_REPEAT_TIMER_FUNC sizeof(buzzer_led_duty_duration) / sizeof(uint8_t)

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
    printf("Timer has stoped\n");
}

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
    else if (xTimer == music_notes_timer)
    {
        printf("Music notes timer has started\n");
    }
}

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

void stop_music(void)
{
    if (g_pwm_init_handle && !is_timer_paused)
    {
        ledc_timer_pause(ledc_ch[0].mode, ledc_ch[0].timer_index);
    }
}

static void play_music(TimerHandle_t xTimer)
{
    if (is_timer_paused)
    {
        ledc_timer_resume(ledc_ch[0].mode, ledc_ch[0].timer_index);
        is_timer_paused = false;
    }
    uint8_t *note_order = (uint8_t *)pvTimerGetTimerID(xTimer);
    MusicState_t musicState;
    if ((*note_order) < NUM_OF_REPEAT_TIMER_FUNC)
    {
        uint32_t duration = 1000 / buzzer_led_duty_duration[(*note_order)];
        uint32_t pauseBetweenNotes = duration * 1.30;
        update_channel_params(buzzer_led_notes_frq[(*note_order)], buzzer_led_duty_duration[(*note_order)]);
        xTimerChangePeriod(music_notes_timer, pdMS_TO_TICKS(pauseBetweenNotes), portMAX_DELAY);
        (*note_order)++;
    }
    else
    {
        musicState.musicState = MUSIC_OFF;
        xQueueSend(queue_music_handle, &musicState, portMAX_DELAY);
    }
}

static void control_music_task(void)
{
    MusicState_t musicState;
    uint8_t *note_order_p = &note_order;
    while (1)
    {
        if (xQueueReceive(queue_music_handle, &musicState.musicState, portMAX_DELAY))
        {
            switch (musicState.musicState)
            {
            case MUSIC_START:
                printf("MUSIC_START case\n");
                musicState.musicState = MUSIC_PLAY;
                xQueueSend(queue_music_handle, &musicState, portMAX_DELAY);
                start_timer(music_notes_timer);
                break;
            case MUSIC_OFF:
                printf("MUSIC_OFF case\n");
                stop_music();
                stop_timer(music_notes_timer);
                is_timer_paused = true;
                (*note_order_p) = 1;
            default:
                break;
            }
        }
    }
    vTaskDelete(NULL);
}

void buzzer_app(void)
{
    if (g_pwm_init_handle != true)
    {
        buzzer_led_pwm_init();
    }
    xTaskCreatePinnedToCore(control_music_task, "control_music_task", BUZZER_APP_TASK_STACK_SIZE, NULL, BUZZER_APP_TASK_PRIORITY, NULL, BUZZER_APP_TASK_CORE_ID);
    music_notes_timer = xTimerCreate("Music notes timer", pdMS_TO_TICKS(FIRST_TIMER_DELAY), pdTRUE, &note_order, play_music);
    ESP_LOGI(TAG, "buzzer app has started");
}