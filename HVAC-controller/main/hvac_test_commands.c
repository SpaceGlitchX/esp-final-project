#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor_manager.h"
#include "hardware_manager.h"
#include "hvac_state_machine.h"
#include "hvac_comms.h"


#include "esp_log.h"

#include "hvac_state_machine.h"

static const char *TAG = "HVAC_TEST";


static void send_test_command(hvac_cmd_t cmd)
{
    if (hvac_queue == NULL) {
        ESP_LOGE(TAG, "HVAC queue not initialized!");
        return;
    }

    if (xQueueSend(hvac_queue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to send command!");
    }
}


/*
 * Automated FSM test
 */
static void hvac_test_task(void *pvParameters)
{
    (void)pvParameters;

    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "HVAC FSM TEST START");
    ESP_LOGI(TAG, "================================");

    /*
     * Start in IDLE
     */
    vTaskDelay(pdMS_TO_TICKS(2000));


    /*
     * TEST 1:
     * Request heating
     *
     * IDLE -> WAIT_DELAY -> IGNITION
     */
    ESP_LOGI(TAG, "TEST 1: Request HEAT");

    send_test_command(CMD_HEAT);

    vTaskDelay(pdMS_TO_TICKS(15000));




    /*
     * TEST 5:
     * Emergency shutdown
     *
     * RUNNING -> IDLE
     */
    ESP_LOGI(TAG, "TEST 5: Emergency OFF");

    send_test_command(CMD_OFF);

    vTaskDelay(pdMS_TO_TICKS(20000));


    /*
     * TEST 6:
     * Test flame failure
     */
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "TEST 6: FLAME FAILURE");
    ESP_LOGI(TAG, "================================");

    send_test_command(CMD_HEAT);

    vTaskDelay(pdMS_TO_TICKS(15000));



    /*
     * Reset system
     */
    ESP_LOGI(TAG, "Resetting after flame fault");

    send_test_command(CMD_OFF);

    vTaskDelay(pdMS_TO_TICKS(2000));


    /*
     * TEST 7:
     * Test fan failure
     */
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "TEST 7: FAN FAILURE");
    ESP_LOGI(TAG, "================================");

    send_test_command(CMD_HEAT);

    vTaskDelay(pdMS_TO_TICKS(1500));




    /*
     * Final shutdown
     */
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "HVAC FSM TEST COMPLETE");
    ESP_LOGI(TAG, "================================");

    send_test_command(CMD_OFF);


    vTaskDelete(NULL);
}

void app_main(void)
{
    hvac_state_machine_init();
    xTaskCreate(
        hvac_state_machine_task,
        "hvac_fsm",
        4096,
        NULL,
        5,
        NULL
    );
    init_hvac_hardware();
    sensor_manager_init();
    uart_setup();

    xTaskCreate(
            hvac_test_task,
            "test",
            4096,
            NULL,
            3,
            NULL
        );
   
}