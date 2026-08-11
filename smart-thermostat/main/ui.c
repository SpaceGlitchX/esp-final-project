#include "ui.h"

extern int outdoor_temperature_raw;
extern int indoor_temperature_raw;
static QueueHandle_t thermo_queue = NULL;
TimerHandle_t update_temperature_timer = NULL;

char* bottom_text = "IN   OUT   SET";

static void IRAM_ATTR isr_handler(void *arg) {
    int button = (int)args;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(thermo_queue, &button, &xHigherPriorityTaskWoken);
    if (xHigherTaskPriorityWoken) {
        portYIELD_FROM_ISR();
    }
}

void get_temperature_raw(TimerHandle_t xTimer) {

}
void ui_init(void) {

    thermo_queue = xQueueCreate(10, sizeof(int));

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

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(SET_UP, isr_handler, (void*)SET_UP));
    ESP_ERROR_CHECK(gpio_isr_handler_add(SET_DWN, isr_handler, (void*)SET_DWN));
    
    lcd_write(bottom_text, 0, 0);

    update_temperature_timer = xTimerCreate("Temp Timer", pdMS_TO_TICKS(100000), pdTRUE, NULL, get_temperature_raw);
    xTimerStart(update_temperature_timer, 100);
}