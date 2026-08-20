#include "thermo_sensors.h"

#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "THERMO_SENSORS";

static adc_oneshot_unit_handle_t adc_handle = NULL;
static SensorData temp;


/* ============================================================
 * INITIALIZE TEMPERATURE SENSOR
 * ============================================================ */

void thermo_sensors_init(void)
{
    printf("THERMO: Starting sensor initialization...\n");

    /* --------------------------------------------------------
     * Initialize ADC Unit 1
     * -------------------------------------------------------- */

    adc_oneshot_unit_init_cfg_t adc_unit_config = {
        .unit_id = ADC_UNIT_1
    };

    esp_err_t result = adc_oneshot_new_unit(
        &adc_unit_config,
        &adc_handle
    );

    if (result != ESP_OK) {
        printf(
            "THERMO: ADC initialization failed: %s\n",
            esp_err_to_name(result)
        );
        return;
    }

    printf("THERMO: ADC unit initialized\n");


    /* --------------------------------------------------------
     * ADC configuration
     * -------------------------------------------------------- */

    adc_oneshot_chan_cfg_t adc_channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12
    };


    /* --------------------------------------------------------
     * Indoor temperature sensor
     *
     * GPIO36 = ADC1_CHANNEL_0
     * -------------------------------------------------------- */

    printf("THERMO: Configuring indoor sensor...\n");

    result = adc_oneshot_config_channel(
        adc_handle,
        INDOOR_TEMP_CHANNEL,
        &adc_channel_config
    );

    if (result != ESP_OK) {
        printf(
            "THERMO: Indoor sensor configuration failed: %s\n",
            esp_err_to_name(result)
        );
        return;
    }

    printf("THERMO: Indoor sensor configured on GPIO36\n");


    /* --------------------------------------------------------
     * Initial values
     * -------------------------------------------------------- */

    temp.indoor_temp = 0;


    printf("Temperature sensor initialized\n");
    printf("Indoor sensor: GPIO36\n");



    /* --------------------------------------------------------
     * Create temperature sensor task
     * -------------------------------------------------------- */

    BaseType_t task_result = xTaskCreate(
        temperature_sensor_task,
        "temperature_sensor",
        3072,
        NULL,
        2,
        NULL
    );

    if (task_result != pdPASS) {
        ESP_LOGE(
            TAG,
            "Failed to create temperature sensor task"
        );
        return;
    }

    printf("Temperature sensor task started\n");
}


/* ============================================================
 * TEMPERATURE SENSOR TASK
 * ============================================================ */

void temperature_sensor_task(void *pvParameters)
{
    (void)pvParameters;

    while (1) {

        int indoor_reading = 0;

        /* ----------------------------------------------------
         * Read indoor temperature sensor
         * ---------------------------------------------------- */

        esp_err_t result = adc_oneshot_read(
            adc_handle,
            INDOOR_TEMP_CHANNEL,
            &indoor_reading
        );


        if (result == ESP_OK) {

            temp.indoor_temp = indoor_reading;

            printf(
                "Indoor ADC: %d \n",
                indoor_reading
            );

        } else {

            printf(
                "Failed to read indoor temperature: %s\n",
                esp_err_to_name(result)
            );
        }


        vTaskDelay(
            pdMS_TO_TICKS(SENSOR_PERIOD_MS)
        );
    }
}


/* ============================================================
 * GET INDOOR TEMPERATURE
 * ============================================================ */

float get_indoor_temperature(void)
{
    return (float)temp.indoor_temp;
}
