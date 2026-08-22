#include "sensor_manager.h"

#include "esp_log.h"

#include "hvac_state_machine.h"

static const char *TAG = "SENSOR_MANAGER";


/* ============================================================
 * Global Sensor Values
 * ============================================================ */

uint32_t current_rpm = 0;
uint16_t current_adc = 0;


/* ============================================================
 * Sensor Timers
 * ============================================================ */

static TimerHandle_t flame_check_timer = NULL;
static TimerHandle_t fan_check_timer = NULL;


/* ============================================================
 * Thresholds
 * ============================================================ */

/*
 * Flame ADC threshold.
 *
 * ADC range = 0 to 4095.
 */
#define FLAME_THRESHOLD 30


/*
 * Minimum acceptable fan RPM.
 *
 * Adjust this based on your fan's measured operating speed.
 */


/*
 * Time allowed for flame to appear.
 */
#define FLAME_TIMEOUT_MS 5000


/*
 * Time allowed for fan RPM verification.
 */
#define FAN_TIMEOUT_MS 5000


/* ============================================================
 * Flame Timer Callback
 * ============================================================ */

static void flame_check_callback(
    TimerHandle_t xTimer)
{
    (void)xTimer;

    current_adc =
        (uint16_t)get_flame_sensor_adc();


    ESP_LOGI(
        TAG,
        "Flame ADC = %u",
        current_adc
    );


    if (current_adc < FLAME_THRESHOLD) {

        hvac_cmd_t cmd =
            CMD_FLAME_DETECTED;

        if (hvac_queue != NULL) {

            xQueueSend(
                hvac_queue,
                &cmd,
                0
            );
        }

        /*
         * Flame has been detected.
         *
         * Stop this monitoring timer.
         */

        xTimerStop(
            flame_check_timer,
            0
        );
    }
}


/* ============================================================
 * Fan Timer Callback
 * ============================================================ */

static void fan_check_callback(
    TimerHandle_t xTimer)
{
    (void)xTimer;

    current_rpm =
        get_tach_sensor_rpm();


    ESP_LOGI(
        TAG,
        "Fan RPM = %.1f",
        current_rpm
    );


    if (current_rpm >= FAN_MIN_RPM) {

        hvac_cmd_t cmd =
            CMD_FAN_OK;

        if (hvac_queue != NULL) {

            xQueueSend(
                hvac_queue,
                &cmd,
                0
            );
        }

        /*
         * Fan passed verification.
         */

        xTimerStop(
            fan_check_timer,
            0
        );
    }
}


/* ============================================================
 * Flame Timeout Task
 * ============================================================ */

static void flame_timeout_callback(
    TimerHandle_t xTimer)
{
    (void)xTimer;

    /*
     * Check one final time.
     */

    current_adc =
        (uint16_t)get_flame_sensor_adc();


    if (current_adc < FLAME_THRESHOLD) {

        hvac_cmd_t cmd =
            CMD_FLAME_TIMEOUT;

        if (hvac_queue != NULL) {

            xQueueSend(
                hvac_queue,
                &cmd,
                0
            );
        }
    }
}


/* ============================================================
 * Fan Timeout
 * ============================================================ */

static void fan_timeout_callback(
    TimerHandle_t xTimer)
{
    (void)xTimer;

    current_rpm =
        get_tach_sensor_rpm();


    if (current_rpm < FAN_MIN_RPM) {

        hvac_cmd_t cmd =
            CMD_TACH_TIMEOUT;

        if (hvac_queue != NULL) {

            xQueueSend(
                hvac_queue,
                &cmd,
                0
            );
        }
    }
}


/* ============================================================
 * Initialization
 * ============================================================ */

esp_err_t sensor_manager_init(void)
{
    esp_err_t err;


    /* Flame sensor */

    err = flame_sensor_init();

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Flame sensor initialization failed"
        );

        return err;
    }


    /* Tachometer */

    err = tach_sensor_init();

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Tachometer initialization failed"
        );

        return err;
    }


    /* Flame monitoring timer */

    flame_check_timer =
        xTimerCreate(
            "flame_check",
            pdMS_TO_TICKS(100),
            pdTRUE,
            NULL,
            flame_check_callback
        );

    if (flame_check_timer == NULL) {
        return ESP_FAIL;
    }


    /* Fan monitoring timer */

    fan_check_timer =
        xTimerCreate(
            "fan_check",
            pdMS_TO_TICKS(RPM_SAMPLE_TIME_MS),
            pdTRUE,
            NULL,
            fan_check_callback
        );

    if (fan_check_timer == NULL) {
        return ESP_FAIL;
    }


    ESP_LOGI(
        TAG,
        "Sensor manager initialized"
    );

    return ESP_OK;
}


/* ============================================================
 * Start Flame Check
 * ============================================================ */

void start_flame_check(void)
{
    if (flame_check_timer == NULL) {
        return;
    }

    ESP_LOGI(
        TAG,
        "Starting flame verification"
    );


    /*
     * Check frequently while waiting for flame.
     */

    xTimerChangePeriod(
        flame_check_timer,
        pdMS_TO_TICKS(100),
        0
    );

    xTimerStart(
        flame_check_timer,
        0
    );


    /*
     * Separate one-shot timeout.
     *
     * This creates a temporary timer.
     */

    TimerHandle_t timeout_timer =
        xTimerCreate(
            "flame_timeout",
            pdMS_TO_TICKS(FLAME_TIMEOUT_MS),
            pdFALSE,
            NULL,
            flame_timeout_callback
        );

    if (timeout_timer != NULL) {

        xTimerStart(
            timeout_timer,
            0
        );
    }
}


/* ============================================================
 * Stop Flame Check
 * ============================================================ */

void stop_flame_check(void)
{
    if (flame_check_timer != NULL) {

        xTimerStop(
            flame_check_timer,
            0
        );
    }

    ESP_LOGI(
        TAG,
        "Flame verification stopped"
    );
}


/* ============================================================
 * Start Fan Check
 * ============================================================ */

void start_fan_check(void)
{
    if (fan_check_timer == NULL) {
        return;
    }

    ESP_LOGI(
        TAG,
        "Starting fan RPM verification"
    );


    xTimerChangePeriod(
        fan_check_timer,
        pdMS_TO_TICKS(RPM_SAMPLE_TIME_MS),
        0
    );


    xTimerStart(
        fan_check_timer,
        0
    );


    /*
     * Separate fan timeout.
     */

    TimerHandle_t timeout_timer =
        xTimerCreate(
            "fan_timeout",
            pdMS_TO_TICKS(FAN_TIMEOUT_MS),
            pdFALSE,
            NULL,
            fan_timeout_callback
        );

    if (timeout_timer != NULL) {

        xTimerStart(
            timeout_timer,
            0
        );
    }
}


/* ============================================================
 * Stop Fan Check
 * ============================================================ */

void stop_fan_check(void)
{
    if (fan_check_timer != NULL) {

        xTimerStop(
            fan_check_timer,
            0
        );
    }

    ESP_LOGI(
        TAG,
        "Fan RPM verification stopped"
    );
}


/* ============================================================
 * Getters
 * ============================================================ */

uint32_t sensor_get_rpm(void)
{
    return current_rpm;
}


uint16_t sensor_get_flame_adc(void)
{
    return current_adc;
}