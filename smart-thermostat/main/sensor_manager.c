#include "sensor_manager.h"

extern TempSensor temp_sensor;

TimerHandle_t temp_read_timer = NULL;
QueueHandle_t temp_queue = NULL;
float current_temp;
static void temp_read_timer_callback(TimerHandle_t xTimer) {
	if (temp_sensor.read != NULL) {
        temp_sensor.read(&temp_sensor);
		current_temp = temp_sensor.temp;
		xQueueSend(temp_queue, &current_temp, 0);
    }
}
void sensor_manager_init(void) {
	current_temp = 0.0;
	temp_queue = xQueueCreate(10, sizeof(uint32_t));

	if (temp_sensor.init != NULL) {
		temp_sensor.init(&temp_sensor);
	}

	temp_read_timer = xTimerCreate("TEMP_READ_TIMER", pdMS_TO_TICKS(5000), pdTRUE, NULL, temp_read_timer_callback);

	xTimerStart(temp_read_timer, 1000);
}