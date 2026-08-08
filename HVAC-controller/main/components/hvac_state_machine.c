#include "hvac_state_machine.h"
#include "sensor_manager.h"
#include "hvac_hardware.h"
#include "hvac_states.h"

static hvac_state_t current_state = STATE_IDLE;
static hvac_flt_t fault_flag = NULL;

static update_state(hvac_state_t state) {
    current_state = state;
}

static update_fault(hvac_flt_t fault) {
    fault_flag = fault;
}

static void IRAM_ATTR fault_isr_handler(void *arg) {
    hvac_cmd_t cmd = CMD_OFF;
    BaseType_t xHigherPriorityTaskWoken = pdFalse; 
    
    xQueueSendFromISR(hvac_queue, &cmd, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}
void hvac_state_machine_task(void *pvParameters) {
    hvac_cmd_t event;

    while(1) {
        if (xQueueReceive(hvac_queue, &event, portMAX_DELAY) == pdTRUE) {

            if (event == CMD_OFF && current_state != STATE_FAULT) {
                set_heater_state(0);
                set_fan_state(0);
                update_state(STATE_IDLE);
                update_fault(NULL);
                continue;
            }
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
                    if (event == CMD_FLAME_DETECTED) {
                        stop_flame_proving_monitor();
                        xTimerStart(fan_warmup_timer, 0);
                        xTimerStop(flame_proving_timer, 0);
                        update_state(STATE_WARMUP);
                    } else if (event == CMD_FLAME_TIMEOUT) {
                        stop_flame_proving_monitor();
                        xTimerStop(flame_proving_timer, 0);
                        update_fault(FLT_FLAME);
                        update_state(STATE_FAULT);
                    }
                    break;
                case STATE_WARMUP:
                    if (event == CMD_WARMUP_COMPLETE) {
                        xTimerStop(fan_warmup_timer, 0);
                        start_tach_monitoring();
                        update_state(STATE_VERIFY_RPM);
                    }
                    break;
                case STATE_VERIFY_RPM:
                    if (event == CMD_TACH_TIMEOUT) {
                        stop_tach_monitoring();
                        update_fault(FLT_FAN);
                        update_state(STATE_FAULT);
                    } else if (event == CMD_RPM_VERIFIED) {
                        stop_tach_monitoring();
                        update_state(STATE_RUNNING);
                    }
                    break;
                case STATE_RUNNING:
                    if (event == CMD_OFF) {
                        set_heater_state(0);
                        set_fan_state(0);
                        update_state(STATE_IDLE);
                    } else if (event == CMD_FAN_)
                    break;
                case STATE_FAULT:
                    set_heater_state(0);
                    set_fan_state(0);
                    if (event == CMD_OFF) {
                        update_state(STATE_IDLE);
                    }
                    break;
            }
        }
    }
}