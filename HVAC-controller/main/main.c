#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "hvac_state_machine.h"
#include "hardware_manager.h"
#include "sensor_manager.h"

// Forward declaration of test runner task
extern void hvac_test_runner_task(void *pvParameters);

void app_main(void) {
    ESP_LOGI("MAIN", "Initializing HVAC Subsystems...");

    // 1. Initialize Hardware & Timers
    init_hvac_hardware();
    sensor_manager_init();

    // 2. Initialize FSM Queue & Pacing Timers
    if (hvac_state_machine_init() != ESP_OK) {
        ESP_LOGE("MAIN", "Failed to initialize HVAC FSM!");
        return;
    }

    // 3. Create FSM Background Task (MUST run alongside test task!)
    xTaskCreatePinnedToCore(
        hvac_state_machine_task,
        "hvac_fsm_task",
        4096,
        NULL,
        5,              // Priority 5
        NULL,
        0               // Core 0
    );

    // 4. Spawn Test Runner Task
    xTaskCreatePinnedToCore(
        hvac_test_runner_task,
        "hvac_test_runner",
        4096,
        NULL,
        4,              // Slightly lower priority than FSM so FSM processes events quickly
        NULL,
        0
    );
}