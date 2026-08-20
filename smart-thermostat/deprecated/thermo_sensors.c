#include "thermo_sensors.h"

#include <stdio.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"


/* ============================================================
 * HARDWARE DEFINITIONS
 * ============================================================ */

#define ADC_UNIT        ADC_UNIT_1
#define ADC_CHANNEL     ADC_CHANNEL_6       // GPIO34
#define ADC_ATTEN       ADC_ATTEN_DB_12


/* ============================================================
 * THERMISTOR PARAMETERS
 * ============================================================ */

#define THERMISTOR_NOMINAL       10000.0f
#define TEMPERATURE_NOMINAL      25.0f
#define BETA_COEFFICIENT         3950.0f
#define SERIES_RESISTOR          10000.0f
#define VCC_VOLTAGE              3300.0f


/* ============================================================
 * INTERNAL VARIABLES
 * ============================================================ */

static const char *TAG = "THERMISTOR";

static adc_oneshot_unit_handle_t adc_handle = NULL;

static adc_cali_handle_t cali_handle = NULL;

static bool has_calibration = false;

static QueueHandle_t temperature_queue = NULL;

static float latest_temperature = 0.0f;


/* ============================================================
 * ADC CALIBRATION
 * ============================================================ */

static bool init_adc_calibration(
    adc_unit_t unit,
    adc_channel_t channel,
    adc_atten_t atten,
    adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;

    esp_err_t ret = ESP_FAIL;

    bool calibrated = false;


#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED

    ESP_LOGI(
        TAG,
        "Registering ADC line fitting calibration..."
    );

    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };

    ret = adc_cali_create_scheme_line_fitting(
        &cali_config,
        &handle
    );

    if (ret == ESP_OK)
    {
        calibrated = true;

        ESP_LOGI(
            TAG,
            "ADC calibration enabled"
        );
    }
    else
    {
        ESP_LOGW(
            TAG,
            "ADC calibration failed"
        );
    }

#endif


    *out_handle = handle;

    return calibrated;
}


/* ============================================================
 * TEMPERATURE SENSOR TASK
 * ============================================================ */

static void temperature_sensor_task(void *pvParameters)
{
    (void)pvParameters;

    ESP_LOGI(
        TAG,
        "Temperature sensor task started"
    );


    while (1)
    {
        int raw_reading = 0;

        int voltage_mv = 0;

        int accumulated_raw = 0;

        const int samples = 10;


        /* ----------------------------------------------------
         * TAKE 10 ADC SAMPLES
         * ---------------------------------------------------- */

        for (int i = 0; i < samples; i++)
        {
            esp_err_t ret = adc_oneshot_read(
                adc_handle,
                ADC_CHANNEL,
                &raw_reading
            );


            if (ret != ESP_OK)
            {
                ESP_LOGE(
                    TAG,
                    "ADC read failed: %s",
                    esp_err_to_name(ret)
                );

                raw_reading = 0;
            }


            accumulated_raw += raw_reading;


            vTaskDelay(
                pdMS_TO_TICKS(10)
            );
        }


        /* ----------------------------------------------------
         * AVERAGE ADC
         * ---------------------------------------------------- */

        raw_reading =
            accumulated_raw / samples;


        /* ----------------------------------------------------
         * CONVERT ADC TO VOLTAGE
         * ---------------------------------------------------- */

        if (has_calibration)
        {
            esp_err_t ret =
                adc_cali_raw_to_voltage(
                    cali_handle,
                    raw_reading,
                    &voltage_mv
                );


            if (ret != ESP_OK)
            {
                ESP_LOGE(
                    TAG,
                    "ADC calibration conversion failed"
                );

                vTaskDelay(
                    pdMS_TO_TICKS(SENSOR_PERIOD_MS)
                );

                continue;
            }
        }
        else
        {
            voltage_mv =
                (raw_reading * 3300) / 4095;
        }


        /* ----------------------------------------------------
         * CHECK VOLTAGE
         * ---------------------------------------------------- */

        if (voltage_mv <= 0 ||
            voltage_mv >= VCC_VOLTAGE)
        {
            ESP_LOGE(
                TAG,
                "Invalid voltage: %d mV",
                voltage_mv
            );

            vTaskDelay(
                pdMS_TO_TICKS(SENSOR_PERIOD_MS)
            );

            continue;
        }


        /* ----------------------------------------------------
         * CALCULATE THERMISTOR RESISTANCE
         *
         * Thermistor:
         *
         *      3.3 V
         *        |
         *     10k resistor
         *        |
         *        +---- ADC
         *        |
         *    Thermistor
         *        |
         *       GND
         *
         * Rt = Rfixed * Vout / (Vcc - Vout)
         * ---------------------------------------------------- */

        float resistance =
            SERIES_RESISTOR *
            (
                (float)voltage_mv /
                (VCC_VOLTAGE - (float)voltage_mv)
            );


        /* ----------------------------------------------------
         * BETA EQUATION
         * ---------------------------------------------------- */

        float steinhart;

        steinhart =
            resistance /
            THERMISTOR_NOMINAL;

        steinhart =
            logf(steinhart);

        steinhart /=
            BETA_COEFFICIENT;

        steinhart +=
            1.0f /
            (TEMPERATURE_NOMINAL + 273.15f);

        steinhart =
            1.0f /
            steinhart;


        float temperature_celsius =
            steinhart - 273.15f;


        /* ----------------------------------------------------
         * CREATE TEMPERATURE DATA
         * ---------------------------------------------------- */

        TemperatureData data;

        data.temperature_c =
            temperature_celsius;

        data.raw_adc =
            raw_reading;

        data.voltage_mv =
            voltage_mv;

        data.resistance_ohm =
            resistance;


        /* ----------------------------------------------------
         * UPDATE LATEST VALUE
         * ---------------------------------------------------- */

        latest_temperature =
            temperature_celsius;


        /* ----------------------------------------------------
         * SEND DATA TO QUEUE
         *
         * xQueueOverwrite() keeps only the
         * newest temperature measurement.
         * ---------------------------------------------------- */

        if (temperature_queue != NULL)
        {
            xQueueOverwrite(
                temperature_queue,
                &data
            );
        }


        /* ----------------------------------------------------
         * DISPLAY
         * ---------------------------------------------------- */

        ESP_LOGI(
            TAG,
            "Raw: %d | Voltage: %d mV | Resistance: %.1f Ohm | Temp: %.2f C",
            raw_reading,
            voltage_mv,
            resistance,
            temperature_celsius
        );


        /* ----------------------------------------------------
         * WAIT
         * ---------------------------------------------------- */

        vTaskDelay(
            pdMS_TO_TICKS(
                SENSOR_PERIOD_MS
            )
        );
    }
}


