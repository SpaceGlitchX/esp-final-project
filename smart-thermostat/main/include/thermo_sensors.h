#ifndef THERMO_SENSORS_H
#define THERMO_SENSORS_H

#include <stdint.h>
#include "hvac_states.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define INDOOR_TEMP_CHANNEL ADC_CHANNEL_6 // GPIO PIN 34

#define SENSOR_PERIOD_MS 1000

typedef struct {
	int indoor_temp;
} SensorData;

void thermo_sensors_init(void);
void temperature_sensor_task(void *pvParameters);

extern float get_indoor_adc(void);

#endif