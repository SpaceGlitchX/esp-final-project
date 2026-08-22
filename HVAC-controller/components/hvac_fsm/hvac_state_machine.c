#include "hvac_state_machine.h"

#include "esp_log.h"

#include "hardware_manager.h"
#include "sensor_manager.h"


static const char *TAG = "HVAC_FSM";


/* ============================================================
 * Global Handles
 * ============================================================ */

QueueHandle_t hvac_queue = NULL;

SemaphoreHandle_t hvac_state_mutex = NULL;

TimerHandle_t fan_warmup_timer = NULL;

TimerHandle_t fan_cooldown_timer = NULL;


/* ============================================================
 * Internal State
 * ============================================================ */

static hvac_state_t g_current_hvac_state = STATE_IDLE;

static hvac_flt_t g_current_hvac_fault = FLT_NONE;

static hvac_cmd_t g_current_hvac_cmd = CMD_OFF;

static bool fan_auto = true;

static bool safety_latch = false;


/* ============================================================
 * Forward Declarations
 * ============================================================ */

static void state_machine_task(void *pvParameters);

static void update_state(hvac_state_t state);

static void update_fault(hvac_flt_t fault);

static void safe_shutdown_actuators(void);

static void heating_complete(void);

static void fan_warmup_callback(
    TimerHandle_t xTimer);

static void fan_cooldown_callback(
    TimerHandle_t xTimer);


/* ============================================================
 * Warmup Timer
 * ============================================================ */

static void fan_warmup_callback(
    TimerHandle_t xTimer)
{
    (void)xTimer;

    hvac_cmd_t cmd =
        CMD_WARMUP_DONE;

    if (hvac_queue != NULL) {

        xQueueSend(
            hvac_queue,
            &cmd,
            0
        );
    }
}


/* ============================================================
 * Cooldown Timer
 * ============================================================ */

static void fan_cooldown_callback(
    TimerHandle_t xTimer)
{
    (void)xTimer;

    hvac_cmd_t cmd =
        CMD_COOLDOWN_DONE;

    if (hvac_queue != NULL) {

        xQueueSend(
            hvac_queue,
            &cmd,
            0
        );
    }
}


/* ============================================================
 * State Update
 * ============================================================ */

static void update_state(
    hvac_state_t state)
{
    if (hvac_state_mutex != NULL) {

        xSemaphoreTake(
            hvac_state_mutex,
            portMAX_DELAY
        );

        ESP_LOGI(
            TAG,
            "STATE: %d -> %d",
            g_current_hvac_state,
            state
        );

        g_current_hvac_state =
            state;

        xSemaphoreGive(
            hvac_state_mutex
        );
    }
}


/* ============================================================
 * Fault Update
 * ============================================================ */

static void update_fault(
    hvac_flt_t fault)
{
    if (hvac_state_mutex != NULL) {

        xSemaphoreTake(
            hvac_state_mutex,
            portMAX_DELAY
        );

        g_current_hvac_fault =
            fault;

        xSemaphoreGive(
            hvac_state_mutex
        );
    }


    if (fault != FLT_NONE) {

        safety_latch = true;

        ESP_LOGE(
            TAG,
            "HVAC FAULT: %d",
            fault
        );

        safe_shutdown_actuators();

        update_state(
            STATE_FAULT
        );
    }
}


/* ============================================================
 * Safe Shutdown
 * ============================================================ */

static void safe_shutdown_actuators(void)
{
    set_heater_state(0);

    set_fan_state(0);

    stop_fan_check();

    stop_flame_check();


    if (fan_warmup_timer != NULL) {

        xTimerStop(
            fan_warmup_timer,
            0
        );
    }


    if (fan_cooldown_timer != NULL) {

        xTimerStop(
            fan_cooldown_timer,
            0
        );
    }
}


/* ============================================================
 * Heating Complete
 * ============================================================ */

static void heating_complete(void)
{
    ESP_LOGI(
        TAG,
        "Heat OFF - starting 30 second cooldown"
    );


    /* Burner off */

    set_heater_state(0);


    /* Fan remains on */

    set_fan_state(1);


    stop_fan_check();

    stop_flame_check();


    xTimerStart(
        fan_cooldown_timer,
        0
    );


    update_state(
        STATE_COOLDOWN
    );
}


/* ============================================================
 * Initialization
 * ============================================================ */

