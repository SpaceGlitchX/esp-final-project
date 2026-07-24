#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "esp_err.h"

// Hardware pin configuration
#define FLAME_ADC_CHANNEL ADC_CHANNEL_6
#define FLAME_ADC_UNIT ADC_UNIT_1
#define FLAME_THRESHOLD 600

#define FAN_TACH_PIN 16

#define DS18B20_GPIO 4

esp_err_t sensor_manager_init(void);

int sensor_manager_flame_check(void);

float sensor_manager_read_temperature(void);

uint32_t sensor_manager_get_fan_rpm(void);

#endif