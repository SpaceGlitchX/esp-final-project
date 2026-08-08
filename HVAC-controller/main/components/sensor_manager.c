#include "sensor_manager.h"


/**
 * @class SensorManager
 * @brief Manages the initialization and monitoring of various sensors in the HVAC system.
 */

static TimerHandle_t analog_sample_timer = NULL;
static TimerHandle_t temp_sample_timer = NULL;
static TimerHandle_t tach_monitor_timer = NULL;

/**
 * @brief Callback function for analog sample timer
 */
static void analog_flame_check_callback(TimerHandle_t xTimer) {
    flame_sense.read();
}

/**
 * @brief Callback function for temperature sample timer
 */
static void tach_monitor_callback(TimerHandle_t xTimer) {
    tach_sense.read();
}

/**
 * @brief Initialize the sensors
 */
void sensor_manager_init(void) {
    flame_sense.init();
    tach_sense.init();

    analog_sample_timer = xTimerCreate("ADC_SAMPLE_TIMER", pdMS_TO_TICKS(100), pdTRUE, NULL, analog_flame_check_callback);
    tach_monitor_timer = xTimerCreate("TACH_SAMPLE_TIMER", pdMS_TO_TICKS(100), pdTRUE, NULL, tach_monitor_callback);
}

/**
 * @brief Start the flame proving monitor
 */
void start_flame_proving_monitor(void) {
    xTimerStart(analog_sample_timer, 0);
}

/**
 * @brief Stop the flame proving monitor
 */
void stop_flame_proving_monitor(void) {
    xTimerStop(analog_sample_timer, 0);
}

/**
 * @brief Start tachometer monitoring
 */
void start_tach_monitoring(void) {
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
    xTimerStart(tach_monitor_timer, 0);
}

/**
 * @brief Stop tachometer monitoring
 */
void stop_tach_monitoring(void) {
    ESP_ERROR_CHECK(pcnt_unit_stop(pcnt_unit));
    xTimerStop(tach_monitor_timer, 0);
}

