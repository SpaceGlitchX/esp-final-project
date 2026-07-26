#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "esp_err.h"

// Hardware pin configuration
#define FLAME_ADC_CHANNEL ADC_CHANNEL_6 // GPIO 34
#define FLAME_ADC_UNIT ADC_UNIT_1
#define FLAME_THRESHOLD 500

#define FAN_TACH_PIN 16

#define TEMP_SENSE_PIN 4

esp_err_t sesnor_manager_init(void);

float sensor_manager_read_temperature(void);

int sensor_manager_is_flame_detected(void);

uint32_t sensor_manager_get_fan_rpm(void);

#endif