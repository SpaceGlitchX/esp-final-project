#include "thermo_ui_2.c"
//#include <pthread.h>
//#include <unistd.h>

//pthread_mutex_t mutex;
//pthread_cond_t cond;

QueueHandle_t isr_handle;
TaskHandle_t ui_task_handle;
TimerHandle_t timer_handle;

void ui_writer_task(void);
void timerCallback(TimerHandle_t timer);

void app_main(void){

    //pthread_t t1;
    //pthread_t t2;
    //  Define ISR
    isr_handle = xQueueCreate(10, sizeof(int));
    ESP_LOGI("main","interrupt queue created");
    
    button_setup(isr_handle);
    lcd_init();

    timer_handle = xTimerCreate("timer", pdMS_TO_TICKS(250), pdTRUE, NULL, timerCallback);
    ESP_LOGI("main","timer created");

    xTaskCreate(ui_writer_task, "ui_writer_task", 2048, NULL, tskIDLE_PRIORITY, &ui_task_handle);
    ESP_LOGI("main","task created");

    xTimerStart(timer_handle, 0);
    ESP_LOGI("main","timer started");

    /*
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_create(&t1, NULL, &produce, NULL);
    pthread_create(&t2, NULL, &consume, NULL);
    pthread_join(&t1, NULL);
    pthread_join(&t2, NULL);
    */
}

void ui_writer_task(void){
    ESP_LOGI("ui_writer_task","task started");
    for(;;){
        int pinAddress = 0;

        if(xQueueReceive(isr_handle, &pinAddress, portMAX_DELAY)){
            ESP_LOGI("timer","interrupt from pin %d",(pinAddress));
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void timerCallback(TimerHandle_t timer){
    ESP_LOGI("timerCallback","timer started");
    
}