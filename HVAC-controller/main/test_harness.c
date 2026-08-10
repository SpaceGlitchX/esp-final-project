#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Includes from your project architecture
#include "hvac_state_machine.h"
#include "hvac_states.h"

static const char *TAG_TEST = "HVAC_TEST_HARNESS";

// Helper strings for logging status
static const char* state_to_str(hvac_state_t state) {
    switch (state) {
        case STATE_IDLE:          return "STATE_IDLE";
        case STATE_WAIT_DELAY:    return "STATE_WAIT_DELAY";
        case STATE_IGNITION:      return "STATE_IGNITION";
        case STATE_WARMUP:        return "STATE_WARMUP";
        case STATE_VERIFY_RPM:    return "STATE_VERIFY_RPM";
        case STATE_RUNNING:       return "STATE_RUNNING";
        case STATE_FAN_CIRCULATE: return "STATE_FAN_CIRCULATE";
        case STATE_FAULT:         return "STATE_FAULT";
        default:                  return "UNKNOWN";
    }
}

static const char* fault_to_str(hvac_flt_t fault) {
    switch (fault) {
        case FLT_NONE:  return "FLT_NONE";
        case FLT_FLAME: return "FLT_FLAME (Flame Ignition Failure)";
        case FLT_FAN:   return "FLT_FAN (Blower RPM Verification Failure)";
        default:        return "FLT_UNKNOWN";
    }
}

static void send_event(hvac_cmd_t cmd, const char* name) {
    if (hvac_queue != NULL) {
        printf("\n[THERMOSTAT SENDER] Sending Event: %s\n", name);
        xQueueSend(hvac_queue, &cmd, portMAX_DELAY);
    } else {
        ESP_LOGE(TAG_TEST, "HVAC Queue is NULL!");
    }
}

static void print_help(void) {
    printf("\n=================================================================\n");
    printf("                  HVAC FSM MANUAL TEST HARNESS\n");
    printf("=================================================================\n");
    printf(" Thermostat Control Commands:\n");
    printf("  heat       : Send CMD_HEAT (Initiate Heat Sequence)\n");
    printf("  fan        : Send CMD_FAN_ONLY (Initiate Fan Circulation)\n");
    printf("  off        : Send CMD_OFF (Shutdown System / Clear Lockout)\n\n");
    printf(" Sensor / Timer Event Injections:\n");
    printf("  flame_ok   : Send CMD_FLAME_DETECTED (Simulate Sensor Active)\n");
    printf("  flame_tout : Send CMD_FLAME_TIMEOUT (Simulate Ignition Fail)\n");
    printf("  warmup_ok  : Send CMD_WARMUP_DONE (Simulate Heat Delay Complete)\n");
    printf("  fan_ok     : Send CMD_FAN_OK (Simulate TACH RPM Valid)\n");
    printf("  fan_tout   : Send CMD_TACH_TIMEOUT (Simulate Blower Fail)\n\n");
    printf(" Diagnostic Commands:\n");
    printf("  status     : Print Current FSM State and Active Faults\n");
    printf("  help       : Display this control menu\n");
    printf("=================================================================\n\n");
}

void app_main(void)
{
    // 1. Initialize Hardware & Timers
    init_hvac_hardware();
    sensor_manager_init();
    // 1. Initialize State Machine Resources
    if (hvac_state_machine_init() != ESP_OK) {
        ESP_LOGE(TAG_TEST, "Failed to initialize state machine!");
        return;
    }

    // 2. Launch HVAC State Machine Task
    xTaskCreate(hvac_state_machine_task, "hvac_fsm_task", 4096, NULL, 5, NULL);

    vTaskDelay(pdMS_TO_TICKS(100)); // Brief pause for print alignment
    print_help();

    char line[64];
    while (1) {
        printf("THERMOSTAT_MOCK> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) != NULL) {
            // Strip trailing newlines/returns
            line[strcspn(line, "\r\n")] = 0;

            if (strlen(line) == 0) continue;

            // Normalize input to lowercase
            for (int i = 0; line[i]; i++) line[i] = tolower((unsigned char)line[i]);

            // Map commands to queues
            if (strcmp(line, "heat") == 0) {
                send_event(CMD_HEAT, "CMD_HEAT");
            } 
            else if (strcmp(line, "fan") == 0) {
                send_event(CMD_FAN_ONLY, "CMD_FAN_ONLY");
            } 
            else if (strcmp(line, "off") == 0) {
                send_event(CMD_OFF, "CMD_OFF");
            } 
            else if (strcmp(line, "flame_ok") == 0) {
                send_event(CMD_FLAME_DETECTED, "CMD_FLAME_DETECTED");
            } 
            else if (strcmp(line, "flame_tout") == 0) {
                send_event(CMD_FLAME_TIMEOUT, "CMD_FLAME_TIMEOUT");
            } 
            else if (strcmp(line, "warmup_ok") == 0) {
                send_event(CMD_WARMUP_DONE, "CMD_WARMUP_DONE");
            } 
            else if (strcmp(line, "fan_ok") == 0) {
                send_event(CMD_FAN_OK, "CMD_FAN_OK");
            } 
            else if (strcmp(line, "fan_tout") == 0) {
                send_event(CMD_TACH_TIMEOUT, "CMD_TACH_TIMEOUT");
            } 
            else if (strcmp(line, "status") == 0) {
                printf("\n--- CURRENT SYSTEM METRICS ---\n");
                printf(" State : %s (%d)\n", state_to_str(g_current_hvac_state), g_current_hvac_state);
                printf(" Fault : %s (%d)\n", fault_to_str(g_current_hvac_fault), g_current_hvac_fault);
                printf("------------------------------\n\n");
            } 
            else if (strcmp(line, "help") == 0) {
                print_help();
            } 
            else {
                printf("Unknown command: '%s'. Type 'help' for options.\n", line);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}