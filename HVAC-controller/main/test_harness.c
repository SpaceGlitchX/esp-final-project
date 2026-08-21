#include "sensor_manager.h"
#include "hardware_manager.h"
#include "hvac_state_machine.h"
#include "hvac_comms.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>

void test_harness_task(void *pvParameters)
{
    (void)pvParameters;

    hvac_cmd_t cmd;

    while (1)
    {
        /* =========================================
         * TEST 1: FAN ONLY
         * ========================================= */

        cmd = CMD_FAN_ONLY;

        if (xQueueSend(
                hvac_queue,
                &cmd,
                pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            printf("TEST 1: CMD_FAN_ONLY\n");
        }
        else
        {
            printf("TEST 1 FAILED: Queue full\n");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));


        /* =========================================
         * TEST 2: HEAT
         * ========================================= */

        cmd = CMD_HEAT;

        if (xQueueSend(
                hvac_queue,
                &cmd,
                pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            printf("TEST 2: CMD_HEAT\n");
        }
        else
        {
            printf("TEST 2 FAILED: Queue full\n");
        }

        /*
         * Give the furnace plenty of time to:
         *
         * IDLE
         *   ↓
         * WAIT_DELAY
         *   ↓
         * IGNITION
         *   ↓
         * WARMUP
         *   ↓
         * VERIFY_RPM
         *   ↓
         * RUNNING
         *
         * Actual completion depends on flame and tach
         * sensor events.
         */

        vTaskDelay(pdMS_TO_TICKS(50000));


        /* =========================================
         * TEST 3: OFF
         * ========================================= */

        cmd = CMD_OFF;

        if (xQueueSend(
                hvac_queue,
                &cmd,
                pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            printf("TEST 3: CMD_OFF\n");
        }
        else
        {
            printf("TEST 3 FAILED: Queue full\n");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));


        /*
         * Test sequence completed.
         * Delete this task so it doesn't repeat.
         */

        printf("TEST HARNESS COMPLETE\n");

        vTaskDelete(NULL);
    }
}