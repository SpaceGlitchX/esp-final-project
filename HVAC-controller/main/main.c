#include "sensor_manager.h"
#include "hardware_manager.h"
#include "hvac_state_machine.h"
#include "hvac_comms.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "test_harness.c"


void app_main(void)
{
    hvac_state_machine_init();
    sensor_manager_init();
    uart_init();
    hvac_hardware_init();

    xTaskCreate(test_harness_task, "test", 2048, NULL, 4, NULL);
    
}
