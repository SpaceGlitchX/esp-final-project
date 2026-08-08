#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "components/hvac_states.h"


QueueHandle_t hvac_queue = NULL;
hvac_state_t g_current_hvac_state = STATE_IDLE;
hvac_flt_t g_current_hvac_fault = FLT_NONE;

extern void hvac_state_machine_task(void *pvParameters);
extern void hvac_test_runner_task(void *pvParameters);

void app_main(void) {
    // 1. Create FreeRTOS Event Queue
    hvac_queue = xQueueCreate(10, sizeof(hvac_cmd_t));

    // 2. Start Core FSM Task
    xTaskCreate(hvac_state_machine_task, "fsm_task", 4096, NULL, 10, NULL);

    // 3. Launch Test Suite Task
    xTaskCreate(hvac_test_runner_task, "test_runner", 4096, NULL, 5, NULL);
}