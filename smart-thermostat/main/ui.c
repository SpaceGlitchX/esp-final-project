#include "ui.h"

extern int outdoor_temperature_raw;
extern int indoor_temperature_raw;

void ui_init(void) {

    user_setpoint = 0;
    input_buttons = ((1ULL << SET_UP) | (1ULL << SET_DWN) | (1ULL << SEL_UP) | (1ULL << SEL_DWN));
    i2c_lcd_init(void);
    lcd_clear();

    gpio_config_t button_config = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = input_buttons,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_in));

    





}