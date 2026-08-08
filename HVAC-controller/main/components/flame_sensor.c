#include "flame_sensor.h"

static const char *TAG = "FlameSensor";

void flame_sensor_init(FlameSensor* self) {
    if (!self) return;

    self->init = flame_sensor_init;
    self->read = flame_sensor_read;
    self->value = 0;

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &self->adc_handle));
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(self->adc_handle, ADC_CHAN, &config));
}

void flame_sensor_read(FlameSensor* self) {
    if (!self || !self->adc_handle) return;

    int raw_value = 0;
    
    esp_err_t err = adc_oneshot_read(self->adc_handle, ADC_CHAN, &raw_value);
    
    if (err == ESP_OK) {
        self->value = (uint16_t)raw_value;
        ESP_LOGI(TAG, "Raw ADC Value: %u", self->value);
    } else {
        ESP_LOGE(TAG, "Failed to read ADC channel");
    }
}