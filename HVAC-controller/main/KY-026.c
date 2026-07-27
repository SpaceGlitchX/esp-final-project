/**
 * KY-026 Flame Sensor
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define KY_ADC_UNIT     ADC_UNIT_1
#define KY_ADC_CHAN     ADC_CHANNEL_3  // Corresponds to GPIO 4 on ESP32
#define KY_ADC_ATTEN    ADC_ATTEN_DB_12 // Full scale voltage range (~3.3V)


void app_main(void) {
    // 1. Initialize ADC one-shot handle
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = KY_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // 2. Configure ADC channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = KY_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, KY_ADC_CHAN, &config));

    int raw_value = 0;

    // 3. Read loop
    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, KY_ADC_CHAN, &raw_value));
        ESP_LOGI("KY-026", "Flame Sensor Raw ADC Value: %d", raw_value);
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}