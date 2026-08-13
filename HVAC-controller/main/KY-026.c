#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "esp_timer.h"
static const char *TAG = "FLAME_ADC";

/* =========================
 * ADC CONFIGURATION
 * ========================= */

/*
 * Change these to match your wiring.
 *
 * Example below:
 * GPIO 34 -> ADC1_CHANNEL_6 on ESP32
 */

#define FLAME_ADC_UNIT       ADC_UNIT_1
#define FLAME_ADC_CHANNEL    ADC_CHANNEL_6
#define FLAME_ADC_ATTEN      ADC_ATTEN_DB_12
#define HEATER_PIN GPIO_NUM_18
/* =========================
 * ADC HANDLE
 * ========================= */

static adc_oneshot_unit_handle_t adc_handle;


/* =========================
 * ADC INITIALIZATION
 * ========================= */

static void flame_adc_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = FLAME_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(
            &init_config,
            &adc_handle
        )
    );

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = FLAME_ADC_ATTEN,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            FLAME_ADC_CHANNEL,
            &config
        )
    );

    // Configure Relay Control Output Pins
    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << HEATER_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&io_config);

    ESP_LOGI(
        TAG,
        "ADC initialized"
    );

    ESP_LOGI(
        TAG,
        "ADC unit = %d",
        FLAME_ADC_UNIT
    );

    ESP_LOGI(
        TAG,
        "ADC channel = %d",
        FLAME_ADC_CHANNEL
    );
}


/* =========================
 * MAIN TEST
 * ========================= */

void app_main(void)
{
    flame_adc_init();

    printf("\n");
    printf("========================================\n");
    printf("       FLAME SENSOR ADC TEST\n");
    printf("========================================\n");
    printf("Raw ADC readings:\n");
    printf("========================================\n");

    
    int raw_value = 0;

    for (int i=0 ; i < 100 ; i++){

    
        /*
         * Read ADC using oneshot mode.
         */


        if (i==5) {
            gpio_set_level(HEATER_PIN, 1);
            printf(" ON ");
        }
        if (i==20) {
            gpio_set_level(HEATER_PIN, 0);
            printf(" OFF ");
        }
         
        ESP_ERROR_CHECK(
            adc_oneshot_read(
                adc_handle,
                FLAME_ADC_CHANNEL,
                &raw_value
            )
        );
       
        /*
         * Print raw ADC value.
         */
        printf(
            "%.2u,%d\n",
            (unsigned int)(esp_timer_get_time)(),raw_value
        );

        /*
         * Allow the system to continue running.
         */
        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }
    gpio_set_level(HEATER_PIN, 0);

}
