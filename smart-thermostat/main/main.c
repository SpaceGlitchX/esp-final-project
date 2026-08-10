#include "thermo_sensors.h"
#include "thermo_sensors.c"
#include "thermo_ui.h"
#include "thermo_ui.c"
#include "thermo_comms.h"
#include "thermo_comms.c"

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

void app_main(void)
{

}