#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

// -----------------------------------------------------------------------------
// Configuration Settings
// -----------------------------------------------------------------------------
#define TACH_GPIO              GPIO_NUM_4        // Input GPIO connected to Fan Tach
#define PULSES_PER_REV         2                 // Standard PC fan = 2 pulses/revolution
#define MIN_PULSE_DELTA_US     3000              // Noise filter: Ignore >10,000 RPM (3ms)
#define STALL_TIMEOUT_US       2000000           // 2 seconds with no pulse = 0 RPM (Fan stopped)
#define EMA_ALPHA              0.2f              // Smoothing factor (0.1 = very smooth, 0.5 = fast response)

static const char *TAG = "FAN_TACH";

// -----------------------------------------------------------------------------
// Global Variables & Locks
// -----------------------------------------------------------------------------
static portMUX_TYPE g_spinlock = portMUX_INITIALIZER_UNLOCKED;

static volatile uint64_t g_last_pulse_time = 0;
static volatile uint64_t g_current_delta_us = 0;
static float             g_smoothed_rpm     = 0.0f;

// -----------------------------------------------------------------------------
// Interrupt Service Routine (ISR)
// Runs instantly on every falling edge of the Tachometer signal
// -----------------------------------------------------------------------------
static void IRAM_ATTR tach_isr_handler(void* arg)
{
    uint64_t now = esp_timer_get_time();
    uint64_t delta = now - g_last_pulse_time;

    // Debounce / Noise Filter: Ignore spikes faster than max physical speed
    if (delta > MIN_PULSE_DELTA_US) {
        taskENTER_CRITICAL_ISR(&g_spinlock);
        g_current_delta_us = delta;
        g_last_pulse_time = now;
        taskEXIT_CRITICAL_ISR(&g_spinlock);
    }
}

// -----------------------------------------------------------------------------
// RPM Calculation Function
// Safe to call from any FreeRTOS task or callback loop
// -----------------------------------------------------------------------------
uint32_t get_fan_rpm(void)
{
    uint64_t now = esp_timer_get_time();
    uint64_t last_pulse;
    uint64_t delta;

    // Safely copy ISR variables within a critical section
    taskENTER_CRITICAL(&g_spinlock);
    last_pulse = g_last_pulse_time;
    delta = g_current_delta_us;
    taskEXIT_CRITICAL(&g_spinlock);

    // 1. Check for fan stall (zero-speed timeout)
    if ((now - last_pulse) > STALL_TIMEOUT_US || delta == 0) {
        g_smoothed_rpm = 0.0f;
        return 0;
    }

    // 2. Calculate raw instantaneous RPM
    // Formula: RPM = (60s * 1,000,000 us) / (Delta_us * PPR)
    uint32_t raw_rpm = (uint32_t)((60000000ULL / PULSES_PER_REV) / delta);

    // 3. Apply Exponential Moving Average (EMA) to smooth low-speed bounce
    if (g_smoothed_rpm == 0.0f) {
        g_smoothed_rpm = (float)raw_rpm; // Cold start initialization
    } else {
        g_smoothed_rpm = (EMA_ALPHA * (float)raw_rpm) + ((1.0f - EMA_ALPHA) * g_smoothed_rpm);
    }

    return (uint32_t)(g_smoothed_rpm + 0.5f); // Return rounded integer
}

// -----------------------------------------------------------------------------
// Main Application Loop
// -----------------------------------------------------------------------------
void app_main(void)
{
    ESP_LOGI(TAG, "Initializing Fan Speed Monitor...");

    // 1. Configure the GPIO Pin for Tachometer input
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,          // Trigger on Falling Edge
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << TACH_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,        // Internal Pull-Up for Open-Drain signal
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    // 2. Install ISR Service and attach Handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(TACH_GPIO, tach_isr_handler, NULL);

    ESP_LOGI(TAG, "Tachometer Reader active on GPIO %d.", TACH_GPIO);

    // 3. Continuously read and print fan speed
    while (1) {
        uint32_t rpm = get_fan_rpm();
        ESP_LOGI(TAG, "Fan Speed: %" PRIu32 " RPM", rpm);

        // Delay interval here controls log frequency ONLY. 
        // It does NOT affect pulse capture accuracy!
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}