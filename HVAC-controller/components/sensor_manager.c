#include "sensor_manager.h"
#include "hvac_hardware.h"
#include "driver/gpio.h"

static adc_oneshot_unit_handle_t adc_handle = NULL;
static TimerHandle_t analog_sample_timer = NULL;
static TimerHandle_t temp_sample_timer = NULL;

static void analog_flame_check_callback(TimerHandle_t xTimer) {
    int raw_analog_value = 0;
    if (adc_oneshot_read(adc_handle, ANALOG_FLAME_CHANNEL, &raw_analog_value) == ESP_OK) {
        printf("Flame Sensor:", raw_analog_value);
    }
}