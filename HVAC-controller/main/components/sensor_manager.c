#include "sensor_manager.h"
#include "hvac_hardware.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include <stdio.h>

static TimerHandle_t analog_sample_timer = NULL;

static void analog_flame_check_callback(TimerHandle_t xTimer) {
    float raw_analog_value = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHAN, &raw_analog_value)
    ESP_LOGI("KY-026", "Flame sensor Raw ADC Value: %0.2f", raw_analog_value)
}

void init_sensors(void) {

    // Analog input channel config for flame sewnsor
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t = init_config1 = {
        .unit_id = ADC_UNIT
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BANDWIDTH_DEFAULT,
        .atten = ADC_ATTEN // Full scale configuration up to 3.3 V
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHAN, &config))
}

void start_flame_proving_monitor(void) {
    xTimerStart(analog_sample_timer, 0);
}

void stop_flame_proving_monitor(void) {
    xTimerStop(analog_sample_timer, 0);
}