esp_err_t state_machine_init(void)
{
    /*
     * Command queue
     */

    hvac_queue =
        xQueueCreate(
            20,
            sizeof(hvac_cmd_t)
        );

    if (hvac_queue == NULL) {

        ESP_LOGE(
            TAG,
            "Failed to create HVAC queue"
        );

        return ESP_FAIL;
    }


    /*
     * Mutex
     */

    hvac_state_mutex =
        xSemaphoreCreateMutex();

    if (hvac_state_mutex == NULL) {

        return ESP_FAIL;
    }


    /*
     * Warmup timer
     */

    fan_warmup_timer =
        xTimerCreate(
            "warmup_timer",
            pdMS_TO_TICKS(10000),
            pdFALSE,
            NULL,
            fan_warmup_callback
        );

    if (fan_warmup_timer == NULL) {

        return ESP_FAIL;
    }


    /*
     * Cooldown timer
     */

    fan_cooldown_timer =
        xTimerCreate(
            "cooldown_timer",
            pdMS_TO_TICKS(30000),
            pdFALSE,
            NULL,
            fan_cooldown_callback
        );

    if (fan_cooldown_timer == NULL) {

        return ESP_FAIL;
    }


    /*
     * Initial state
     */

    g_current_hvac_state =
        STATE_IDLE;

    g_current_hvac_fault =
        FLT_NONE;

    g_current_hvac_cmd =
        CMD_OFF;

    fan_auto = true;

    safety_latch = false;


    /*
     * Ensure outputs are safe.
     */

    safe_shutdown_actuators();


    /*
     * Start FSM task.
     */

    BaseType_t result =
        xTaskCreate(
            state_machine_task,
            "hvac_fsm",
            4096,
            NULL,
            5,
            NULL
        );

    if (result != pdPASS) {

        ESP_LOGE(
            TAG,
            "Failed to create FSM task"
        );

        return ESP_FAIL;
    }


    ESP_LOGI(
        TAG,
        "HVAC state machine initialized"
    );

    return ESP_OK;
}


/* ============================================================
 * State Machine Task
 * ============================================================ */

