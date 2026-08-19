#include "user_input.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static QueueHandle_t thermo_queue = NULL;
TimerHandle_t update_temperature_timer = NULL;
TimerHandle_t ui_update_timer = NULL;
static int user_setpoint;
int fan_mode = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void IRAM_ATTR isr_handler(void *arg) {
    int button = (int)arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(thermo_queue, &button, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void user_input_monitor(void* pvParameters) {
    cmd_t cmd;
    int button_id;

    while (1) {
        if (xQueueReceive(thermo_queue, &button_id, portMAX_DELAY) == pdTRUE) {

            switch (button_id) {
                case POWER {
                    cmd = CMD_OFF;
                    // TRANSMIT NEW CMD
                }
                break;

                case FAN_MODE {
                    fan_mode = !fan_mode;

                    if (fan_mode == 0) {
                        cmd = FAN_AUTO;
                        // TRANSMIT
                    } else {
                        cmd = FAN_ON;
                        // TRANSMIT
                    }
                }
                break;

                case TEMP_UP {
                    set_setpoint(1);
                }
                break;
                
                case TEMP_DOWN {
                    set_setpoint(0);
                }
                break;
            }
        }
    }

}
void user_input_init(void) {

    thermo_queue = xQueueCreate(10, sizeof(int));

    user_setpoint = 20;
    input_buttons = ((1ULL << FAN_MODE) | (1ULL << POWER) | (1ULL << TEMP_UP) | (1ULL << TEMP_DOWN));

    gpio_config_t button_config = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = input_buttons,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    ESP_ERROR_CHECK(gpio_config(&button_config));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(FAN_MODE, isr_handler, (void*)FAN_MODE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(POWER, isr_handler, (void*)POWER));
    ESP_ERROR_CHECK(gpio_isr_handler_add(TEMP_UP, isr_handler, (void*)TEMP_UP));
    ESP_ERROR_CHECK(gpio_isr_handler_add(TEMP_DOWN, isr_handler, (void*)TEMP_DOWN));

}