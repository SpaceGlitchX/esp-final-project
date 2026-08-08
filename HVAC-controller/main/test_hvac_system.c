#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "hvac_states.h"
#include "flame_sensor.h"
#include "tach_sensor.h"

static const char *TAG = "HVAC_TEST";

// Shared pointers/handles declared in your main core logic
extern QueueHandle_t hvac_queue;
extern hvac_state_t g_current_hvac_state;
extern hvac_flt_t g_current_hvac_fault;

// Hardware/Sensor struct instances (if applicable)
extern FlameSensor flame_sensor;
extern TachSensor tach_sensor;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Directly posts a command event to the FSM queue
static void inject_cmd(hvac_cmd_t cmd) {
    xQueueSend(hvac_queue, &cmd, pdMS_TO_TICKS(100));
    vTaskDelay(pdMS_TO_TICKS(50)); // Allow FSM task time to process
}

// Blocks until expected state is reached or timeout occurs
static bool assert_state_timeout(hvac_state_t expected_state, uint32_t timeout_ms) {
    uint32_t elapsed = 0;
    while (elapsed < timeout_ms) {
        if (g_current_hvac_state == expected_state) {
            ESP_LOGI(TAG, "[PASS] State transitioned to %d", expected_state);
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        elapsed += 100;
    }
    ESP_LOGE(TAG, "[FAIL] Expected State %d, but timed out in State %d", expected_state, g_current_hvac_state);
    return false;
}

// ============================================================================
// TEST CASES
// ============================================================================

// TEST 1: Basic Fan Only Mode Cycle
static void test_fan_circulate_mode(void) {
    ESP_LOGI(TAG, "=== TEST 1: FAN CIRCULATE MODE ===");

    inject_cmd(CMD_FAN_ONLY);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_FAN_CIRCULATE, 1000));

    inject_cmd(CMD_OFF);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_IDLE, 1000));
}

// TEST 2: Complete Ignition & Warmup Sequence
static void test_normal_heating_cycle(void) {
    ESP_LOGI(TAG, "=== TEST 2: FULL HEATING CYCLE ===");

    // 1. Request Heat
    inject_cmd(CMD_HEAT);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_IGNITION, 1000));

    // 2. Simulate Flame Detected Sensor Reading
    flame_sensor.base.value = 2500; // Above flame threshold
    inject_cmd(CMD_FLAME_DETECTED);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_WARMUP, 1000));

    // 3. Simulate Warmup Complete Timer Event
    inject_cmd(CMD_WARMUP_DONE);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_VERIFY_RPM, 1000));

    // 4. Simulate Fan Tachometer Verified
    tach_sensor.fan_rpm = 1500; // Above min RPM threshold
    inject_cmd(CMD_RPM_VERIFIED);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_RUNNING, 1000));

    // 5. Normal Shutdown
    inject_cmd(CMD_OFF);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_IDLE, 1000));
}

// TEST 3: Ignition Failure (Flame Timeout) & Safety Lockout
static void test_flame_fault_lockout(void) {
    ESP_LOGI(TAG, "=== TEST 3: FLAME FAULT LOCKOUT & RESET ===");

    inject_cmd(CMD_HEAT);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_IGNITION, 1000));

    // Simulate Flame Proving Timeout (Flame failed to ignite)
    flame_sensor.base.value = 100; // Below threshold
    inject_cmd(CMD_FLAME_TIMEOUT);

    // Assert system locks out in FAULT state
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_FAULT, 1000));
    TEST_ASSERT_EQUAL(FLT_FLAME, g_current_hvac_fault);

    // Verify FSM ignores further HEAT commands while locked out
    inject_cmd(CMD_HEAT);
    vTaskDelay(pdMS_TO_TICKS(200));
    TEST_ASSERT_EQUAL(STATE_FAULT, g_current_hvac_state);

    // Send OFF command to clear fault
    inject_cmd(CMD_OFF);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_IDLE, 1000));
    TEST_ASSERT_EQUAL(FLT_NONE, g_current_hvac_fault);
}

// TEST 4: Blower Fan Tachometer Timeout Failure
static void test_tach_fault_lockout(void) {
    ESP_LOGI(TAG, "=== TEST 4: TACHOMETER RPM FAULT ===");

    // Fast-forward to VERIFY_RPM
    inject_cmd(CMD_HEAT);
    inject_cmd(CMD_FLAME_DETECTED);
    inject_cmd(CMD_WARMUP_DONE);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_VERIFY_RPM, 1000));

    // Simulate Tach Failure Timeout (Fan didn't reach target RPM)
    tach_sensor.fan_rpm = 0;
    inject_cmd(CMD_TACH_TIMEOUT);

    // Assert FAULT state & correct fault flag
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_FAULT, 1000));
    TEST_ASSERT_EQUAL(FLT_FAN, g_current_hvac_fault);

    // Clear Fault
    inject_cmd(CMD_OFF);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_IDLE, 1000));
}

// ============================================================================
// TEST RUNNER TASK
// ============================================================================
void hvac_test_runner_task(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for FSM task to initialize

    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "       STARTING HVAC DIRECT-QUEUE TEST SUITE      ");
    ESP_LOGI(TAG, "==================================================");

    UNITY_BEGIN();

    RUN_TEST(test_fan_circulate_mode);
    RUN_TEST(test_normal_heating_cycle);
    RUN_TEST(test_flame_fault_lockout);
    RUN_TEST(test_tach_fault_lockout);

    UNITY_END();

    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "              ALL TESTS COMPLETED                 ");
    ESP_LOGI(TAG, "==================================================");

    vTaskDelete(NULL);
}