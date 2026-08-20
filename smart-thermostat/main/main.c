#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor_manager.h"
#include "thermo_logic.h"
#include "thermo_comms.h"
#include "user_input.h"


void app_main(void)
{
    printf("\n");
    printf("============================================\n");
    printf("       THERMOSTAT SYSTEM STARTING\n");
    printf("============================================\n");

    fflush(stdout);


    /*
     * Initialize temperature sensor FIRST.
     *
     * This creates:
     * - ADC
     * - ADC calibration
     * - temperature queue
     * - temperature sensor task
     */

	sensor_manager_init();

    /*
     * Initialize UART communication.
     */

    thermo_comms_init();


    /*
     * Initialize thermostat logic.
     *
     * This creates the control task that reads
     * temperature from the queue.
     */

    thermo_logic_init();


    /*
     * Initialize buttons / user input.
     */

    user_input_init();


    printf("\n");
    printf("============================================\n");
    printf(" Thermostat app_main initialization complete\n");
    printf("============================================\n");

    fflush(stdout);


    /*
     * app_main stays alive.
     */

    while (1)
    {
        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );
    }
}