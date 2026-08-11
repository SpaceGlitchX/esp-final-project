#pragma once

#include <stdint.h>
#include <stdio.h>
#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

//~  SENSORS HEADER
//~ Defines the temperature sensor pins, shared ADC readings
//~  and the sensors initialization and task functions

#define INDOOR_TEMP_CHANNEL ADC_CHANNEL_0 //& GPIO36
#define OUTDOOR_TEMP_CHANNEL ADC_CHANNEL_3 //& GPIO39

//Reads both sensors every 10 sec.
#define SENSOR_PERIOD_MS 10000

//  Thermistor inputs (Setup ADC on these)
#define TEMP_VI GPIO_NUM_36 // Indoor temperature sensor 
#define TEMP_VO GPIO_NUM_39 // Outdoor temperature sensor 

struct SensorData {
    int indoor_temp;
    int outdoor_temp;
};

//Configure the ADC and creates the temp. sensor task
void thermo_sensors_init(void);

//Reads both thermistors
void temperature_sensor_task(void *pvParameters);
