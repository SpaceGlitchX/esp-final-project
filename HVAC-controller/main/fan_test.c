#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/ledc.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_err.h"


static const char *TAG = "FAN_TEST";


/* ============================================================
 * FAN CONFIGURATION
 * ============================================================ */

#define FAN_PWM_GPIO       GPIO_NUM_18
#define FAN_TACH_GPIO      GPIO_NUM_19

#define FAN_PWM_FREQUENCY  25000

#define FAN_PWM_RESOLUTION LEDC_TIMER_10_BIT

#define FAN_PWM_TIMER      LEDC_TIMER_0
#define FAN_PWM_CHANNEL    LEDC_CHANNEL_0
#define FAN_PWM_MODE       LEDC_HIGH_SPEED_MODE

#define FAN_SUPPLY_VOLTAGE 12.0f


/*
 * Tachometer pulses per revolution.
 *
 * Currently assumed to be 2 pulses/revolution.
 */
#define TACH_PULSES_PER_REV 2


/*
 * RPM measurement window.
 *
 * 1000 ms makes the RPM calculation simple:
 *
 * RPM = pulses / pulses_per_rev * 60
 */
#define RPM_SAMPLE_TIME_MS 1000


/*
 * Number of measurements at each PWM level.
 */
#define NUM_SAMPLES 20


/*
 * PWM sweep increment.
 */
#define PWM_STEP_PERCENT 5


/* ============================================================
 * PCNT
 * ============================================================ */

static pcnt_unit_handle_t pcnt_unit = NULL;


/* ============================================================
 * PWM INITIALIZATION
 * ============================================================ */

static void fan_pwm_init(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode       = FAN_PWM_MODE,
        .duty_resolution  = FAN_PWM_RESOLUTION,
        .timer_num        = FAN_PWM_TIMER,
        .freq_hz          = FAN_PWM_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(
        ledc_timer_config(&timer_config)
    );


    ledc_channel_config_t channel_config = {
        .gpio_num       = FAN_PWM_GPIO,
        .speed_mode     = FAN_PWM_MODE,
        .channel        = FAN_PWM_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = FAN_PWM_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };

    ESP_ERROR_CHECK(
        ledc_channel_config(&channel_config)
    );


    ESP_LOGI(
        TAG,
        "PWM initialized: GPIO=%d, Frequency=%d Hz",
        FAN_PWM_GPIO,
        FAN_PWM_FREQUENCY
    );
}


/* ============================================================
 * SET PWM DUTY CYCLE
 * ============================================================ */

static void fan_set_pwm(uint8_t percent)
{
    if (percent > 100)
    {
        percent = 100;
    }


    const uint32_t max_duty =
        (1 << 10) - 1;


    uint32_t duty =
        ((uint32_t)percent * max_duty) / 100;


    ESP_ERROR_CHECK(
        ledc_set_duty(
            FAN_PWM_MODE,
            FAN_PWM_CHANNEL,
            duty
        )
    );


    ESP_ERROR_CHECK(
        ledc_update_duty(
            FAN_PWM_MODE,
            FAN_PWM_CHANNEL
        )
    );


    
}


/* ============================================================
 * TACHOMETER INITIALIZATION
 * ============================================================ */

static void fan_tach_init(void)
{
    pcnt_unit_config_t unit_config = {
        .high_limit = 10000,
        .low_limit  = -1
    };


    /*
     * IMPORTANT:
     *
     * Do NOT redeclare pcnt_unit here.
     *
     * We want to store the PCNT handle in the
     * global variable used by measure_rpm().
     */

    ESP_ERROR_CHECK(
        pcnt_new_unit(
            &unit_config,
            &pcnt_unit
        )
    );


    /*
     * Ignore very short glitches/noise.
     */
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000
    };


    ESP_ERROR_CHECK(
        pcnt_unit_set_glitch_filter(
            pcnt_unit,
            &filter_config
        )
    );


    /*
     * Configure tachometer input.
     */
    pcnt_chan_config_t chan_config = {
        .edge_gpio_num  = FAN_TACH_GPIO,
        .level_gpio_num = -1
    };


    pcnt_channel_handle_t pcnt_chan = NULL;


    ESP_ERROR_CHECK(
        pcnt_new_channel(
            pcnt_unit,
            &chan_config,
            &pcnt_chan
        )
    );


    /*
     * Fan tachometer is normally an open-collector/open-drain
     * output, so enable the ESP32 internal pull-up.
     */
    ESP_ERROR_CHECK(
        gpio_set_pull_mode(
            FAN_TACH_GPIO,
            GPIO_PULLUP_ONLY
        )
    );


    /*
     * Count rising edges.
     */
    ESP_ERROR_CHECK(
        pcnt_channel_set_edge_action(
            pcnt_chan,
            PCNT_CHANNEL_EDGE_ACTION_INCREASE,
            PCNT_CHANNEL_EDGE_ACTION_HOLD
        )
    );


    /*
     * Enable PCNT.
     */
    ESP_ERROR_CHECK(
        pcnt_unit_enable(pcnt_unit)
    );


    ESP_ERROR_CHECK(
        pcnt_unit_clear_count(pcnt_unit)
    );


    ESP_ERROR_CHECK(
        pcnt_unit_start(pcnt_unit)
    );


    ESP_LOGI(
        TAG,
        "Tachometer initialized: GPIO=%d",
        FAN_TACH_GPIO
    );
}


