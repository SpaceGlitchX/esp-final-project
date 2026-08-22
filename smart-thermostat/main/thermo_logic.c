#include "thermo_logic.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "user_input.h"
#include "sensor_manager.h"
#include "thermo_comms.h"

QueueHandle_t thermo_queue = NULL;
extern Settings settings;


void thermo_control_task(void *pvParameters) {
    (void)pvParameters;
    float temp;
    hvac_cmd_t cmd;
    
    while (1) {

        if (settings.power_mode ==)
        if (xQueueReceive(temp_queue, &temp, portMAX_DELAY) == pdTRUE) {
            
            float indoor_temp = temp;
            if (indoor_temp < settings.setpoint - DEADBAND_C) {
                cmd = CMD_HEAT;
                xQueueSend(thermo_queue, &cmd, portMAX_DELAY);
            } else {
                cmd = CMD_OFF;
                xQueueSend(thermo_queue, &cmd, portMAX_DELAY);
            }

            


void thermo_logic_init(void)


{
    
    thermo_queue = xQueueCreate(10, sizeof(hvac_cmd_t));

    if (thermo_queue == NULL) {
		printf("Failed to create user input queue\n");
		return;
	}


    BaseType_t result =
        xTaskCreate(
            thermo_control_task,
            "thermo_control",
            3072,
            NULL,
            2,
            NULL
        );


    if (result != pdPASS)
    {
        printf(
            "Failed to create thermostat control task\n"
        );

        return;
    }


    printf(
        "Thermostat control task started\n"
    );
}