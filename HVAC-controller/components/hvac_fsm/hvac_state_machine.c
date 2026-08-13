#include "hvac_state_machine.h"
#include "sensor_manager.h"
#include "hardware_manager.h"
#include "hvac_states.h"
#include "esp_log.h"

static const char *TAG = "HVAC_FSM";

// Queue Handle
QueueHandle_t hvac_queue = NULL;

// Global System Monitoring Variables
hvac_state_t g_current_hvac_state = STATE_FAN_CIRCULATE;
hvac_flt_t g_current_hvac_fault   = FLT_NONE;

// Timer & Pacing Delay Handles
static TimerHandle_t state_pacing_timer = NULL;
static hvac_state_t  g_next_pending_state = STATE_FAN_CIRCULATE;

static void update_state(hvac_state_t state) {
    ESP_LOGI(TAG, "State Transition: %d -> %d", g_current_hvac_state, state);
    g_current_hvac_state = state;
}

static void update_fault(hvac_flt_t fault) {
    if (fault != FLT_NONE) {
        ESP_LOGE(TAG, "HVAC Fault Occurred: Fault Code %d", fault);
    }
    g_current_hvac_fault = fault;
}

/**
 * @brief Safe emergency shutdown of all relays and sampling timers.
 */
static void safe_shutdown_actuators(void) {
    set_heater_state(0);
    set_fan_state(0);
    
    stop_flame_proving_monitor();
    stop_tach_monitoring();

    if (state_pacing_timer  != NULL) xTimerStop(state_pacing_timer,  portMAX_DELAY);
    if (flame_proving_timer != NULL) xTimerStop(flame_proving_timer, portMAX_DELAY);
    if (fan_warmup_timer   != NULL) xTimerStop(fan_warmup_timer,   portMAX_DELAY);
    if (tach_window_timer  != NULL) xTimerStop(tach_window_timer,  portMAX_DELAY);
}

/**
 * @brief Timer callback when inter-state delay expires.
 */
static void state_delay_callback(TimerHandle_t xTimer) {
    (void)xTimer;
    hvac_cmd_t cmd = CMD_STATE_DELAY_COMPLETE;
    xQueueSend(hvac_queue, &cmd, 0);
}

/**
 * @brief Initiates a non-blocking delay before moving to the next state.
 */
static void transition_with_delay(hvac_state_t target_state, uint32_t delay_ms) {
    g_next_pending_state = target_state;
    update_state(STATE_WAIT_DELAY);

    xTimerChangePeriod(state_pacing_timer, pdMS_TO_TICKS(delay_ms), portMAX_DELAY);
    xTimerStart(state_pacing_timer, portMAX_DELAY);
}

/**
 * @brief Hardware Emergency Interrupt Handler
 */
