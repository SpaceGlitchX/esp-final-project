#include "thermo_ui.h"



void app_main(void)
{
    //  Define ISR
    isr_handle = xQueueCreate(10, sizeof(int));
    ESP_LOGI("main","interrupt queue created");

    timer_handle = xTimerCreate("timer", pdMS_TO_TICKS(250), pdTRUE, NULL, timerCallback);
    ESP_LOGI("main","timer created");

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    gpio_isr_handler_add(SET_UP, set_up_isr, (void *)SET_UP);
    gpio_isr_handler_add(SET_DWN, set_dwn_isr, (void *)SET_DWN);
    gpio_isr_handler_add(SEL_UP, sel_up_isr, (void *)SEL_UP);
    gpio_isr_handler_add(SEL_DWN, sel_dwn_isr, (void *)SEL_DWN);
    ESP_LOGI("main","ISRs created");

    xTimerStart(timer_handle, 0);
    ESP_LOGI("main","timer started");
}

void lcd_setup(void){
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
}
void button_setup(void){
    ESP_ERROR_CHECK(gpio_set_direction(SET_UP, GPIO_MODE_INPUT));
    gpio_pulldown_dis(SET_UP); 
    gpio_pullup_en(SET_UP);
    ESP_ERROR_CHECK(gpio_set_direction(SET_DWN, GPIO_MODE_INPUT));
    gpio_pulldown_dis(SET_DWN); 
    gpio_pullup_en(SET_DWN);
    ESP_ERROR_CHECK(gpio_set_direction(SEL_UP, GPIO_MODE_INPUT));
    gpio_pulldown_dis(SEL_UP); 
    gpio_pullup_en(SEL_UP);
    ESP_ERROR_CHECK(gpio_set_direction(SEL_DWN, GPIO_MODE_INPUT));
    gpio_pulldown_dis(SEL_DWN); 
    gpio_pullup_en(SEL_DWN);
    ESP_LOGI("button_setup","pin setup complete");
}

void timerCallback(TimerHandle_t timer){

    int pinAddress = 0;

    if(xQueueReceive(isr_handle, &pinAddress, portMAX_DELAY)){
        bmap = 0000 & 0xFF; // clear bmap
        if(pinAddress == SET_UP){bmap = 0001 & 0xFF;}
        if(pinAddress == SET_DWN){bmap = 0010 & 0xFF;}
        if(pinAddress == SEL_UP){bmap = 0100 & 0xFF;}
        if(pinAddress == SEL_DWN){bmap = 1000 & 0xFF;}
        ESP_LOGI("timer","interrupt from pin %d",(pinAddress));
    }
}

static void IRAM_ATTR set_up_isr(void *param){
    int pin = (int)param;
    xQueueSendFromISR(isr_handle, &pin, NULL);
}
static void IRAM_ATTR set_dwn_isr(void *param){
    int pin = (int)param;
    xQueueSendFromISR(isr_handle, &pin, NULL);
}
static void IRAM_ATTR sel_up_isr(void *param){
    int pin = (int)param;
    xQueueSendFromISR(isr_handle, &pin, NULL);
}
static void IRAM_ATTR sel_dwn_isr(void *param){
    int pin = (int)param;
    xQueueSendFromISR(isr_handle, &pin, NULL);
}

