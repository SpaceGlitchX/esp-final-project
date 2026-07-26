#include "sensor_manager.h"
#include "hvac_hardware.h"
#include "driver/gpio.h"

static adc_oneshot_unit_handle_t adc_handle = NULL;
static TimerHandle_t analog_sample_timer = NULL;

static void analog_flame_check_callback(TimerHandle_t xTimer) {
    int raw_analog_value = 0;
    if (adc_oneshot_read(adc_handle, ANALOG_FLAME_CHANNEL, &raw_analog_value) == ESP_OK) {
        printf("Flame Sensor:", raw_analog_value);
    }
    // if threshold send flame detected command to queue

}

void init_sensors(void) {

    // Analog input channel config for flame sewnsor
    adc_oneshot_unit_init_cfg_t = init_config1 = {
        .unit_id = ADC_UNIT_1
    };
    ESP_ERROR_CHECK(adc_oneshot_unit_new_unit(&init_config1, &adc_handle));

    adc_oneshot_chan_cfg_t adc_config = {
        .bitwidth = ADC_BANDWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12 // Full scale configuration up to 3.3 V
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ANALOG_FLAME_CHANNEL))
}

void
}