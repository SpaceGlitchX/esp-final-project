#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#pragma once

/*  SENSORS HEADER
Covers the sensors and sensor data.
*/

//  Thermistor inputs (Setup ADC on these)
#define TEMP_V1 GPIO_NUM_36
#define TEMP_V2 GPIO_NUM_39