#include "flame_sensor.h"

static const char *TAG = "FLAME_SENSOR";

// Instantiate global instance
FlameSensor flame_sensor = {0};


void flame_sensor_read(FlameSensor* self) {
    if (self == NULL || self->adc_handle == NULL) {
        ESP_LOGE(TAG, "Cannot read: ADC handle is uninitialized");
        return;
    }

    int raw_val = 0;
    esp_err_t err = adc_oneshot_read(self->adc_handle, ADC_CHAN, &raw_val);
    if (err == ESP_OK) {
        self->value = (uint16_t)raw_val;
    } else {
        ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(err));
    }
}

void flame_sensor_init(FlameSensor* self) {
    if (self == NULL) return;

    // 1. Bind method pointers to this instance
    self->init = flame_sensor_init;
    self->read = flame_sensor_read;

    // 2. Configure ADC Oneshot Unit
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    
    esp_err_t err = adc_oneshot_new_unit(&init_config, &self->adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC unit: %s", esp_err_to_name(err));
        return;
    }

    // 3. Configure ADC Channel Attenuation
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    
    err = adc_oneshot_config_channel(self->adc_handle, ADC_CHAN, &config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Flame sensor ADC initialized on Channel %d", ADC_CHAN);
    } else {
        ESP_LOGE(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(err));
    }
}