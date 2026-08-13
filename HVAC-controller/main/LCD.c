#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "LCD1602";

/* =========================================================
 * LCD PIN CONFIGURATION
 * ========================================================= */

/*
 * LCD:
 *
 * RS  -> GPIO 25
 * R/W -> GND
 * E   -> GPIO 26
 *
 * D4  -> GPIO 27
 * D5  -> GPIO 14
 * D6  -> GPIO 12
 * D7  -> GPIO 13
 */

#define LCD_RS      GPIO_NUM_25
#define LCD_EN      GPIO_NUM_26

#define LCD_D4      GPIO_NUM_27
#define LCD_D5      GPIO_NUM_14
#define LCD_D6      GPIO_NUM_12
#define LCD_D7      GPIO_NUM_13


/* =========================================================
 * LCD COMMANDS
 * ========================================================= */

#define LCD_CLEAR_DISPLAY       0x01
#define LCD_RETURN_HOME         0x02

#define LCD_ENTRY_MODE_SET      0x06

#define LCD_DISPLAY_OFF        0x08
#define LCD_DISPLAY_ON         0x0C

#define LCD_CURSOR_ON           0x0E
#define LCD_CURSOR_BLINK        0x0F

#define LCD_FUNCTION_SET        0x28

#define LCD_SET_DDRAM           0x80


/* =========================================================
 * GPIO INITIALIZATION
 * ========================================================= */

