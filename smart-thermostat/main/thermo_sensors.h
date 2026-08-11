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


//  Thermistor inputs (Setup ADC on these)
#define TEMP_VI GPIO_NUM_36 // Indoor temperature sensor 
#define TEMP_VO GPIO_NUM_39 // Outdoor temperature sensor 

//Most recent indoor temp. reading
int indoor_temperature_raw;

//Most recent outdoor temp. reading
int outdoor_temperature_raw;

//Configure the ADC and creates the temp. sensor task
void thermo_sensors_init(void);

//Reads both thermistors
void temperature_sensor_task(void *pvParameters);