#include "hvac_hardware.h"
#include "driver.gpio.h"
#include "esp_log.h"

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
}