#include "thermo_logic.h"

static const char *TAG = "THERMISTOR";

#define ADC_CHANNEL         ADC_CHANNEL_6     // GPIO34 on ESP32
#define ADC_ATTEN           ADC_ATTEN_DB_12   // Up to ~3.3V range
#define R_FIXED             10000.0f          // Fixed resistor value (Ohms)
#define NTC_R25             10000.0f          // Thermistor resistance at 25°C
#define NTC_BETA            3950.0f           // Beta coefficient
#define NTC_T25_K           298.15f           // 25°C in Kelvin
#define V_SUPPLY            3300.0f           // Supply voltage in mV

static void update(float temperature) {
    if (temp_queue != NULL) {
        if (xQueueSend(temp_queue, &temperature, 0) != pdPASS) {
            ESP_LOGE(TAG, "HVAC Event Queue Full! Dropped CMD: %d", cmd);
        }
    }
}
// Helper function to initialize factory ADC calibration eFuse settings
static adc_cali_handle_t init_adc_calibration(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten) {
    adc_cali_handle_t handle = NULL;
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_config, &handle) == ESP_OK) {
        ESP_LOGI(TAG, "ADC Calibration curve fitting initialized successfully");
    } else {
        ESP_LOGW(TAG, "Calibration scheme not supported or eFuse not burned; raw voltage conversion may be imprecise");
    }
    return handle;
}

// FreeRTOS Task to handle sampling, averaging, and temperature calculation
void thermistor_task(void *pvParameters) {
    // 1. Initialize ADC Unit
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // 2. Configure ADC Channel Attenuation
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config));

    // 3. Initialize Factory Hardware Calibration Handle
    adc_cali_handle_t cali_handle = init_adc_calibration(ADC_UNIT_1, ADC_CHANNEL, ADC_ATTEN);

    while (1) {
        // Sample averaging to reduce ADC noise
        int raw_sum = 0;
        const int samples = 64;
        for (int i = 0; i < samples; i++) {
            int raw_val = 0;
            adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_val);
            raw_sum += raw_val;
            vTaskDelay(pdMS_TO_TICKS(2));
        }
        int avg_raw = raw_sum / samples;

        // Convert raw ADC value to calibrated Voltage (mV)
        int voltage_mv = 0;
        if (cali_handle) {
            adc_cali_raw_to_voltage(cali_handle, avg_raw, &voltage_mv);
        } else {
            // Fallback estimation if no calibration scheme is configured
            voltage_mv = (avg_raw * 3300) / 4095;
        }

        // Calculate Thermistor Resistance (R_ntc = R_fixed * V_measured / (V_supply - V_measured))
        float v_meas = (float)voltage_mv;
        if (v_meas >= V_SUPPLY) v_meas = V_SUPPLY - 1.0f; // Prevent division by zero
        
        float r_ntc = R_FIXED * (v_meas / (V_SUPPLY - v_meas));

        // Beta Equation to convert Resistance to Temperature in Celsius
        float temp_k = 1.0f / ((1.0f / NTC_T25_K) + (logf(r_ntc / NTC_R25) / NTC_BETA));
        float temp_c = temp_k - 273.15f;

        ESP_LOGI(TAG, "Raw: %d | Voltage: %d mV | Resistance: %.1f Ohm | Temp: %.2f C", 
                 avg_raw, voltage_mv, r_ntc, temp_c);
        
        update(temp_c);
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Read every second
    }

    // Clean up handle on exit
    if (cali_handle) adc_cali_delete_scheme_curve_fitting(cali_handle);
    adc_oneshot_del_unit(adc_handle);
    vTaskDelete(NULL);
}