#include "sensor_manager.h"
#include "hardware_manager.h"
#include "hvac_state_machine.h"
#include "hvac_comms.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void app_main(void)
{
    hvac_state_machine_init();
    xTaskCreate(
        hvac_state_machine_task,
        "hvac_fsm",
        4096,
        NULL,
        5,
        NULL
    );
    init_hvac_hardware();
    sensor_manager_init();
    uart_setup();
}
