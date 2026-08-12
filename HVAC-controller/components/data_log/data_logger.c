#include "data_logger.h"

static const char *TAG = "DATA_LOGGER";
static QueueHandle_t logger_queue = NULL;

void data_logger_init(void) {
    logger_queue = xQueueCreate(10, sizeof(log_msg_t));
    if (logger_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create logger queue");
        return;
    }

    log_timer = xTimerCreate("log interval", pdMS_TO_TICKS(1000), pdTRUE, NULL, log_timer_callback);
    if (log_timer == NULL) {
        ESP_LOGE(TAG, "Failed to cerate logger timer");
        return;
    }

    xTaskCreate(data_logger_task, "data_logger", 2048, NULL, 5, NULL);
    xTimerStart(log_timer, portMAX_DELAY);
}

static void collect_log_data(hvac_log_data_t *data) {

    log_data.state = g_current_hvac_state();
    log_data.fault = g_current_hvac_fault();
    log_data.fan_state = get_fan_state();
    log_data.heater_state = get_heater_state();
    log_data.flame_value = flame_sensor.value();
    log_data.fan_rpm = tach_sensor.fan_rpm();

}

static void log_timer_callback(TimerHandle_t xTimer) {

    log_msg_t msg;
    msg.type = LOG_EVENT_PEROIDIC;

    if (logger_queue != NULL) {
        xQueueSend(logger_queue, &message, 0);
    }
}

static void data_logger_write(const hvac_log_data_t *data) {

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
        "%02d:%02d:%02s,"
        
        "%d,"
        "%d,"
        "%d,"
        "%d,"
        "%f,"
        "%f\n",

        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,

        timeinfo_tm.hour,
        timeinfo.tm_min,
        timeinfo.tm_sec,

        
        (int)state,
        (int)fault,
        fan_state,
        heater_state,
        flame_value,
        fan_rpm,
    );
}

void data_logger_task(hvac_log_data_t *log_data) {



        
        
        data_logger_log(&log_data);

    }

    void data_logger_log_event(log_event_type_t type) {

        log_message_t = msg;
        msg.type = type;
        xQueueSend(logger_queue, &message, 0);
    }
