#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// Hardware Definitions
#define ADC_UNIT           ADC_UNIT_1
#define ADC_CHANNEL        ADC_CHANNEL_6           // GPIO 34 on ESP32
#define ADC_ATTEN          ADC_ATTEN_DB_12         // Allows reading up to ~3.1V

// Thermistor Parameters
#define THERMISTOR_NOMINAL 10000.0                 // Resistance at 25 degrees C (10k)
#define TEMPERATURE_NOMINAL 25.0                   // Nominal temperature in C
#define BETA_COEFFICIENT   3950.0                  // Beta coefficient of the thermistor
#define SERIES_RESISTOR    10000.0                 // Value of the fixed resistor (10k)
#define VCC_VOLTAGE        3300.0                  // Supply voltage in mV (3.3V)

static const char *TAG = "THERMISTOR";

// Helper function to check and initialize calibration
static bool init_adc_calibration(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "Registering Line Fitting calibration scheme...");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) calibrated = true;
    }
#endif

    *out_handle = handle;
    return calibrated;
}

void app_main(void) {
    // 1. Initialize ADC Unit
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // 2. Configure ADC Channel
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config));

    // 3. Initialize Calibration
    adc_cali_handle_t cali_handle = NULL;
    bool has_calibration = init_adc_calibration(ADC_UNIT, ADC_CHANNEL, ADC_ATTEN, &cali_handle);
    if (!has_calibration) {
        ESP_LOGW(TAG, "Calibration not supported; raw readings will fallback to nominal configurations.");
    }

    while (1) {
        int raw_reading = 0;
        int voltage_mv = 0;

        // 4. Sample and Multi-sample to average noise
        int samples = 10;
        int accumulated_raw = 0;
        for (int i = 0; i < samples; i++) {
            adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_reading);
            accumulated_raw += raw_reading;
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        raw_reading = accumulated_raw / samples;

        // 5. Convert Raw data to Calibrated Voltage
        if (has_calibration) {
            adc_cali_raw_to_voltage(cali_handle, raw_reading, &voltage_mv);
        } else {
            // Fallback calculation if eFuses aren't burned
            voltage_mv = (raw_reading * 3300) / 4095;
        }

        // Avoid division by zero if voltage spikes near VCC or GND
        if (voltage_mv > 0 && voltage_mv < VCC_VOLTAGE) {
            
            // 6. Calculate Thermistor Resistance from Voltage Divider
            // Equation based on Thermistor tied to GND: R_t = R_fixed * (V_out / (Vcc - V_out))
            float resistance = SERIES_RESISTOR * ((float)voltage_mv / (VCC_VOLTAGE - (float)voltage_mv));

            // 7. Apply Beta Equation
            float steinhart;
            steinhart = resistance / THERMISTOR_NOMINAL;          // (R/Ro)
            steinhart = log(steinhart);                           // ln(R/Ro)
            steinhart /= BETA_COEFFICIENT;                        // 1/B * ln(R/Ro)
            steinhart += 1.0 / (TEMPERATURE_NOMINAL + 273.15);    // + (1/To)
            steinhart = 1.0 / steinhart;                          // Invert to absolute Kelvin
            
            float temperature_celsius = steinhart - 273.15;       // Convert Kelvin to Celsius

            ESP_LOGI(TAG, "Raw: %d | Voltage: %d mV | Resistance: %.1f Ohm | Temp: %.2f °C", 
                    raw_reading, voltage_mv, resistance, temperature_celsius);
        } else {
            ESP_LOGE(TAG, "Invalid voltage reading: %d mV", voltage_mv);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }

    // Clean up handles if the loop ever breaks
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_handle));
    if (has_calibration) {
        adc_cali_delete_scheme_line_fitting(cali_handle);
    }
}
