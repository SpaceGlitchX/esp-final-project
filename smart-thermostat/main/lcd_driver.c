/*
Based on the 1602 driver written by Aad van Gerwen
https://components.espressif.com/components/vgerwen/lcd1602
*/

#include "lcd_driver.h"

static void lcd_enable(void){
    gpio_set_level(LCD_E, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(LCD_E, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(LCD_E, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
}

static void send_nibble(gpio_num_t data, unsigned char c){
    uint8_t i;
    for (i = 0; i < 4; i++) {
        // Send each bit through its corresponding pin; use 0x01 to cut out the empty byte
        gpio_set_level(data[i], (c >> i) & 0x01);
    }
}

static void lcd_command(unsigned char command){
    gpio_set_level(LCD_RS, 0);  // Set RS to 0 to enter command mode

    // upper 4 bits
    send_nibble(command >> 4);
    lcd_enable();

    // lower 4 bits
    send_nibble(command);
    lcd_enable();

    vTaskDelay(pdMS_TO_TICKS(10));
}

static void lcd_write_byte(unsigned char data){
    gpio_set_level(LCD_RS, 1);  // Set RS to 1 to enter write mode

    // upper 4 bits
    send_nibble(data >> 4);
    lcd_enable();

    // lower 4 bits
    send_nibble(data);
    lcd_enable();

    vTaskDelay(pdMS_TO_TICKS(10));
}

void lcd_init(void){
    //  Set up the LCD pins
    ESP_ERROR_CHECK(gpio_set_direction(LCD_E, GPIO_MODE_OUTPUT));
    gpio_pulldown_dis(LCD_E);
    gpio_pullup_dis(LCD_E);
    ESP_ERROR_CHECK(gpio_set_direction(LCD_RS, GPIO_MODE_OUTPUT));
    gpio_pulldown_dis(LCD_RS);
    gpio_pullup_dis(LCD_RS);
    ESP_ERROR_CHECK(gpio_set_direction(LCD_D7, GPIO_MODE_OUTPUT));
    gpio_pulldown_dis(LCD_D7);
    gpio_pullup_dis(LCD_D7);
    ESP_ERROR_CHECK(gpio_set_direction(LCD_D6, GPIO_MODE_OUTPUT));
    gpio_pulldown_dis(LCD_D6);
    gpio_pullup_dis(LCD_D6);
    ESP_ERROR_CHECK(gpio_set_direction(LCD_D5, GPIO_MODE_OUTPUT));
    gpio_pulldown_dis(LCD_D5);
    gpio_pullup_dis(LCD_D5);
    ESP_ERROR_CHECK(gpio_set_direction(LCD_D4, GPIO_MODE_OUTPUT));
    gpio_pulldown_dis(LCD_D4);
    gpio_pullup_dis(LCD_D4);
    ESP_LOGI("lcd_setup","pin setup complete");

    dac_oneshot_new_channel(&dac_config, &dac_handle);
    dac_oneshot_output_voltage(dac_handle, dac_high);   // set dac to full voltage on startup, according to best practice from datasheet
    ESP_LOGI("lcd_setup","DAC channel initialized");

    // 100 ms delay
    vTaskDelay(pdMS_TO_TICKS(100)); // From datasheet: Must wait >15ms after powering on before sending anything

    // SET 0x03
    gpio_set_level(LCD_E, 0);
    gpio_set_level(LCD_RS, 0);
    gpio_set_level(LCD_D4, 1);
    gpio_set_level(LCD_D5, 1);
    gpio_set_level(LCD_D6, 0);
    gpio_set_level(LCD_D7, 0);

    /*  LATCH 0x03
    The lcd_enable() function latches the 0x03 to the LCD
    This is repeated three times
    From datasheet: 
        Must wait >4.5ms after sending the first pulse
        Must wait >100us after sending the second pulse
        No further delay is needed after the third pulse
    For convenience, we will set a standard delay between each
    */ 
    lcd_enable();
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd_enable();
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd_enable();
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd_enable();

    /*  SET 0x02
    This is to enable 4 bit mode
    */ 
    gpio_set_level(LCD_E, 0);
    gpio_set_level(LCD_RS, 0);
    gpio_set_level(LCD_D4, 0);
    gpio_set_level(LCD_D5, 1);
    gpio_set_level(LCD_D6, 0);
    gpio_set_level(LCD_D7, 0);

    // LATCH 0x02
    lcd_enable();
    vTaskDelay(pdMS_TO_TICKS(10));

    /*  CONFIGURE THE DISPLAY
    After setting and latching 0x02, the LCD is going to be operating on 4 bit communication
    The final lines below are for configuring the display

    For reference, an excerpt from Aad van Gerwen's driver:
        lcdWriteCmd(0x28, LCD_CMD); // 4-bit: DL=0, 2-line: N=1, 5x8: F=0
        lcdWriteCmd(0x08, LCD_CMD); // Instruction Flow was Display Off: D=0 cursor off C=0 blinking off B=0
        lcdWriteCmd(0x01, LCD_CMD); // Clear LCD cursor home
        lcdWriteCmd(0x06, LCD_CMD); // Auto-Increment I/D=1 S=0
        lcdWriteCmd(0x0C, LCD_CMD); // Display On, No blink was 0c
    
    From the datasheet, the config block is:
    RS  RW  D7  D6  D5  D4
    0   0   0   0   1   0 <-- This instruction was completed above
    0   0   0   0   1   0 <-- The next commands start here
    0   0   N   F   X   X
    0   0   0   0   0   0
    0   0   1   0   0   0
    0   0   0   0   0   0
    0   0   0   0   0   1
    0   0   0   0   0   0
    0   0   0   1  I/D  S
    
    */
    lcd_command(0x28);
    lcd_command(0x08);
    lcd_command(0x01);
    lcd_command(0x06);
    lcd_command(0x0C);
}
