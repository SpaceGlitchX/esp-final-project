#include "hvac_hardware.h"
#include "driver.gpio.h"
#include "esp_log.h"

/**
 * HVAC_HARDWARE_C
 * Implements setup functions and timer callbacks for heating startup stages
 */
QueueHandle_t hvac_queue = NULL;
TimerHandle_t flame_proving_timer = NULL;
TimerHandle_t fan_warmup_timer = NULL;
TimerHandle_t tach_window_timer = NULL;

static void install_event(hvac_cmd_t cmd) {
    if (hvac_queue != NULL) {
        xQueueSend(hvac_queue, &cmd, portMAX_DELAY);
    }
}

static void flame_proving_timeout_callback(TimerHandle_t xTimer) {
    install_event(CMD_FLAME_TIMEOUT);
}

static void warmup_done_callback(TimerHandle_t xTimer) {
    install_event(CMD_WARMUP_DONE);
}

static void tach_timeout_callback(TimerHandle_t xTimer) {
    install_event(CMD_TACH_TIMEOUT);
}

void init_hvac_hardware(void) {

    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << HEATER_PIN) | (1ULL << FAN_PIN),
        .mode = GPIO_MODE_OUPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE
    };
    gpio(&io_config);

    set_fan_state(0);
    set_heater_state(0);

    // Initialize timers
    flame_proving_timer = xTimerCreate("FlameTimer", pdMS_TO_TICKS(5000), pdFALSE, NULL, flame_proving_timeout_callback);
    fan_warmup_timer = xTimerCreate("WarmupTimer", pdMS_TO_TICKS(10000), pdFALSE, NULL, warmup_done_callback);
    tach_window_timer = xTimerCreate("TachTimeout", pdMS_TO_TICKS(1000), pdFALSE, NULL, tach_timeout_callback);
}

void set_heater_state(int level) {
    gpio_set_level(HEATER_PIN, level);
}
void set_fan_state(int level) {
    gpio_set_level(FAN_PIN, level);
}