void IRAM_ATTR fault_isr_handler(void *arg) {
    (void)arg;
    hvac_cmd_t cmd = CMD_OFF;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE; 
    
    xQueueSendFromISR(hvac_queue, &cmd, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief Initializes queues, software timers, and initial states.
 */
esp_err_t hvac_state_machine_init(void) {
    ESP_LOGI(TAG, "Initializing HVAC State Machine...");

    if (hvac_queue == NULL) {
        hvac_queue = xQueueCreate(10, sizeof(hvac_cmd_t));
        if (hvac_queue == NULL) {
            ESP_LOGE(TAG, "Failed to create HVAC command queue!");
            return ESP_FAIL;
        }
    }

    if (state_pacing_timer == NULL) {
        state_pacing_timer = xTimerCreate("StatePacingTimer", 
                                          pdMS_TO_TICKS(4000), 
                                          pdFALSE, 
                                          NULL, 
                                          state_delay_callback);
        if (state_pacing_timer == NULL) {
            ESP_LOGE(TAG, "Failed to create inter-state delay timer!");
            return ESP_FAIL;
        }
    }

    g_current_hvac_state = STATE_IDLE;
    g_current_hvac_fault = FLT_NONE;
    safe_shutdown_actuators();

    ESP_LOGI(TAG, "HVAC State Machine Initialized Successfully.");
    return ESP_OK;
}

/**
 * @brief Main HVAC Finite State Machine Task
 */
void hvac_state_machine_task(void *pvParameters) {
    (void)pvParameters;
    hvac_cmd_t event;

    ESP_LOGI(TAG, "HVAC State Machine Task Started.");

    while (1) {
        if (xQueueReceive(hvac_queue, &event, portMAX_DELAY) == pdTRUE) {
            
            // 1. GLOBAL EMERGENCY / OFF COMMAND
            if (event == CMD_OFF) {
                safe_shutdown_actuators();
                update_fault(FLT_NONE);
                update_state(STATE_IDLE);
                ESP_LOGI(TAG, "System returned to IDLE via CMD_OFF.");
                continue;
            }

            // 2. INTER-STATE DELAY HANDLING
            if (g_current_hvac_state == STATE_WAIT_DELAY) {
                if (event == CMD_STATE_DELAY_COMPLETE) {
                    hvac_state_t next = g_next_pending_state;
                    update_state(next);
                    
                    // Trigger entry actions for target states
                    if (next == STATE_IGNITION) {
                        set_heater_state(1);
                        xTimerStart(flame_proving_timer, portMAX_DELAY);
                        start_flame_proving_monitor();
                    } else if (next == STATE_WARMUP) {
                        xTimerStart(fan_warmup_timer, portMAX_DELAY);
                    } else if (next == STATE_VERIFY_RPM) {
                        set_fan_state(1);
                        start_tach_monitoring();
                        xTimerStart(tach_window_timer, portMAX_DELAY);
                    }
                }
                continue; 
            }

            // 3. NORMAL EVENT SWITCH
            switch (g_current_hvac_state) {

                case STATE_IDLE:
                    if (event == CMD_HEAT) {
                        set_fan_state(0);
                        transition_with_delay(STATE_IGNITION, 1000);
                    } else if (event == CMD_FAN_ONLY) {
                        set_fan_state(1);
                        transition_with_delay(STATE_FAN_CIRCULATE, 500);
                    }
                    break;

                case STATE_FAN_CIRCULATE:
                    if (event == CMD_HEAT) {
                        transition_with_delay(STATE_IGNITION, 1000);
                    }
                    break;

                case STATE_IGNITION:
                    if (event == CMD_FLAME_DETECTED) {
                        stop_flame_proving_monitor();
                        xTimerStop(flame_proving_timer, portMAX_DELAY);
                        transition_with_delay(STATE_WARMUP, 1000);

                    } else if (event == CMD_FLAME_TIMEOUT) {
                        safe_shutdown_actuators();
                        update_fault(FLT_FLAME);
                        update_state(STATE_FAULT);
                    }
                    break;

                case STATE_WARMUP:
                    if (event == CMD_WARMUP_DONE) {
                        xTimerStop(fan_warmup_timer, portMAX_DELAY);
                        transition_with_delay(STATE_VERIFY_RPM, 500);
                    }
                    break;

                case STATE_VERIFY_RPM:
                    if (event == CMD_FAN_OK) {
                        xTimerStop(tach_window_timer, portMAX_DELAY);
                        stop_tach_monitoring();
                        transition_with_delay(STATE_RUNNING, 1000);

                    } else if (event == CMD_TACH_TIMEOUT) {
                        safe_shutdown_actuators();
                        update_fault(FLT_FAN);
                        update_state(STATE_FAULT);
                    }
                    break;

                case STATE_RUNNING:
                    // Maintain heat & fan status
                    break;

                case STATE_FAULT:
                    // System locked out until CMD_OFF is sent
                    set_heater_state(0);
                    set_fan_state(0);
                    break;

                default:
                    safe_shutdown_actuators();
                    update_state(STATE_IDLE);
                    break;
            }
        }
    }
}

hvac_state_t get_hvac_state(void) {
    return (g_current_hvac_state);
}
hvac_flt_t get_hvac_fault(void) {
    return (g_current_hvac_fault);
}