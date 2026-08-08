#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#pragma once

/*  COMMUNICATIONS HEADER
Covers the UART hardware, transmission functions, receiving functions, and data
*/

//  UART pins
#define UART_TX GPIO_NUM_1  // UART connection to HVAC, TX
#define UART_RX GPIO_NUM_3  // UART connection to HVAC, RX