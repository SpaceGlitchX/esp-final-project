#include "sensor_manager.h"

static const char *TAG = "SENSOR_MGR";

// Static software timer handles
TimerHandle_t analog_sample_timer = NULL;
TimerHandle_t tach_monitor_timer  = NULL;

// Declare global sensor instances referenced by this module
extern FlameSensor flame_sensor;
extern TachSensor tach_sensor;

// External PCNT unit handle from tach_sensor.c
extern pcnt_unit_handle_t pcnt_unit;

/**
 * @brief Callback function for analog flame sample timer
 */
static void analog_flame_check_callback(TimerHandle_t xTimer) {
    (void)xTimer;
    if (flame_sensor.read != NULL) {
        flame_sensor.read(&flame_sensor);
    }
}

/**
 * @brief Callback function for tachometer sample timer
 */
static void tach_monitor_callback(TimerHandle_t xTimer) {
    (void)xTimer;
    if (tach_sensor.read != NULL) {
        tach_sensor.read(&tach_sensor);
    }
}

/**
 * @brief Initialize sensor hardware and FreeRTOS monitoring timers
 */
void sensor_manager_init(void) {
    if (flame_sensor.init != NULL) {
        flame_sensor.init(&flame_sensor);
    }
    if (tach_sensor.init != NULL) {
        tach_sensor.init(&tach_sensor);
    }

    // Create periodic timers for sampling
    analog_sample_timer = xTimerCreate(
        "ADC_SAMPLE_TIMER", 
        pdMS_TO_TICKS(100), 
        pdTRUE, 
        NULL, 
        analog_flame_check_callback
    );

    tach_monitor_timer = xTimerCreate(
        "TACH_SAMPLE_TIMER", 
        pdMS_TO_TICKS(100), 
        pdTRUE, 
        NULL, 
        tach_monitor_callback
    );

    if (analog_sample_timer == NULL || tach_monitor_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS sensor sampling timers!");
    } else {
        ESP_LOGI(TAG, "Sensor manager software timers created successfully.");
    }
}

/**
 * @brief Start the flame proving monitor
 */
void start_flame_proving_monitor(void) {
    if (analog_sample_timer != NULL) {
        xTimerStart(analog_sample_timer, portMAX_DELAY);
        ESP_LOGI(TAG, "Flame proving timer started.");
    }
}

/**
 * @brief Stop the flame proving monitor
 */
void stop_flame_proving_monitor(void) {
    if (analog_sample_timer != NULL) {
        xTimerStop(analog_sample_timer, portMAX_DELAY);
        ESP_LOGI(TAG, "Flame proving timer stopped.");
    }
}

/**
 * @brief Start tachometer monitoring and hardware pulse counter
 */
void start_tach_monitoring(void) {
    if (pcnt_unit != NULL) {
        pcnt_unit_start(pcnt_unit);
    }
    if (tach_monitor_timer != NULL) {
        xTimerStart(tach_monitor_timer, portMAX_DELAY);
        ESP_LOGI(TAG, "Tachometer monitoring started.");
    }
}

/**
 * @brief Stop tachometer monitoring and hardware pulse counter
 */
void stop_tach_monitoring(void) {
    if (pcnt_unit != NULL) {
        pcnt_unit_stop(pcnt_unit);
    }
    if (tach_monitor_timer != NULL) {
        xTimerStop(tach_monitor_timer, portMAX_DELAY);
        ESP_LOGI(TAG, "Tachometer monitoring stopped.");
    }
}