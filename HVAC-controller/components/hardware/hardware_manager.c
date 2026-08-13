#include "hardware_manager.h"
#include "hvac_states.h"
#include "hvac_state_machine.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "HVAC_HW";

// Global FreeRTOS Handles
TimerHandle_t flame_proving_timer = NULL;
TimerHandle_t fan_warmup_timer   = NULL;
TimerHandle_t tach_window_timer  = NULL;

// Post event to FSM queue 
static void install_event(hvac_cmd_t cmd) {
    if (hvac_queue != NULL) {
        if (xQueueSend(hvac_queue, &cmd, 0) != pdPASS) {
            ESP_LOGE(TAG, "HVAC Event Queue Full! Dropped CMD: %d", cmd);
        }
    }
}

// Callback function for flame proving timeout
static void flame_proving_timeout_callback(TimerHandle_t xTimer) {
    (void)xTimer;
    ESP_LOGW(TAG, "Safety Timer Expired: Flame Proving Timeout");
    install_event(CMD_FLAME_TIMEOUT);
}

// Callback function for fan warmup completion
static void warmup_done_callback(TimerHandle_t xTimer) {
    (void)xTimer;
    ESP_LOGI(TAG, "Safety Timer Expired: Warmup Complete");
    install_event(CMD_WARMUP_DONE);
}

// Callback function for tachometer timeout
static void tach_timeout_callback(TimerHandle_t xTimer) {
    (void)xTimer;
    ESP_LOGW(TAG, "Safety Timer Expired: Tachometer RPM Timeout");
    install_event(CMD_TACH_TIMEOUT);
}

// Initialize the HVAC hardware, GPIO pins, and FreeRTOS safety timers
extern void init_hvac_hardware(void) {

    gpio_set_level(HEATER_PIN, 0);
    gpio_set_level(FAN_PIN, 0);

    // Configure Relay Control Output Pins
    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << HEATER_PIN) | (1ULL << FAN_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&io_config);

    // Initialize One-Shot Timers
    flame_proving_timer = xTimerCreate("FlameTimer",  pdMS_TO_TICKS(1000), pdFALSE, NULL, flame_proving_timeout_callback);
    fan_warmup_timer    = xTimerCreate("WarmupTimer", pdMS_TO_TICKS(1000), pdFALSE, NULL, warmup_done_callback);
    tach_window_timer   = xTimerCreate("TachTimeout", pdMS_TO_TICKS(1000), pdFALSE, NULL, tach_timeout_callback);

    if (flame_proving_timer == NULL || fan_warmup_timer == NULL || tach_window_timer == NULL) {
        ESP_LOGE(TAG, "CRITICAL: Failed to allocate memory for FreeRTOS HVAC safety timers!");
    } else {
        ESP_LOGI(TAG, "HVAC Hardware and Safety Timers Initialized Successfully.");
    }
}

// Getter and Setter functions
void set_heater_state(int level) {
    gpio_set_level(HEATER_PIN, level);
}

void set_fan_state(int level) {
    gpio_set_level(FAN_PIN, level);
}

extern int get_fan_state(void) {
    return gpio_get_level(FAN_PIN);
}

extern int get_heater_state(void) {
    return gpio_get_level(HEATER_PIN);
}