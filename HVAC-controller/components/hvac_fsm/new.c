#include "hvac_state_machine.h"

#include "hardware_manager.h"
#include "hvac_states.h"
#include "sensor_manager.h"

#include "esp_log.h"


static const char *TAG = "HVAC_FSM";

QueueHandle_t hvac_queue = NULL;
SemaphoreHandle_t hvac_state_mutex = NULL;

hvac_state_t g_current_hvac_state = STATE_IDLE;
hvac_flt_t g_current_hvac_fault = FLT_NONE;
hvac_cmd_t g_current_hvac_cmd = CMD_OFF;


static bool fan_auto = true;
static bool safety_latch = false;

static TimerHandle_t state_pacing_timer = NULL;
static hvac_state_t g_next_pending_state = STATE_IDLE;

static void update_state(hvac_state_t state) {
	g_current_hvac_state = state;
}

static void update_fault(hvac_flt_t fault) {

	if (fault != FLT_NONE) {
		safe_shutdown_actuators(0);
		xQueueSend(hvac_queue, CMD_SAFETY_FAULT, 0)
	}

    g_current_hvac_fault = fault;
}

static void safe_shutdown_actuators(void)
{
    // Turn everything OFF
    set_heater_state(0);
    set_fan_state(0);

    // Stop sensor monitoring
    stop_flame_proving_monitor();
    stop_tach_monitoring();

    // Stop state timer
    if (state_pacing_timer != NULL) {
		xTimerStop(state_pacing_timer, 0);
    }

    // Stop flame timer
    if (flame_proving_timer != NULL) {
        xTimerStop(flame_proving_timer, 0);
    }

    // Stop warmup timer
    if (fan_warmup_timer != NULL) {
        xTimerStop(fan_warmup_timer, 0);
    }

    // Stop tach timer
    if (tach_window_timer != NULL) {
        xTimerStop(tach_window_timer, 0);
    }
}

esp_err_t hvac_state_machine_init(void) {
    ESP_LOGI(TAG, "Initializing HVAC State Machine...");

    hvac_queue = xQueueCreate(10, sizeof(hvac_cmd_t));

    if (hvac_queue == NULL){
        ESP_LOGE(TAG, "Failed to create HVAC command queue!");
        return ESP_FAIL;
    }

    hvac_state_mutex = xSemaphoreCreateMutex();

    if (hvac_state_mutex == NULL) {
        ESP_LOGE(
            TAG,
            "Failed to create HVAC state mutex!"
        );
        return ESP_FAIL;
    }

    state_pacing_timer = xTimerCreate(
        "StatePacingTimer",
        pdMS_TO_TICKS(1000),
        pdFALSE,
        NULL,
        state_delay_callback
    );

    if (state_pacing_timer == NULL) {
        ESP_LOGE(
            TAG,
            "Failed to create state pacing timer!"
        );
        return ESP_FAIL;
    }

    g_current_hvac_state = STATE_IDLE;
    g_current_hvac_fault = FLT_NONE;
    g_current_hvac_cmd = CMD_OFF;
    fan_auto = true;

    safe_shutdown_actuators();

    BaseType_t task_result = xTaskCreate(
        hvac_state_machine_task,
        "hvac_state_machine",
        4096,
        NULL,
        5,
        NULL
    );

    if (task_result != pdPASS) {
        ESP_LOGE(
            TAG,
            "Failed to create HVAC state machine task!"
        );
        return ESP_FAIL;
    }

    ESP_LOGI(
        TAG,
        "HVAC State Machine initialized"
    );

    return ESP_OK;
}

