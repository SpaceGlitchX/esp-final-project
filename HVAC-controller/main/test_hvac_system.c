#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "unity.h"

#include "hvac_states.h"
#include "hvac_state_machine.h"
#include "flame_sensor.h"
#include "tach_sensor.h"

static const char *TAG = "HVAC_TEST";

// Shared global handles and state variables from core engine
extern QueueHandle_t hvac_queue;
extern hvac_state_t g_current_hvac_state;
extern hvac_flt_t g_current_hvac_fault;

// Hardware/Sensor struct instances
extern FlameSensor flame_sensor;
extern TachSensor tach_sensor;

// Default timeout threshold to accommodate state pacing delays (500ms - 1000ms)
#define FSM_TEST_TIMEOUT_MS 2500

// ============================================================================
// HELPER FUNCTIONS & TEST FIXTURES
// ============================================================================

/**
 * @brief Helper to flush any remaining stale events in the FreeRTOS queue.
 */
static void flush_hvac_queue(void) {
    hvac_cmd_t dummy_cmd;
    if (hvac_queue != NULL) {
        while (xQueueReceive(hvac_queue, &dummy_cmd, 0) == pdTRUE) {
            // Drain queue
        }
    }
}

/**
 * @brief Unity Setup Fixture - Runs BEFORE every test case
 */
void setUp(void) {
    flush_hvac_queue();
    g_current_hvac_state = STATE_IDLE;
    g_current_hvac_fault = FLT_NONE;
}

/**
 * @brief Unity Teardown Fixture - Runs AFTER every test case
 */
void tearDown(void) {
    // Send OFF command to ensure actuators and pacing timers shut down safely
    hvac_cmd_t cmd = CMD_OFF;
    if (hvac_queue != NULL) {
        xQueueSend(hvac_queue, &cmd, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    flush_hvac_queue();
}

/**
 * @brief Injects a command event into the FSM queue and yields execution
 */
static void inject_cmd(hvac_cmd_t cmd) {
    if (hvac_queue != NULL) {
        xQueueSend(hvac_queue, &cmd, pdMS_TO_TICKS(1000));
        vTaskDelay(pdMS_TO_TICKS(100)); // Allow FSM task context switch to process
    }
}

/**
 * @brief Blocks until expected state is reached or timeout occurs
 */
static bool assert_state_timeout(hvac_state_t expected_state, uint32_t timeout_ms) {
    uint32_t elapsed = 0;
    while (elapsed < timeout_ms) {
        if (g_current_hvac_state == expected_state) {
            ESP_LOGI(TAG, "[PASS] State transitioned to %d", expected_state);
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
        elapsed += 50;
    }
    ESP_LOGE(TAG, "[FAIL] Expected State %d, but timed out in State %d", 
             expected_state, g_current_hvac_state);
    return false;
}

// ============================================================================
// TEST CASES
// ============================================================================

/**
 * TEST 1: Basic Fan Only Mode Cycle
 */
static void test_fan_circulate_mode(void) {
    ESP_LOGI(TAG, "=== TEST 1: FAN CIRCULATE MODE ===");

    inject_cmd(CMD_FAN_ONLY);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_FAN_CIRCULATE, FSM_TEST_TIMEOUT_MS));

    inject_cmd(CMD_OFF);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_IDLE, FSM_TEST_TIMEOUT_MS));
}

/**
 * TEST 2: Complete Ignition, Warmup, and Running Sequence
 */
static void test_normal_heating_cycle(void) {
    ESP_LOGI(TAG, "=== TEST 2: FULL HEATING CYCLE ===");

    // 1. Request Heat
    inject_cmd(CMD_HEAT);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_IGNITION, FSM_TEST_TIMEOUT_MS));

    // 2. Simulate Flame Proved Reading
    flame_sensor.value = 2500; // Above flame threshold
    inject_cmd(CMD_FLAME_DETECTED);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_WARMUP, FSM_TEST_TIMEOUT_MS));

    // 3. Simulate Warmup Complete Timer Event
    inject_cmd(CMD_WARMUP_DONE);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_VERIFY_RPM, FSM_TEST_TIMEOUT_MS));

    // 4. Simulate Fan Tachometer Verified
    tach_sensor.fan_rpm = 1500; // Above min RPM threshold
    inject_cmd(CMD_FAN_OK);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_RUNNING, FSM_TEST_TIMEOUT_MS));

    // 5. Normal Shutdown
    inject_cmd(CMD_OFF);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_IDLE, FSM_TEST_TIMEOUT_MS));
}

/**
 * TEST 3: Ignition Failure (Flame Timeout) & Safety Lockout
 */
static void test_flame_fault_lockout(void) {
    ESP_LOGI(TAG, "=== TEST 3: FLAME FAULT LOCKOUT & RESET ===");

    inject_cmd(CMD_HEAT);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_IGNITION, FSM_TEST_TIMEOUT_MS));

    // Simulate Flame Proving Timeout (Flame failed to ignite)
    flame_sensor.value = 100; // Below threshold
    inject_cmd(CMD_FLAME_TIMEOUT);

    // Assert system locks out in FAULT state
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_FAULT, FSM_TEST_TIMEOUT_MS));
    TEST_ASSERT_EQUAL(FLT_FLAME, g_current_hvac_fault);

    // Verify FSM ignores further HEAT commands while locked out
    inject_cmd(CMD_HEAT);
    vTaskDelay(pdMS_TO_TICKS(200));
    TEST_ASSERT_EQUAL(STATE_FAULT, g_current_hvac_state);

    // Send OFF command to clear fault
    inject_cmd(CMD_OFF);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_IDLE, FSM_TEST_TIMEOUT_MS));
    TEST_ASSERT_EQUAL(FLT_NONE, g_current_hvac_fault);
}

/**
 * TEST 4: Blower Fan Tachometer Timeout Failure
 */
static void test_tach_fault_lockout(void) {
    ESP_LOGI(TAG, "=== TEST 4: TACHOMETER RPM FAULT ===");

    // Step-by-step sequence to reach STATE_VERIFY_RPM cleanly
    inject_cmd(CMD_HEAT);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_IGNITION, FSM_TEST_TIMEOUT_MS));

    inject_cmd(CMD_FLAME_DETECTED);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_WARMUP, FSM_TEST_TIMEOUT_MS));

    inject_cmd(CMD_WARMUP_DONE);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_VERIFY_RPM, FSM_TEST_TIMEOUT_MS));

    // Simulate Tach Failure Timeout (Fan failed to reach speed)
    tach_sensor.fan_rpm = 0;
    inject_cmd(CMD_TACH_TIMEOUT);

    // Assert FAULT state & correct fault code
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_FAULT, FSM_TEST_TIMEOUT_MS));
    TEST_ASSERT_EQUAL(FLT_FAN, g_current_hvac_fault);

    // Clear Fault
    inject_cmd(CMD_OFF);
    TEST_ASSERT_TRUE(assert_state_timeout(STATE_IDLE, FSM_TEST_TIMEOUT_MS));
    TEST_ASSERT_EQUAL(FLT_NONE, g_current_hvac_fault);
}

// ============================================================================
// TEST RUNNER TASK
// ============================================================================

void hvac_test_runner_task(void *pvParameters) {
    (void)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(1000)); // Allow background FreeRTOS tasks to initialize

    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "       STARTING HVAC FSM UNITY TEST SUITE         ");
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