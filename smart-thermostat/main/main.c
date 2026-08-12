/*  PINOUTS:
    Buttons:    SET_UP GPIO4, SET_DWN GPIO0, SEL_UP GPIO2, SEL_DWN GPIO15
    Thermistors: TEMP_V1 GPIO36, TEMP_V2 GPIO39
    LCD:        V0(contrast) GPIO25, RS GPIO19, E GPIO21, D7-D4 GPIO18/5/17/16, R/W tied to GND
    Status LED: GPIO13 (PWM)
    UART:       TX GPIO1, RX GPIO3
    Boards share a common ground.
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "thermo_comms.h"
#include "thermo_sensors.h"
#include "thermo_logic.h"
#include "ui.h"

#define CONTROL_LOOP_PERIOD_MS 1000

/* TODO: placeholder until real thermistor voltage-divider / Steinhart-Hart
   conversion is added (needs beta value + divider resistor). */
static float adc_to_celsius(int raw_adc)
{
    return (raw_adc / 4095.0f) * 100.0f;
}

/* Bang-bang control with a small deadband to prevent chatter.
   NOTE: requires `temp` (thermo_sensors) and `user_setpoint` (ui) to be
   `extern`, not `static`, in their headers - see chat notes. */
static void update_thermostat_command(void)
{
    extern struct SensorData temp;
    extern int user_setpoint;

    const float deadband_c = 0.5f;
    float indoor_c = adc_to_celsius(temp.indoor_temp);

    if (indoor_c < (float)user_setpoint - deadband_c) {
        thermostat_command = HVAC_CMD_HEAT;
    } else if (indoor_c > (float)user_setpoint + deadband_c) {
        thermostat_command = HVAC_CMD_OFF;
    }
}

static void control_task(void *pvParameters)
{
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1) {
        update_thermostat_command();
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(CONTROL_LOOP_PERIOD_MS));
    }
}

void app_main(void)
{
    thermo_sensors_init();  /* starts temperature_sensor_task (10s ADC reads) */
    thermo_comms_init();    /* starts uart_transmit_task / uart_receive_task (10s) */
    ui_init();               /* button ISRs, LCD, UI/temp display timers */

    xTaskCreate(control_task, "Control task", 3072, NULL, 2, NULL);

    printf("Thermostat app_main initialization complete\n");
}
