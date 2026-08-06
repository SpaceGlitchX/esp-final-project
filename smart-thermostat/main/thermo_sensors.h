#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#pragma once

/*  SENSORS HEADER
Covers the sensors and sensor data.
*/

//  Thermistor inputs (Setup ADC on these)
#define TEMP_VI GPIO_NUM_36 // Indoor temperature sensor (Analog voltage input)
#define TEMP_VO GPIO_NUM_39 // Outdoor temperature sensor (Analog voltage output)