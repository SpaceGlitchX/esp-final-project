#include "data_logger.h"

#include <stdio.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "esp_log.h"

#include "sensor_manager.h"
#include "hardware_manager.h"
#include "hvac_state_machine.h"

static const char *TAG = "DATA_LOGGER";

static QueueHandle_t logger_queue = NULL;
static TimerHandle_t log_timer = NULL;

static void data_logger_task(void *pvParameters);
static void collect_log_data(hvac_log_data_t *data);
static void data_logger_write(const hvac_log_data_t *data,
                            log_event_type_t type);
static void log_timer_callback(TimerHandle_t xTimer);


void data_logger_init(void)
{
    logger_queue = xQueueCreate(10, sizeof(log_msg_t));

    if (logger_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create logger queue");
        return;
    }

    log_timer = xTimerCreate(
        "log interval",
        pdMS_TO_TICKS(1000),
        pdTRUE,
        NULL,
        log_timer_callback
    );

    if (log_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create logger timer");
        return;
    }

    BaseType_t task_result = xTaskCreate(
        data_logger_task,
        "data_logger",
        2048,
        NULL,
        5,
        NULL
    );

    if (task_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create logger task");
        return;
    }

    xTimerStart(log_timer, 0);
}


static void collect_log_data(hvac_log_data_t *data)
{
    if (data == NULL) {
        return;
    }

    data->state = g_current_hvac_state;
    data->fault = g_current_hvac_fault;
    data->fan_state = get_fan_state();
    data->heater_state = get_heater_state();

    /*
     * Replace these with the actual functions
     * provided by sensor_manager.h.
     */
    data->flame_value = flame_sensor.value;
    data->fan_rpm = tach_sensor.fan_rpm;
}


static void log_timer_callback(TimerHandle_t xTimer)
{
    log_msg_t msg;

    msg.type = LOG_EVENT_PERIODIC;

    if (logger_queue != NULL) {
        xQueueSend(logger_queue, &msg, 0);
    }
}


static void data_logger_write(const hvac_log_data_t *data,log_event_type_t type)
{
    if (data == NULL) {
        return;
    }

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    printf(
        "HVAC-DATA,"
        "%04d-%02d-%02d,"
        "%02d:%02d:%02d,"
        "%d,"
        "%d,"
        "%d,"
        "%d,"
        "%f,"
        "%f,"
        "%d\n",

        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,

        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec,

        (int)data->state,
        (int)data->fault,
        data->fan_state,
        data->heater_state,
        data->flame_value,
        data->fan_rpm,
        (int)type
    );
}


static void data_logger_task(void *pvParameters)
{
    log_msg_t msg;

    while (1) {

        if (xQueueReceive(
                logger_queue,
                &msg,
                portMAX_DELAY) == pdTRUE) {

            hvac_log_data_t data;

            collect_log_data(&data);

            data_logger_write(&data, msg.type);
        }
    }
}


void data_logger_log_event(log_event_type_t type)
{
    if (logger_queue == NULL) {
        ESP_LOGW(TAG, "Logger queue not initialized");
        return;
    }

    log_msg_t msg;
    msg.type = type;

    if (xQueueSend(logger_queue, &msg, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Logger queue full, event dropped");
    }
}