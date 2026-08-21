#include "temperature_sensor.h"

#include <stdio.h>
#include <math.h>

#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "THERMISTOR";


/* ============================================================
 * GLOBAL SENSOR
 * ============================================================ */

TempSensor temp_sensor = {0};


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

    if (ret == ESP_OK) {

        calibrated = true;

        ESP_LOGI(
            TAG,
            "ADC calibration enabled"
        );
    }

#endif


    *out_handle = handle;

    return calibrated;
}


/* ============================================================
 * READ TEMPERATURE
 * ============================================================ */

void temp_sensor_read(TempSensor *self)
{
    if (self == NULL) {
        return;
    }


    int raw_reading = 0;

    int voltage_mv = 0;


    /* --------------------------------------------------------
     * Take 10 samples
     * -------------------------------------------------------- */

    int samples = 10;

    int accumulated_raw = 0;


    for (int i = 0; i < samples; i++) {

        esp_err_t ret = adc_oneshot_read(
            self->adc_handle,
            ADC_CHANNEL,
            &raw_reading
        );


        if (ret != ESP_OK) {

            ESP_LOGE(
                TAG,
                "ADC read failed: %s",
                esp_err_to_name(ret)
            );

            return;
        }


        accumulated_raw += raw_reading;


        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }


    self->raw_reading =
        accumulated_raw / samples;


    /* --------------------------------------------------------
     * Convert ADC to voltage
     * -------------------------------------------------------- */

    if (self->has_calibration) {

        esp_err_t ret =
            adc_cali_raw_to_voltage(
                self->cali_handle,
                self->raw_reading,
                &voltage_mv
            );


        if (ret != ESP_OK) {

            ESP_LOGE(
                TAG,
                "ADC calibration conversion failed"
            );

            return;
        }

    }
    else {

        voltage_mv =
            (self->raw_reading * 3300) / 4095;
    }


    self->voltage_mv = voltage_mv;


    /* --------------------------------------------------------
     * Check voltage
     * -------------------------------------------------------- */

    if (
        voltage_mv <= 0 ||
        voltage_mv >= VCC_VOLTAGE
    ) {

        ESP_LOGE(
            TAG,
            "Invalid voltage: %d mV",
            voltage_mv
        );

        return;
    }


    /* --------------------------------------------------------
     * Calculate thermistor resistance
     *
     * Thermistor connected to GND:
     *
     * R = Rfixed * Vout / (Vcc - Vout)
     * -------------------------------------------------------- */

    self->resistance =
        SERIES_RESISTOR *
        (
            (float)voltage_mv /
            (VCC_VOLTAGE - (float)voltage_mv)
        );


    /* --------------------------------------------------------
     * Beta equation
     * -------------------------------------------------------- */

    float steinhart;


    steinhart =
        self->resistance /
        THERMISTOR_NOMINAL;


    steinhart =
        log(steinhart);


    steinhart /=
        BETA_COEFFICIENT;


    steinhart +=
        1.0 /
        (TEMPERATURE_NOMINAL + 273.15);


    steinhart =
        1.0 /
        steinhart;


    self->temp =
        steinhart - 273.15;


    /* --------------------------------------------------------
     * Print result
     * -------------------------------------------------------- */

    ESP_LOGI(
        TAG,
        "Raw: %d | Voltage: %d mV | Resistance: %.1f Ohm | Temp: %.2f C",
        self->raw_reading,
        self->voltage_mv,
        self->resistance,
        self->temp
    );
}


/* ============================================================
 * INITIALIZE SENSOR
 * ============================================================ */

void temp_sensor_init(TempSensor *self)
{
    if (self == NULL) {
        return;
    }


    ESP_LOGI(
        TAG,
        "Initializing temperature sensor..."
    );


    /* --------------------------------------------------------
     * Initialize ADC unit
     * -------------------------------------------------------- */

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT
    };


    esp_err_t ret =
        adc_oneshot_new_unit(
            &init_config,
            &self->adc_handle
        );


    if (ret != ESP_OK) {

        ESP_LOGE(
            TAG,
            "Failed to initialize ADC: %s",
            esp_err_to_name(ret)
        );

        return;
    }


    ESP_LOGI(
        TAG,
        "ADC unit initialized"
    );


    /* --------------------------------------------------------
     * Configure ADC channel
     * -------------------------------------------------------- */

    adc_oneshot_chan_cfg_t chan_config = {

        .bitwidth =
            ADC_BITWIDTH_DEFAULT,

        .atten =
            ADC_ATTEN
    };


    ret =
        adc_oneshot_config_channel(
            self->adc_handle,
            ADC_CHANNEL,
            &chan_config
        );


    if (ret != ESP_OK) {

        ESP_LOGE(
            TAG,
            "Failed to configure ADC channel: %s",
            esp_err_to_name(ret)
        );

        return;
    }


    ESP_LOGI(
        TAG,
        "ADC GPIO34 / CHANNEL6 configured"
    );


    /* --------------------------------------------------------
     * Initialize calibration
     * -------------------------------------------------------- */

    self->cali_handle = NULL;


    self->has_calibration =
        init_adc_calibration(
            ADC_UNIT,
            ADC_CHANNEL,
            ADC_ATTEN,
            &self->cali_handle
        );


    if (!self->has_calibration) {

        ESP_LOGW(
            TAG,
            "ADC calibration unavailable"
        );

    }
    else {

        ESP_LOGI(
            TAG,
            "ADC calibration complete"
        );
    }


    /* --------------------------------------------------------
     * Initialize values
     * -------------------------------------------------------- */

    self->raw_reading = 0;

    self->voltage_mv = 0;

    self->resistance = 0;

    self->temp = 0;


    /* --------------------------------------------------------
     * Assign function pointers
     * -------------------------------------------------------- */

    self->init =
        temp_sensor_init;

    self->read =
        temp_sensor_read;


    ESP_LOGI(
        TAG,
        "Temperature sensor initialized"
    );
}