static void state_machine_task(
    void *pvParameters)
{
    (void)pvParameters;

    hvac_cmd_t event;


    ESP_LOGI(
        TAG,
        "HVAC FSM task started"
    );


    while (1) {

        /*
         * Block until an event arrives.
         */

        if (xQueueReceive(
                hvac_queue,
                &event,
                portMAX_DELAY) != pdTRUE) {

            continue;
        }


        g_current_hvac_cmd =
            event;


        ESP_LOGI(
            TAG,
            "EVENT=%d STATE=%d",
            event,
            g_current_hvac_state
        );


        /* ====================================================
         * Global OFF
         * ==================================================== */

        if (event == CMD_OFF) {

            safe_shutdown_actuators();

            safety_latch = false;

            fan_auto = true;

            update_fault(FLT_NONE);

            update_state(
                STATE_IDLE
            );

            ESP_LOGI(
                TAG,
                "System returned to IDLE"
            );

            continue;
        }


        /* ====================================================
         * Fault Lockout
         * ==================================================== */

        if (safety_latch) {

            safe_shutdown_actuators();

            update_state(
                STATE_FAULT
            );

            continue;
        }


        /* ====================================================
         * Heat OFF
         * ==================================================== */

        if (
            event == CMD_HEAT_OFF &&
            (
                g_current_hvac_state == STATE_RUNNING ||
                g_current_hvac_state == STATE_WARMUP ||
                g_current_hvac_state == STATE_VERIFY_RPM ||
                g_current_hvac_state == STATE_IGNITION
            )
        ) {

            heating_complete();

            continue;
        }


        /* ====================================================
         * State Processing
         * ==================================================== */

        switch (g_current_hvac_state) {


        /* ====================================================
         * IDLE
         * ==================================================== */

        case STATE_IDLE:

            if (event == CMD_HEAT) {

                set_heater_state(1);
                set_fan_state(0);
                start_flame_check();

                update_state(
                    STATE_IGNITION
                );
            }


            else if (event == CMD_FAN_ON) {

                fan_auto = false;

                set_fan_state(1);

                update_state(
                    STATE_FAN_CIRCULATE
                );
            }


            else if (event == CMD_FAN_AUTO) {

                fan_auto = true;

                set_fan_state(0);
            }

            break;


        /* ====================================================
         * FAN CIRCULATE
         * ==================================================== */

        case STATE_FAN_CIRCULATE:
            set_fan_state(1);
            if (event == CMD_FAN_AUTO) {

                fan_auto = true;

                set_fan_state(0);

                update_state(
                    STATE_IDLE
                );
            }


            else if (event == CMD_HEAT) {

                set_heater_state(1);
                set_fan_state(0);
                start_flame_check();

                update_state(
                    STATE_IGNITION
                );
            }

            break;


        /* ====================================================
         * IGNITION
         * ==================================================== */

        case STATE_IGNITION:

            if (event == CMD_FLAME_DETECTED) {

                stop_flame_check();

                xTimerStart(
                    fan_warmup_timer,
                    0
                );

                update_state(
                    STATE_WARMUP
                );
            }


            else if (event == CMD_FLAME_TIMEOUT) {

                stop_flame_check();

                update_fault(
                    FLT_FLAME
                );
            }

            break;


        /* ====================================================
         * WARMUP
         * ==================================================== */

        case STATE_WARMUP:

            if (event == CMD_WARMUP_DONE) {

                set_fan_state(1);

                start_fan_check();

                update_state(
                    STATE_VERIFY_RPM
                );
            }

            break;


        /* ====================================================
         * VERIFY RPM
         * ==================================================== */

        case STATE_VERIFY_RPM:

            if (event == CMD_FAN_OK) {

                stop_fan_check();

                update_state(
                    STATE_RUNNING
                );
            }


            else if (event == CMD_TACH_TIMEOUT) {

                stop_fan_check();

                update_fault(
                    FLT_FAN
                );
            }

            break;


        /* ====================================================
         * RUNNING
         * ==================================================== */

        case STATE_RUNNING:

            if (event == CMD_FAN_AUTO) {

                fan_auto = true;
            }


            else if (event == CMD_FAN_ON) {

                fan_auto = false;
            }

            break;


        /* ====================================================
         * COOLDOWN
         * ==================================================== */

        case STATE_COOLDOWN:

            set_heater_state(0);

            set_fan_state(1);


            if (event == CMD_COOLDOWN_DONE) {

                ESP_LOGI(
                    TAG,
                    "Cooldown complete"
                );


                if (fan_auto) {

                    set_fan_state(0);

                    update_state(
                        STATE_IDLE
                    );
                }

                else {

                    update_state(
                        STATE_FAN_CIRCULATE
                    );
                }
            }


            else if (event == CMD_HEAT) {

                /*
                 * Re-ignition during cooldown.
                 */

                xTimerStop(
                    fan_cooldown_timer,
                    0
                );

                set_heater_state(1);

                start_flame_check();

                update_state(
                    STATE_IGNITION
                );
            }

            break;


        /* ====================================================
         * FAULT
         * ==================================================== */

        case STATE_FAULT:

            safe_shutdown_actuators();

            break;


        /* ====================================================
         * Invalid State
         * ==================================================== */

        default:

            safe_shutdown_actuators();

            update_state(
                STATE_IDLE
            );

            break;
        }
    }
}


/* ============================================================
 * Get State
 * ============================================================ */

hvac_state_t hvac_get_state(void)
{
    hvac_state_t state =
        STATE_IDLE;


    if (hvac_state_mutex != NULL) {

        if (
            xSemaphoreTake(
                hvac_state_mutex,
                portMAX_DELAY
            ) == pdTRUE
        ) {

            state =
                g_current_hvac_state;

            xSemaphoreGive(
                hvac_state_mutex
            );
        }
    }


    return state;
}


/* ============================================================
 * Get Fault
 * ============================================================ */

hvac_flt_t hvac_get_fault(void)
{
    hvac_flt_t fault =
        FLT_NONE;


    if (hvac_state_mutex != NULL) {

        if (
            xSemaphoreTake(
                hvac_state_mutex,
                portMAX_DELAY
            ) == pdTRUE
        ) {

            fault =
                g_current_hvac_fault;

            xSemaphoreGive(
                hvac_state_mutex
            );
        }
    }


    return fault;
}


/* ============================================================
 * Get Command
 * ============================================================ */

hvac_cmd_t hvac_get_command(void)
{
    hvac_cmd_t cmd =
        CMD_OFF;


    if (hvac_state_mutex != NULL) {

        if (
            xSemaphoreTake(
                hvac_state_mutex,
                portMAX_DELAY
            ) == pdTRUE
        ) {

            cmd =
                g_current_hvac_cmd;

            xSemaphoreGive(
                hvac_state_mutex
            );
        }
    }


    return cmd;
}