#include "thermo_logic.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "sensor_manager.h"
#include "thermo_comms.h"


/* ============================================================
 * GLOBAL DATA
 * ============================================================ */

float setpoint = SETPOINT_START;

/* ============================================================
 * SETPOINT
 * ============================================================ */

void set_setpoint(int level)
{
    if (level == 1)
    {
        setpoint += SETPOINT_STEP;

        if (setpoint > MAX_SETPOINT)
        {
            setpoint = MAX_SETPOINT;
        }
    }
    else
    {
        setpoint -= SETPOINT_STEP;

        if (setpoint < MIN_SETPOINT)
        {
            setpoint = MIN_SETPOINT;
        }
    }


    printf(
        "Setpoint: %.1f C\n",
        setpoint
    );
}


/* ============================================================
 * GET SETPOINT
 * ============================================================ */

float get_setpoint(void)
{
    return setpoint;
}


/* ============================================================
 * HEATING REQUIRED
 * ============================================================ */

bool heating_required(void)
{
    
    if (indoor_temp <
        (setpoint - DEADBAND_C))
    {
        return true;
    }


    return false;
}


/* ============================================================
 * THERMOSTAT CONTROL TASK
 * ============================================================ */

void thermo_control_task(void *pvParameters)
{
    (void)pvParameters;

    while (1)
    {
        /* ----------------------------------------------------
         * GET NEW TEMPERATURE
         * ---------------------------------------------------- */

        if (xQueueReceive(temp_queue, &current_temp, portMAX_DELAY) == pdTRUE) {
            indoor_temp = current_temp;
        }

        float current_setpoint = get_setpoint();

        /* ----------------------------------------------------
         * DISPLAY
         * ---------------------------------------------------- */

        printf(
            "\n"
            "============================================\n"
            "              THERMOSTAT DATA              \n"
            "============================================\n"
        );


        printf(
            "Indoor:       %.2f C\n",
            indoor_temp
        );


        printf(
            "Setpoint:     %.1f C\n",
            current_setpoint
        );


        /* ----------------------------------------------------
         * HEATING DECISION
         * ---------------------------------------------------- */

        if (indoor_temp <
            (current_setpoint - DEADBAND_C))
        {
            printf(
                "Heating:      REQUIRED\n"
            );
        }
        else if (
            indoor_temp >
            (current_setpoint + DEADBAND_C))
        {
            printf(
                "Heating:      NOT REQUIRED\n"
            );
        }
        else
        {
            printf(
                "Heating:      DEAD BAND\n"
            );
        }


        /* ----------------------------------------------------
         * HVAC UART DATA
         * ---------------------------------------------------- */

        printf(
            "\n"
            "              HVAC UART DATA              \n"
            "--------------------------------------------\n"
        );


        printf(
            "HVAC State:   %u\n",
            current_hvac_state
        );


        printf(
            "HVAC Fault:   %u\n",
            current_hvac_fault
        );


        printf(
            "Fan State:    %u\n",
            current_hvac_fan
        );


        printf(
            "Heater State: %u\n",
            current_hvac_heater
        );


        printf(
            "============================================\n"
        );


        /* ----------------------------------------------------
         * CONTROL LOOP
         * ---------------------------------------------------- */

    }
}


/* ============================================================
 * INITIALIZATION
 * ============================================================ */

void thermo_logic_init(void)
{
    indoor_temp = 0.0;
    float setpoint =
        SETPOINT_START;




    printf(
        "Thermostat logic initialized\n"
    );


    printf(
        "Initial setpoint: %.1f C\n",
        setpoint
    );


    BaseType_t result =
        xTaskCreate(
            thermo_control_task,
            "thermo_control",
            3072,
            NULL,
            2,
            NULL
        );


    if (result != pdPASS)
    {
        printf(
            "Failed to create thermostat control task\n"
        );

        return;
    }


    printf(
        "Thermostat control task started\n"
    );
}