static void lcd_gpio_init(void)
{
    /*
     * Configure RS and E.
     */

    gpio_config_t output_config = {
        .pin_bit_mask =
            (1ULL << LCD_RS) |
            (1ULL << LCD_EN) |
            (1ULL << LCD_D4) |
            (1ULL << LCD_D5) |
            (1ULL << LCD_D6) |
            (1ULL << LCD_D7),

        .mode = GPIO_MODE_OUTPUT,

        .pull_up_en = GPIO_PULLUP_DISABLE,

        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(
        gpio_config(&output_config)
    );

    /*
     * Start with everything LOW.
     */

    gpio_set_level(LCD_RS, 0);
    gpio_set_level(LCD_EN, 0);

    gpio_set_level(LCD_D4, 0);
    gpio_set_level(LCD_D5, 0);
    gpio_set_level(LCD_D6, 0);
    gpio_set_level(LCD_D7, 0);

    ESP_LOGI(
        TAG,
        "LCD GPIO initialized"
    );
}


/* =========================================================
 * SET DATA PINS
 * ========================================================= */

static void lcd_set_data_pins(
    uint8_t data
)
{
    gpio_set_level(
        LCD_D4,
        (data >> 0) & 0x01
    );

    gpio_set_level(
        LCD_D5,
        (data >> 1) & 0x01
    );

    gpio_set_level(
        LCD_D6,
        (data >> 2) & 0x01
    );

    gpio_set_level(
        LCD_D7,
        (data >> 3) & 0x01
    );
}


/* =========================================================
 * ENABLE PULSE
 * ========================================================= */

static void lcd_enable_pulse(void)
{
    /*
     * Enable HIGH.
     */

    gpio_set_level(
        LCD_EN,
        1
    );

    /*
     * Short enable pulse.
     */

    esp_rom_delay_us(1);

    /*
     * Enable LOW.
     */

    gpio_set_level(
        LCD_EN,
        0
    );

    /*
     * Allow LCD to process data.
     */

    esp_rom_delay_us(50);
}


/* =========================================================
 * WRITE 4-BIT NIBBLE
 * ========================================================= */

static void lcd_write_nibble(
    uint8_t nibble
)
{
    /*
     * Put nibble onto D4-D7.
     */

    lcd_set_data_pins(
        nibble & 0x0F
    );

    /*
     * Pulse E.
     */

    lcd_enable_pulse();
}


/* =========================================================
 * SEND COMMAND
 * ========================================================= */

static void lcd_command(
    uint8_t command
)
{
    /*
     * RS = 0
     *
     * This means command.
     */

    gpio_set_level(
        LCD_RS,
        0
    );

    /*
     * Send upper nibble.
     */

    lcd_write_nibble(
        (command >> 4) & 0x0F
    );

    /*
     * Send lower nibble.
     */

    lcd_write_nibble(
        command & 0x0F
    );

    /*
     * Clear and home commands take
     * longer to execute.
     */

    if (
        command == LCD_CLEAR_DISPLAY ||
        command == LCD_RETURN_HOME
    )
    {
        vTaskDelay(
            pdMS_TO_TICKS(2)
        );
    }
    else
    {
        esp_rom_delay_us(50);
    }
}


/* =========================================================
 * SEND DATA / CHARACTER
 * ========================================================= */

static void lcd_data(
    uint8_t data
)
{
    /*
     * RS = 1
     *
     * This means character data.
     */

    gpio_set_level(
        LCD_RS,
        1
    );

    /*
     * Upper nibble.
     */

    lcd_write_nibble(
        (data >> 4) & 0x0F
    );

    /*
     * Lower nibble.
     */

    lcd_write_nibble(
        data & 0x0F
    );
}


/* =========================================================
 * SEND STRING
 * ========================================================= */

static void lcd_string(
    const char *string
)
{
    if (string == NULL)
    {
        return;
    }

    while (*string)
    {
        lcd_data(
            (uint8_t)*string
        );

        string++;
    }
}


/* =========================================================
 * CLEAR LCD
 * ========================================================= */

static void lcd_clear(void)
{
    lcd_command(
        LCD_CLEAR_DISPLAY
    );

    vTaskDelay(
        pdMS_TO_TICKS(2)
    );
}


/* =========================================================
 * SET CURSOR
 * ========================================================= */

static void lcd_set_cursor(
    uint8_t row,
    uint8_t column
)
{
    uint8_t address;

    /*
     * 1602A DDRAM addresses:
     *
     * Row 0 = 0x00
     * Row 1 = 0x40
     */

    if (row == 0)
    {
        address = 0x00 + column;
    }
    else
    {
        address = 0x40 + column;
    }

    lcd_command(
        LCD_SET_DDRAM | address
    );
}


/* =========================================================
 * LCD INITIALIZATION
 * ========================================================= */

static void lcd_init(void)
{
    ESP_LOGI(
        TAG,
        "Starting LCD initialization..."
    );

    /*
     * LCD needs time after power-up.
     */

    vTaskDelay(
        pdMS_TO_TICKS(50)
    );

    /*
     * RS LOW.
     */

    gpio_set_level(
        LCD_RS,
        0
    );

    /*
     * E LOW.
     */

    gpio_set_level(
        LCD_EN,
        0
    );

    /*
     * -----------------------------------------------------
     * HD44780 INITIALIZATION
     * -----------------------------------------------------
     *
     * The controller powers up expecting 8-bit mode.
     *
     * We send the special initialization sequence
     * to switch it into 4-bit mode.
     */

    lcd_write_nibble(0x03);

    vTaskDelay(
        pdMS_TO_TICKS(5)
    );

    lcd_write_nibble(0x03);

    vTaskDelay(
        pdMS_TO_TICKS(5)
    );

    lcd_write_nibble(0x03);

    vTaskDelay(
        pdMS_TO_TICKS(1)
    );

    /*
     * Switch to 4-bit mode.
     */

    lcd_write_nibble(0x02);

    vTaskDelay(
        pdMS_TO_TICKS(1)
    );

    /*
     * 4-bit mode
     * 2 lines
     * 5x8 font
     */

    lcd_command(
        LCD_FUNCTION_SET
    );

    /*
     * Display OFF.
     */

    lcd_command(
        LCD_DISPLAY_OFF
    );

    /*
     * Clear display.
     */

    lcd_command(
        LCD_CLEAR_DISPLAY
    );

    /*
     * Entry mode:
     *
     * Cursor increments.
     * Display does not shift.
     */

    lcd_command(
        LCD_ENTRY_MODE_SET
    );

    /*
     * Display ON
     * Cursor OFF
     * Blink OFF
     */

    lcd_command(
        LCD_DISPLAY_ON
    );

    ESP_LOGI(
        TAG,
        "LCD initialization complete"
    );
}


/* =========================================================
 * LCD TEST
 * ========================================================= */

static void lcd_test(void)
{
    ESP_LOGI(
        TAG,
        "Running LCD test..."
    );

    /*
     * -----------------------------------------------------
     * TEST 1
     * -----------------------------------------------------
     */

    lcd_clear();

    lcd_set_cursor(
        0,
        0
    );

    lcd_string(
        "LCD TEST"
    );

    lcd_set_cursor(
        1,
        0
    );

    lcd_string(
        "ESP32 WORKING!"
    );

    vTaskDelay(
        pdMS_TO_TICKS(3000)
    );


    /*
     * -----------------------------------------------------
     * TEST 2
     * -----------------------------------------------------
     */

    lcd_clear();

    lcd_set_cursor(
        0,
        0
    );

    lcd_string(
        "1602A LCD"
    );

    lcd_set_cursor(
        1,
        0
    );

    lcd_string(
        "4-BIT MODE"
    );

    vTaskDelay(
        pdMS_TO_TICKS(3000)
    );


    /*
     * -----------------------------------------------------
     * TEST 3
     * -----------------------------------------------------
     */

    lcd_clear();

    lcd_set_cursor(
        0,
        0
    );

    lcd_string(
        "ROW 1: PASS"
    );

    lcd_set_cursor(
        1,
        0
    );

    lcd_string(
        "ROW 2: PASS"
    );

    ESP_LOGI(
        TAG,
        "LCD test complete"
    );
}


/* =========================================================
 * MAIN
 * ========================================================= */

void app_main(void)
{
    ESP_LOGI(
        TAG,
        "================================"
    );

    ESP_LOGI(
        TAG,
        "ESP32 LCD1602A TEST"
    );

    ESP_LOGI(
        TAG,
        "4-BIT PARALLEL INTERFACE"
    );

    ESP_LOGI(
        TAG,
        "================================"
    );

    /*
     * Initialize GPIO.
     */

    lcd_gpio_init();

    /*
     * Initialize LCD.
     */

    lcd_init();

    /*
     * Run test.
     */

    lcd_test();

    /*
     * Keep program running.
     */

    while (1)
    {
        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );
    }
}