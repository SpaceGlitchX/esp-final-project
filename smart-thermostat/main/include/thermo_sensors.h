#ifndef THERMO_SENSORS_H
#define THERMO_SENSORS_H

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

#define INDOOR_TEMP_CHANNEL ADC_CHANNEL_0
#define OUTDOOR_TEMP_CHANNEL ADC_CHANNEL_3

#define SENSOR_PERIOD_MS 1000

typedef struct {
	int indoor_temp;
	int outdoor_temp;
} SensorData;

void thermo_sensors_init(void);

void temperature_sensor_task(void *pvParameters);

float thermo_sensors_get_indoor_temperature(void);

float thermo_sensors_get_outdoor_temperature(void);

int thermo_sensors_get_indoor_adc(void);

int thermo_sensors_get_outdoor_adc(void);

#endif