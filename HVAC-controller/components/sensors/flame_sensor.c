#include "flame_sensor.h"

#include "esp_log.h"

static const char *TAG = "FLAME_SENSOR";

static adc_oneshot_unit_handle_t adc_handle = NULL;

float g_latest_flame = 0.0f;


/* ============================================================
 * Flame Threshold
 * ============================================================ */

/*
 * Your measured ADC values are 0-4095.
 *
 * Start with this value and adjust after testing.
 */
#define FLAME_DETECTED_THRESHOLD 1000


/* ============================================================
 * Initialization
 * ============================================================ */

esp_err_t flame_sensor_init(void)
{
    if (adc_handle != NULL) {
        return ESP_OK;
    }

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };

    esp_err_t err =
        adc_oneshot_new_unit(
            &init_config,
            &adc_handle
        );

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to initialize ADC unit"
        );

        return err;
    }


    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN
    };


    err =
        adc_oneshot_config_channel(
            adc_handle,
            ADC_CHAN,
            &channel_config
        );

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to configure flame ADC channel"
        );

        adc_oneshot_del_unit(adc_handle);
        adc_handle = NULL;

        return err;
    }


    ESP_LOGI(
        TAG,
        "Flame ADC initialized"
    );

    return ESP_OK;
}


/* ============================================================
 * ADC Reading
 * ============================================================ */

float get_flame_sensor_adc(void)
{
    if (adc_handle == NULL) {
        return 0.0f;
    }

    int raw_value = 0;

    esp_err_t err =
        adc_oneshot_read(
            adc_handle,
            ADC_CHAN,
            &raw_value
        );

    if (err != ESP_OK) {

        ESP_LOGE(
            TAG,
            "ADC read failed: %s",
            esp_err_to_name(err)
        );

        return 0.0f;
    }

    g_latest_flame = (float)raw_value;

    return g_latest_flame;
}