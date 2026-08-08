#include "tach_sensor.h"

void tach_sensor_init(struct TachSensor* self) {
    if (!self) return;

    self->init = tach_sensor_init;
    self->read = tach_sensor_read;
    self->pcnt_unit = NULL;
    self->pcnt_chan = NULL;

    self->fan_rpm = 0.0;

    pcnt_unit_config_t unit_config = {
        .high_limit = 10000,
        .low_limit = -1,
    };
    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = TACH_GPIO_PIN,
        .level_gpio_num = -1,
    };
    pcnt_channel_handle_t pcnt_chan = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_chan));

    // Enable internal pull-up on the GPIO pin
    gpio_set_pull_mode(TACH_GPIO_PIN, GPIO_PULLUP_ONLY);

    // Count on rising edge
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));

    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
}

void tach_sensor_read(struct TachSensor* self) 
{
    int pulse_count = 0;
    ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit)); // Reset count for the next cycle

    // Formula: RPM = (Pulses / Pulses_Per_Rev) * (60s / Measurement_Interval_s)
    float pulses_per_second = (float)pulse_count * (1000.0f / SAMPLE_PERIOD_MS);
    uint32_t rpm = (uint32_t)((pulses_per_second / PULSES_PER_REV) * 60.0f);
    self->fan_rpm = rpm;
};
