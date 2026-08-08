#include "tach_sensor.h"
#include "hvac_states.h"
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG = "TACH_SENSOR";

extern QueueHandle_t hvac_queue;

// Global struct instances for linker resolution
TachSensor tach_sensor = {0};

// Global PCNT handle for sensor_manager start/stop control
pcnt_unit_handle_t pcnt_unit = NULL;

static bool s_fan_ok_sent = false; // State flag to avoid queue flooding

void tach_sensor_init(struct TachSensor* self) {
    if (!self) return;

    self->init = tach_sensor_init;
    self->read = tach_sensor_read;
    self->fan_rpm = 0;

    // Prevent re-initialization memory leak
    if (self->pcnt_unit != NULL) {
        ESP_LOGW(TAG, "Tachometer PCNT unit already initialized.");
        return;
    }

    // Configure Pulse Counter (PCNT) Unit
    pcnt_unit_config_t unit_config = {
        .high_limit = 10000,
        .low_limit  = -1,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &self->pcnt_unit));

    // Store in global handle for sensor_manager compatibility
    pcnt_unit = self->pcnt_unit;

    // Glitch Filter (Suppress high-frequency noise on tach wire)
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(self->pcnt_unit, &filter_config));

    // Configure Channel
    pcnt_chan_config_t chan_config = {
        .edge_gpio_num  = FAN_TACH_PIN,
        .level_gpio_num = -1,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(self->pcnt_unit, &chan_config, &self->pcnt_chan));

    // Enable internal pull-up on Tach input GPIO
    gpio_set_pull_mode(FAN_TACH_PIN, GPIO_PULLUP_ONLY);

    // Count on rising edge
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        self->pcnt_chan, 
        PCNT_CHANNEL_EDGE_ACTION_INCREASE, 
        PCNT_CHANNEL_EDGE_ACTION_HOLD
    ));

    ESP_ERROR_CHECK(pcnt_unit_enable(self->pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(self->pcnt_unit));
    
    s_fan_ok_sent = false;
    ESP_LOGI(TAG, "Tachometer Pulse Counter initialized on GPIO %d.", FAN_TACH_PIN);
}


void tach_sensor_read(struct TachSensor* self) {
    if (!self || !self->pcnt_unit) return;

    int pulse_count = 0;
    ESP_ERROR_CHECK(pcnt_unit_get_count(self->pcnt_unit, &pulse_count));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(self->pcnt_unit)); // Reset counter for next window

    // Calculate RPM: RPM = (Pulses / Pulses_Per_Rev) * (60s / Sample_Period_s)
    float pulses_per_second = (float)pulse_count * (1000.0f / (float)DEFAULT_SAMPLE_PERIOD_MS);
    uint32_t rpm = (uint32_t)((pulses_per_second / (float)PULSES_PER_REV) * 60.0f);
    
    self->fan_rpm = rpm;
    ESP_LOGD(TAG, "Current Fan Speed: %u RPM (Pulses: %d)", self->fan_rpm, pulse_count);

    // Trigger state machine once when fan reaches required airflow speed
    if (self->fan_rpm >= 4000) {
        if (!s_fan_ok_sent) {
            hvac_cmd_t cmd = CMD_FAN_OK;
            if (hvac_queue != NULL && xQueueSend(hvac_queue, &cmd, pdMS_TO_TICKS(10)) == pdPASS) {
                s_fan_ok_sent = true; // Block duplicate commands until speed drops below threshold
            }
        }
    } else {
        s_fan_ok_sent = false; // Reset threshold trigger
    }
}