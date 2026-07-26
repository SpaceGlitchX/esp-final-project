#include "hvac_state_machine.h"
#include "sensor_manager.h"
#include "hvac_hardware.h"
#include "hvac_states.h"

static hvac_state_t current_state = STATE_IDLE;

static update_state(hvac_state_t state) {
    current_state = state;
}

void hvac_state_machine_task(void *pvParameters) {
    hvac_cmd_t event;

    while(1) {
        if (xQueueReceive(hvac_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch(current_state) {

                case STATE_IDLE:
                    if (event == CMD_HEAT) {
                        set_fan_state(0);
                        set_heater_state(1);
                        xTimerStart(flame_proving_timer, 0);
                        start_flame_proving_monitor();
                        update_state(STATE_IGNITION);
                    } else if (event == CMD_FAN_ONLY) {
                        set_fan_state(1);
                        update_state(STATE_FAN_CIRCULATE);
                    }
                    break;
                case STATE_FAN_CIRCULATE:
                    if (event == CMD_OFF) {
                        set_fan_state(0);
                        update_state(STATE_IDLE);
                    }
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