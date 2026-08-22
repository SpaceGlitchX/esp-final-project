#ifndef THERMO_SENSORS_H
#define THERMO_SENSORS_H

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "hvac_states.h"
#include "sensor_manager.h"
#include "thermo_logic.h"
#include <stdio.h>
#include <math.h>
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

#define SENSOR_PERIOD_MS 200

void thermistor_task(void *pvParameters);


#endif