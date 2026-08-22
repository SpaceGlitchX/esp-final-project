#include "hardware_manager.h"

static const char *TAG = "HVAC_HW";

static int fan_state = 0;
static int heater_state = 0;


/* ============================================================
 * Initialization
 * ============================================================ */

esp_err_t hardware_manager_init(void)
{
    gpio_config_t io_config = {
        .pin_bit_mask =
            (1ULL << HEATER_PIN) |
            (1ULL << FAN_PIN),

        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t err = gpio_config(&io_config);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO configuration failed");
        return err;
    }

    /* Safe startup condition */

    gpio_set_level(HEATER_PIN, 0);
    gpio_set_level(FAN_PIN, 0);

    heater_state = 0;
    fan_state = 0;

    ESP_LOGI(TAG, "Hardware manager initialized");
    ESP_LOGI(TAG, "Heater GPIO: %d", HEATER_PIN);
    ESP_LOGI(TAG, "Fan GPIO: %d", FAN_PIN);

    return ESP_OK;
}


/* ============================================================
 * Heater Control
 * ============================================================ */

void set_heater_state(int level)
{
    level = level ? 1 : 0;

    gpio_set_level(HEATER_PIN, level);

    heater_state = level;

    ESP_LOGI(
        TAG,
        "Heater %s",
        level ? "ON" : "OFF"
    );
}


/* ============================================================
 * Fan Control
 * ============================================================ */

void set_fan_state(int level)
{
    level = level ? 1 : 0;

    gpio_set_level(FAN_PIN, level);

    fan_state = level;

    ESP_LOGI(
        TAG,
        "Fan %s",
        level ? "ON" : "OFF"
    );
}


/* ============================================================
 * Status
 * ============================================================ */

int get_fan_state(void)
{
    return fan_state;
}


int get_heater_state(void)
{
    return heater_state;
}