/* ============================================================
 * MEASURE RPM
 * ============================================================ */

static float measure_rpm(void)
{
    int count = 0;


    /*
     * Reset counter before measurement.
     */

    ESP_ERROR_CHECK(
        pcnt_unit_stop(pcnt_unit)
    );


    ESP_ERROR_CHECK(
        pcnt_unit_clear_count(pcnt_unit)
    );


    ESP_ERROR_CHECK(
        pcnt_unit_start(pcnt_unit)
    );


    /*
     * Measurement window.
     *
     * This task blocks for one second while PCNT
     * continues counting hardware pulses.
     */

    vTaskDelay(
        pdMS_TO_TICKS(RPM_SAMPLE_TIME_MS)
    );


    /*
     * Stop counting.
     */

    ESP_ERROR_CHECK(
        pcnt_unit_stop(pcnt_unit)
    );


    /*
     * Read pulse count.
     */

    ESP_ERROR_CHECK(
        pcnt_unit_get_count(
            pcnt_unit,
            &count
        )
    );


    /*
     * Convert pulses to revolutions.
     */

    float revolutions =
        (float)count /
        (float)TACH_PULSES_PER_REV;


    /*
     * Convert revolutions during the
     * measurement period to RPM.
     */

    float rpm =
        revolutions *
        (60000.0f /
         (float)RPM_SAMPLE_TIME_MS);


    return rpm;
}


/* ============================================================
 * FAN CHARACTERIZATION TEST
 * ============================================================ */

static void run_fan_test(void)
{
    ESP_LOGI(
        TAG,
        "========================================"
    );

    ESP_LOGI(
        TAG,
        "STARTING FAN PWM/RPM CHARACTERIZATION"
    );

    ESP_LOGI(
        TAG,
        "PWM frequency: %d Hz",
        FAN_PWM_FREQUENCY
    );

    ESP_LOGI(
        TAG,
        "20 samples per PWM level"
    );

    ESP_LOGI(
        TAG,
        "PWM increment: 5%%"
    );

    ESP_LOGI(
        TAG,
        "========================================"
    );


    /*
     * Start fan at 0%.
     */

    fan_set_pwm(0);


    /*
     * Allow system to settle.
     */

    vTaskDelay(
        pdMS_TO_TICKS(2000)
    );


    /*
     * CSV header.
     *
     * Individual measurements:
     */

    printf(
        "\nPWM_Percent,Sample,RPM\n"
    );


    /*
     * Sweep from 0% to 100%.
     */

    for (
        uint8_t pwm = 0;
        pwm <= 100;
        pwm += PWM_STEP_PERCENT
    )
    {
        


        /*
         * Set PWM.
         */

        fan_set_pwm(pwm);


        /*
         * Allow fan speed to stabilize
         * before measurements begin.
         */

        vTaskDelay(
            pdMS_TO_TICKS(3000)
        );


        float rpm_sum = 0.0f;


        /*
         * Take 20 measurements.
         */

        for (
            int sample = 1;
            sample <= NUM_SAMPLES;
            sample++
        )
        {
            float rpm =
                measure_rpm();


            rpm_sum += rpm;


            /*
             * Output individual sample.
             *
             * This can be copied directly
             * into Excel.
             */

            printf(
                "%d,%d,%.2f\n",
                pwm,
                sample,
                rpm
            );
        }


        /*
         * Calculate average RPM.
         */

        float average_rpm =
            rpm_sum /
            (float)NUM_SAMPLES;


        /*
         * Print average to terminal.
         */

       


      
    }


    /*
     * Turn fan off after test.
     */

    fan_set_pwm(0);


    ESP_LOGI(
        TAG,
        "========================================"
    );

    ESP_LOGI(
        TAG,
        "FAN TEST COMPLETE"
    );

    ESP_LOGI(
        TAG,
        "Fan turned OFF"
    );

    ESP_LOGI(
        TAG,
        "========================================"
    );
}


/* ============================================================
 * TEST TASK
 * ============================================================ */

static void fan_test_task(void *pvParameters)
{
    (void)pvParameters;


    /*
     * Initialize PWM.
     */

    fan_pwm_init();


    /*
     * Initialize tachometer.
     */

    fan_tach_init();


    /*
     * Run characterization test once.
     */

    run_fan_test();


    /*
     * Test is complete.
     *
     * Delete this task instead of leaving
     * an infinite loop running.
     */

    vTaskDelete(NULL);
}


/* ============================================================
 * APP MAIN
 * ============================================================ */

void app_main(void)
{
    ESP_LOGI(
        TAG,
        "Starting fan characterization test..."
    );


    xTaskCreate(
        fan_test_task,
        "fan_test",
        4096,
        NULL,
        5,
        NULL
    );
}