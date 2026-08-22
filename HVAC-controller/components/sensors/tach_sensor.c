#include "tach_sensor.h"

#include "esp_log.h"

static const char *TAG = "TACH_SENSOR";

static pcnt_unit_handle_t pcnt_unit = NULL;
static pcnt_channel_handle_t pcnt_channel = NULL;

uint32_t g_latest_rpm = 0.0f;


/* ============================================================
 * Initialization
 * ============================================================ */

esp_err_t tach_sensor_init(void)
{
    /* Create PCNT unit */

    pcnt_unit_config_t unit_config = {
        .high_limit = PCNT_HIGH_LIMIT,
        .low_limit = PCNT_LOW_LIMIT
    };

    esp_err_t err =
        pcnt_new_unit(
            &unit_config,
            &pcnt_unit
        );

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to create PCNT unit"
        );

        return err;
    }


    /* Create PCNT channel */

    pcnt_chan_config_t channel_config = {
        .edge_gpio_num = TACH_GPIO_PIN,
        .level_gpio_num = -1
    };

    err =
        pcnt_new_channel(
            pcnt_unit,
            &channel_config,
            &pcnt_channel
        );

    if (err != ESP_OK) {

        ESP_LOGE(
            TAG,
            "Failed to create PCNT channel"
        );

        pcnt_del_unit(pcnt_unit);
        pcnt_unit = NULL;

        return err;
    }


    /* Count rising edges */

    err =
        pcnt_channel_set_edge_action(
            pcnt_channel,
            PCNT_CHANNEL_EDGE_ACTION_INCREASE,
            PCNT_CHANNEL_EDGE_ACTION_HOLD
        );

    if (err != ESP_OK) {
        return err;
    }


    /* Enable */

    err = pcnt_unit_enable(pcnt_unit);

    if (err != ESP_OK) {
        return err;
    }


    err = pcnt_unit_clear_count(pcnt_unit);

    if (err != ESP_OK) {
        return err;
    }


    err = pcnt_unit_start(pcnt_unit);

    if (err != ESP_OK) {
        return err;
    }


    ESP_LOGI(
        TAG,
        "Tachometer initialized on GPIO %d",
        TACH_GPIO_PIN
    );

    return ESP_OK;
}


/* ============================================================
 * RPM Measurement
 * ============================================================ */

uint32_t get_tach_sensor_rpm(void)
{
    if (pcnt_unit == NULL) {
        return 0.0f;
    }

    int count = 0;

    esp_err_t err =
        pcnt_unit_get_count(
            pcnt_unit,
            &count
        );

    if (err != ESP_OK) {

        ESP_LOGE(
            TAG,
            "Failed to read PCNT"
        );

        return 0.0f;
    }


    /*
     * This function is intended to be called
     * once every RPM_SAMPLE_TIME_MS.
     *
     * RPM =
     *
     * pulses
     * -------------------- × 60
     * sample_time_seconds × pulses_per_rev
     */

    float sample_time_seconds =
        RPM_SAMPLE_TIME_MS / 1000.0f;


    g_latest_rpm =
        ((float)count /
        (sample_time_seconds *
         TACH_PULSES_PER_REV)) * 60.0f;


    /* Reset counter for next measurement */

    pcnt_unit_clear_count(pcnt_unit);


    ESP_LOGD(
        TAG,
        "Tach count=%d RPM=%.1f",
        count,
        g_latest_rpm
    );

    return (uint32_t)g_latest_rpm;
}