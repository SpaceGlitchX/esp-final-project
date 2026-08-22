#include "sensor_manager.h"
#include "hardware_manager.h"
#include "hvac_state_machine.h"
#include "hvac_comms.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void app_main(void)
{

    hardware_manager_init();

    state_machine_init();

    sensor_manager_init();

    uart_init();
}