void hvac_state_machine_task(void *pvParameters) {
    (void)pvParameters;
    hvac_cmd_t event;

    ESP_LOGI(
        TAG,
        "HVAC State Machine Task Started."
    );

   while (1) {


    xQueueReceive(hvac_queue, &event, portMAX_DELAY);

    if (event == CMD_SAFETY_FAULT) {
		safety_latch = true;
		update_state(STATE_FAULT);
        continue;
    }

    if (event == CMD_OFF) {
        safe_shutdown_actuators();
		safety_latch = false;
		update_fault(FLT_NONE);
        continue;
    }

	// STATE MACHINE
    switch (g_current_hvac_state) {

		// Fault
		case STATE_FAULT:
			set_fan_state(0);
			set_heater_state(0);

			if (safety_latch == false) {
				update_state(STATE_IDLE);

			}

		// Idle
        case STATE_IDLE:
            set_fan_state(0);
			set_heater_state(0);

			if (event == CMD_HEAT) {
				update_state(STATE_IGNITION);

			} else if (event == CMD_FAN_ON) {
				fan_auto = false;
				update_state(STATE_FAN_CIRCULATE);
			}
            break;

		// Fan Circulate
        case STATE_FAN_CIRCULATE:
            set_fan_state(1);

			if (event == CMD_FAN_AUTO) {
				fan_auto == true;
				update_state(STATE_IDLE);

			} else if (event == CMD_HEAT) {
				start_flame_proving_monitor();
				update_state(STATE_IGNITION);
			}
            break;

		// Ignition
        case STATE_IGNITION:
            set_fan_state(0);
			set_heater_state(1);
			xTimerStart(flame_proving_timer, 0);

			if (event == CMD_FLAME_DETECTED) {
				stop_flame_proving_monitor();
				xTimerStop(flame_proving_timer);
				xTimerStart(fan_warmup_timer, 0);
				update_state(STATE_WARMUP);

			} else if (event == CMD_FLAME_TIMEOUT) {
				stop_flame_proving_monitor();
				update_fault(FLT_FLAME);
				update_state(STATE_FAULT);

			}
            break;

        case STATE_WARMUP:
            
			if (event == CMD_WARMUP_DONE) {
				update_state(STATE_VERIFY_RPM);
			}
            break;

        case STATE_VERIFY_RPM:
            set_fan_state(1);
			start_tach_monitoring();
			xTimerStart(tach_window_timer, 0);

			if (event == CMD_FAN_OK) {
				stop_tach_monitoring();
				xTimerStop(tach_window_timer);
				update_state(STATE_RUNNING);

			} else if (event == CMD_FLAME_TIMEOUT) {
				stop_tach_monitoring();
				update_fault(FLT_FAN);
				update_state(STATE_FAULT);

			}
            break;

        case STATE_RUNNING:
            
			if (event == CMD_HEAT_OFF && fan_auto == true) {
				update_state(STATE_IDLE);

			} else if (event == CMD_HEAT_OFF && fan_auto == false) {
				update_state(STATE_FAN_CIRCULATE);

			}
            break;

        default:
            safe_shutdown_actuators();
            update_state(STATE_IDLE);
            break;
    }
}

hvac_state_t hvac_get_state(void)
{
    hvac_state_t state = STATE_IDLE;

    if (hvac_state_mutex != NULL)
    {
        if (xSemaphoreTake(
                hvac_state_mutex,
                portMAX_DELAY
            ) == pdTRUE)
        {
            state = g_current_hvac_state;

            xSemaphoreGive(
                hvac_state_mutex
            );
        }
    }

    return state;
}

hvac_flt_t hvac_get_fault(void)
{
    hvac_flt_t fault = FLT_NONE;

    if (hvac_state_mutex != NULL)
    {
        if (xSemaphoreTake(
                hvac_state_mutex,
                portMAX_DELAY
            ) == pdTRUE)
        {
            fault = g_current_hvac_fault;

            xSemaphoreGive(
                hvac_state_mutex
            );
        }
    }

    return fault;
}

hvac_cmd_t hvac_get_command(void)
{
    hvac_cmd_t command = CMD_OFF;

    if (hvac_state_mutex != NULL)
    {
        if (xSemaphoreTake(
                hvac_state_mutex,
                portMAX_DELAY
            ) == pdTRUE)
        {
            command = g_current_hvac_cmd;

            xSemaphoreGive(
                hvac_state_mutex
            );
        }
    }

    return command;
}