/* ============================================================
 * INITIALIZATION
 * ============================================================ */

void thermo_sensors_init(void)
{
    printf(
        "THERMO: Starting sensor initialization...\n"
    );


    /* --------------------------------------------------------
     * CREATE TEMPERATURE QUEUE
     * -------------------------------------------------------- */

    temperature_queue =
        xQueueCreate(
            1,
            sizeof(TemperatureData)
        );


    if (temperature_queue == NULL)
    {
        printf(
            "THERMO: Failed to create temperature queue\n"
        );

        return;
    }


    printf(
        "THERMO: Temperature queue created\n"
    );


    /* --------------------------------------------------------
     * CREATE ADC UNIT
     * -------------------------------------------------------- */

    printf(
        "THERMO: Creating ADC unit...\n"
    );


    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT
    };


    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(
            &init_config,
            &adc_handle
        )
    );


    printf(
        "THERMO: ADC unit created\n"
    );


    /* --------------------------------------------------------
     * CONFIGURE ADC CHANNEL
     * -------------------------------------------------------- */

    printf(
        "THERMO: Configuring GPIO34 / ADC1_CHANNEL_6...\n"
    );


    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN
    };


    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            ADC_CHANNEL,
            &chan_config
        )
    );


    printf(
        "THERMO: ADC channel configured\n"
    );


    /* --------------------------------------------------------
     * ADC CALIBRATION
     * -------------------------------------------------------- */

    printf(
        "THERMO: Initializing ADC calibration...\n"
    );


    has_calibration =
        init_adc_calibration(
            ADC_UNIT,
            ADC_CHANNEL,
            ADC_ATTEN,
            &cali_handle
        );


    if (!has_calibration)
    {
        ESP_LOGW(
            TAG,
            "Calibration unavailable; using nominal conversion"
        );
    }


    printf(
        "THERMO: ADC calibration complete\n"
    );


    printf(
        "Temperature sensor initialized\n"
    );

    printf(
        "Indoor sensor: GPIO34 / ADC1_CHANNEL_6\n"
    );


    /* --------------------------------------------------------
     * CREATE SENSOR TASK
     * -------------------------------------------------------- */

    printf(
        "THERMO: ABOUT TO CREATE SENSOR TASK\n"
    );


    BaseType_t result =
        xTaskCreate(
            temperature_sensor_task,
            "temp_sense",
            4096,
            NULL,
            5,
            NULL
        );


    if (result != pdPASS)
    {
        printf(
            "THERMO: FAILED TO CREATE SENSOR TASK\n"
        );

        return;
    }


    printf(
        "THERMO: SENSOR TASK IS RUNNING\n"
    );
}


/* ============================================================
 * GET TEMPERATURE QUEUE
 * ============================================================ */

QueueHandle_t thermo_temperature_queue(void)
{
    return temperature_queue;
}


/* ============================================================
 * GET LATEST TEMPERATURE
 * ============================================================ */

float get_indoor_temperature(void)
{
    return latest_temperature;
}