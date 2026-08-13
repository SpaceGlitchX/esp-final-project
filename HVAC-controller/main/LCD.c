
#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "LCD1602";

/* =========================================================
 * I2C CONFIGURATION
 * ========================================================= */

#define I2C_MASTER_NUM          I2C_NUM_0

#define I2C_MASTER_SDA_IO       GPIO_NUM_21
#define I2C_MASTER_SCL_IO       GPIO_NUM_22

#define I2C_MASTER_FREQ_HZ      100000

#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TX_BUF_DISABLE   0

/* =========================================================
 * LCD CONFIGURATION
 * ========================================================= */

/*
 * Change this if your LCD uses a different I2C address.
 *
 * Common addresses:
 *     0x27
 *     0x3F
 */
#define LCD_I2C_ADDRESS         0x27

/*
 * Control bytes.
 *
 * These are for the LCD's I2C interface.
 */
#define LCD_CONTROL_COMMAND     0x00
#define LCD_CONTROL_DATA        0x40

/*
 * LCD commands
 */
#define LCD_CMD_CLEAR_DISPLAY   0x01
#define LCD_CMD_RETURN_HOME     0x02
#define LCD_CMD_ENTRY_MODE_SET  0x06
#define LCD_CMD_DISPLAY_CONTROL 0x0C
#define LCD_CMD_FUNCTION_SET    0x38

/* =========================================================
 * I2C INITIALIZATION
 * ========================================================= */

esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,

        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,

        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,

        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err;

    err = i2c_param_config(
        I2C_MASTER_NUM,
        &conf
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "I2C parameter configuration failed: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    err = i2c_driver_install(
        I2C_MASTER_NUM,
        conf.mode,
        I2C_MASTER_RX_BUF_DISABLE,
        I2C_MASTER_TX_BUF_DISABLE,
        0
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "I2C driver installation failed: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    ESP_LOGI(
        TAG,
        "I2C initialized: SDA=%d, SCL=%d, Frequency=%d Hz",
        I2C_MASTER_SDA_IO,
        I2C_MASTER_SCL_IO,
        I2C_MASTER_FREQ_HZ
    );

    return ESP_OK;
}

/* =========================================================
 * SEND LCD COMMAND
 * ========================================================= */

void lcd_send_command(uint8_t cmd)
{
    uint8_t write_buf[2];

    write_buf[0] = LCD_CONTROL_COMMAND;
    write_buf[1] = cmd;

    esp_err_t err =
        i2c_master_write_to_device(
            I2C_MASTER_NUM,
            LCD_I2C_ADDRESS,
            write_buf,
            sizeof(write_buf),
            pdMS_TO_TICKS(1000)
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "LCD command transfer failed: %s",
            esp_err_to_name(err)
        );
    }
}

/* =========================================================
 * SEND LCD DATA
 * ========================================================= */

void lcd_send_data(uint8_t data)
{
    uint8_t write_buf[2];

    write_buf[0] = LCD_CONTROL_DATA;
    write_buf[1] = data;

    esp_err_t err =
        i2c_master_write_to_device(
            I2C_MASTER_NUM,
            LCD_I2C_ADDRESS,
            write_buf,
            sizeof(write_buf),
            pdMS_TO_TICKS(1000)
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "LCD data transfer failed: %s",
            esp_err_to_name(err)
        );
    }
}

/* =========================================================
 * LCD INITIALIZATION
 * ========================================================= */

void lcd_init(void)
{
    /*
     * Allow LCD controller to power up.
     */
    vTaskDelay(
        pdMS_TO_TICKS(50)
    );

    /*
     * Function set:
     *
     * 8-bit
     * 2-line
     * 5x8 character font
     */
    lcd_send_command(
        LCD_CMD_FUNCTION_SET
    );

    vTaskDelay(
        pdMS_TO_TICKS(5)
    );

    /*
     * Display ON
     * Cursor OFF
     * Blink OFF
     */
    lcd_send_command(
        LCD_CMD_DISPLAY_CONTROL
    );

    vTaskDelay(
        pdMS_TO_TICKS(5)
    );

    /*
     * Clear display.
     */
    lcd_send_command(
        LCD_CMD_CLEAR_DISPLAY
    );

    vTaskDelay(
        pdMS_TO_TICKS(5)
    );

    /*
     * Entry mode:
     * Increment cursor after each character.
     */
    lcd_send_command(
        LCD_CMD_ENTRY_MODE_SET
    );

    vTaskDelay(
        pdMS_TO_TICKS(5)
    );

    ESP_LOGI(
        TAG,
        "LCD initialization complete"
    );
}

/* =========================================================
 * SET LCD CURSOR
 * ========================================================= */

void lcd_set_cursor(
    uint8_t row,
    uint8_t col
)
{
    uint8_t address;

    if (row == 0)
    {
        address = 0x00 + col;
    }
    else
    {
        address = 0x40 + col;
    }

    lcd_send_command(
        0x80 | address
    );
}

/* =========================================================
 * SEND STRING
 * ========================================================= */

void lcd_send_string(
    const char *str
)
{
    if (str == NULL)
    {
        return;
    }

    while (*str)
    {
        lcd_send_data(
            (uint8_t)*str
        );

        str++;
    }
}

/* =========================================================
 * CLEAR LCD
 * ========================================================= */

void lcd_clear(void)
{
    lcd_send_command(
        LCD_CMD_CLEAR_DISPLAY
    );

    /*
     * Clear display requires a little
     * more processing time.
     */
    vTaskDelay(
        pdMS_TO_TICKS(5)
    );
}

/* =========================================================
 * LCD TEST
 * ========================================================= */

static void lcd_test(void)
{
    ESP_LOGI(
        TAG,
        "Starting LCD test..."
    );

    /*
     * Test 1
     */
    lcd_clear();

    lcd_set_cursor(
        0,
        0
    );

    lcd_send_string(
        "LCD TEST"
    );

    lcd_set_cursor(
        1,
        0
    );

    lcd_send_string(
        "I2C WORKING!"
    );

    vTaskDelay(
        pdMS_TO_TICKS(3000)
    );

    /*
     * Test 2
     */
    lcd_clear();

    lcd_set_cursor(
        0,
        0
    );

    lcd_send_string(
        "ESP32"
    );

    lcd_set_cursor(
        1,
        0
    );

    lcd_send_string(
        "LCD1602 TEST"
    );

    vTaskDelay(
        pdMS_TO_TICKS(3000)
    );

    /*
     * Test 3
     */
    lcd_clear();

    lcd_set_cursor(
        0,
        0
    );

    lcd_send_string(
        "Row 1: PASS"
    );

    lcd_set_cursor(
        1,
        0
    );

    lcd_send_string(
        "Row 2: PASS"
    );

    ESP_LOGI(
        TAG,
        "LCD test complete"
    );
}

/* =========================================================
 * APP MAIN
 * ========================================================= */

void app_main(void)
{
    ESP_LOGI(
        TAG,
        "Starting standalone LCD test"
    );

    /*
     * Initialize I2C FIRST.
     */
    ESP_ERROR_CHECK(
        i2c_master_init()
    );

    /*
     * Initialize LCD.
     */
    lcd_init();

    /*
     * Run LCD test.
     */
    lcd_test();

    /*
     * Keep program alive.
     */
    while (1)
    {
        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );
    }
}