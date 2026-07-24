#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
    THE SCHEME:
    ESPs receive and output data to peripherals via GPIO
    2 ESPs communicate via UART using byte-array

    BAUD RATE: 
    TICK RATE:  10 seconds 

    HVAC:
        waits for input from thermostat and sensor inputs
        
        INPUTS:
        pilot light sensor
        fan speed sensor
        
        furnace on/off command from thermostat

        OUTPUTS:
        system status: fan on and heat on flags
        error flags

    THERMOSTAT:

        INPUTS:
        temperature
        humidity

        user-determined tempertaure target point

        OUTPUTS:
        UART to HVAC (furnace on/off command)
        LCD or LED indicators
*/

void app_main(void)
{

}