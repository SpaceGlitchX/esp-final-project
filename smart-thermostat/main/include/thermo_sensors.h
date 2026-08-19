#ifndef THERMO_SENSORS_H
#define THERMO_SENSORS_H

#include <stdint.h>

#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define INDOOR_TEMP_CHANNEL ADC_CHANNEL_6 // GPIO PIN 34
#define OUTDOOR_TEMP_CHANNEL ADC_CHANNEL_4 // GPIO PIN 32

#define SENSOR_PERIOD_MS 10000

typedef struct {
	int indoor_temp;
	int outdoor_temp;
} SensorData;

void thermo_sensors_init(void);
void temperature_sensor_task(void *pvParameters);

int get_indoor_adc(void);
int get_outdoor_adc(void);

#endif