#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*  THE SCHEME:
    ESPs receive and output data to peripherals via GPIO
    2 ESPs communicate via UART using byte-array

    BAUD RATE: 
    TICK RATE:  10 seconds
*/
/*  HVAC:
        waits for input from thermostat and sensor inputs
        
        INPUTS:
        pilot light sensor
        fan speed sensor
        
        furnace on/off command from thermostat

        OUTPUTS:
        system status: fan on and heat on flags
        error flags
*/
/*  THERMOSTAT:

        INPUTS:
        temperature
        humidity

        user-determined tempertaure target point

        OUTPUTS:
        UART to HVAC (furnace on/off command)
        LCD or LED indicators
*/
/*  PINOUTS:
    4 button inputs: SET_UP, SET_DWN, SEL_UP, SEL_DWN; mapped to pins GPIO4, GPIO0, GPIO2, GPIO15
    2 thermistors: TEMP_V1, TEMP_V2; mapped to pins GPIO36, GPIO39
    GPIO25 has a DAC, which will go to the LCD pin V0, which controls contrast
    GPIO19 maps to LCD pin RS
    GPIO21 maps to LCD pin E
    GPIO18, GPIO5, GPIO17, GPIO16 map to LCD pins D7, D6, D5, D4 
    LCD will always be in write mode, so R/W pin is tied to ground
    All other LCD pins are either floating or tied to power and ground pins
    GPIO13 needs PWM enabled and will control an indicator LED
    GPIO1 is mapped to the UART TX pin
    GPIO3 is mapped to the UART RX pin
    The two ESP boards must share a common ground!
*/
//  Pushbutton inputs
#define SET_UP GPIO_NUM_4
#define SET_DWN GPIO_NUM_0
#define SEL_UP GPIO_NUM_2
#define SEL_DWN GPIO_NUM_15
//  Thermistor inputs (Setup ADC on these)
#define TEMP_V1 GPIO_NUM_36
#define TEMP_V2 GPIO_NUM_39
//  LCD OUTPUTS
#define CONT GPIO_NUM_25    // Output toward LCD contast pin, needs DAC
#define LCD_RS GPIO_NUM_19  // Register Select pin: 0 means means incoming data is a command, 1 means it's character data
#define LCD_E GPIO_NUM_21   // Enable pin: when pulsed LOW from default HIGH, LCD will record incoming data to memory
#define LCD_D7 GPIO_NUM_18  // LCD Data pin 7
#define LCD_D6 GPIO_NUM_5   // LCD Data pin 6
#define LCD_D5 GPIO_NUM_17  // LCD Data pin 5
#define LCD_D4 GPIO_NUM_16  // LCD Data pin 4
//  UART pins
#define UART_TX GPIO_NUM_1  // UART connection to HVAC, TX
#define UART_RX GPIO_NUM_3  // UART connection to HVAC, RX