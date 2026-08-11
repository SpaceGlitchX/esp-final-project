#include "ui.h"

static QueueHandle_t thermo_queue = NULL;
TimerHandle_t update_temperature_timer = NULL;
static int user_setpoint;
extern struct SensorData temp;

char bottom_text[16] = "IN   OUT   SET";

static void IRAM_ATTR isr_handler(void *arg) {
    int button = (int)arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(thermo_queue, &button, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void get_temperature_update(TimerHandle_t xTimer) {
    snprintf(top_text, sizeof(top_text), "%d   %d", temp.indoor_temp, temp.outdoor_temp);
}

void app_main(void) {

    thermo_queue = xQueueCreate(10, sizeof(int));

    user_setpoint = 20;
    input_buttons = ((1ULL << SET_UP) | (1ULL << SET_DWN) | (1ULL << SEL_UP) | (1ULL << SEL_DWN));
    lcd_init();

    gpio_config_t button_config = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = input_buttons,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    ESP_ERROR_CHECK(gpio_config(&button_config));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(SET_UP, isr_handler, (void*)SET_UP));
    ESP_ERROR_CHECK(gpio_isr_handler_add(SET_DWN, isr_handler, (void*)SET_DWN));
    
    lcd_set_cursor(0, 0);
    lcd_send_string(bottom_text);

    update_temperature_timer = xTimerCreate("Temp Timer", pdMS_TO_TICKS(100000), pdTRUE, NULL, get_temperature_update);
    xTimerStart(update_temperature_timer, 100);
}