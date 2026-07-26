#include "hvac_state_machine.h"
#include "sensor_manager.h"

static hvac_state_t current_state = STATE_IDLE;

void hvac_state_machine_task(void *pvParameters) {
    hvac_cmd_t event;

    while(1) {
        if (xQueueReceive(hvac_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch(current_state) {

                case STATE_IDLE:
                    
                    break;
                case STATE_FAN_CIRCULATE:
                    break;
                case STATE_IGNITION:
                    break;
                case STATE_WARMUP:
                    break;
                case STATE_VERIFY_RPM:
                    break;
                case STATE_RUNNING:
                    break;
                case STATE_FAULT:
                    break;
            }
        }
    }
}