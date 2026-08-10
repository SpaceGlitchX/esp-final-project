#include "thermo_sensors.h"



//~ This file configures ADC UNIT 1 and reads the two temperature sensors. 
//~ Task temperature_sensor_task reads both sensors every 10 seconds then stores and prints their raw ADC values

#define INDOOR_TEMP_CHANNEL ADC_CHANNEL_0 //& GPIO36
#define OUTDOOR_TEMP_CHANNEL ADC_CHANNEL_3 //& GPIO39

//Reads both sensors every 10 sec.
#define SENSOR_PERIOD_MS 10000

//Most recent indoor temp. reading
int indoor_temperature_raw = 0;

//Most recent outdoor temp. reading
int outdoor_temperature_raw = 0;

//Handle used to access ADC unit 1
static adc_oneshot_unit_handle_t adc_handle;

void thermo_sensors_init(void)
{
    //Config for ADC unit 1
    adc_oneshot_unit_init_cfg_t adc_unit_config = {
        .unit_id = ADC_UNIT_1
    };

    //Create the ADC unti
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_config,&adc_handle));

    //Config shared by both ADC channels 
    adc_oneshot_chan_cfg_t adc_channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12 //& allows the adc to measure a wider input-voltage range
    };

    //Config GPIO 36 as indoor temp adc input
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, INDOOR_TEMP_CHANNEL, &adc_channel_config));

    //Config GPIO 39 as outdoor temp adc input
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, OUTDOOR_TEMP_CHANNEL, &adc_channel_config));

    printf("Thermostat temperature sensor initialized \n");

    BaseType_t sensor_task_result;

    //Task that reads both thermistors
    sensor_task_result = xTaskCreate(temperature_sensor_task, "Temperature sensor task", 3072, NULL, 2, NULL);

    //Checks if it was created
    if(sensor_task_result != pdPASS)
    {
        printf("Failed to create temperature sensor task \n");
        return;
    }
    printf("Temperature sensor task was created successfully \n");
}

void temperature_sensor_task(void *pvParameters)
{
    esp_err_t indoor_read_result;
    esp_err_t outdoor_read_result;

    while(1)
    {
        //Reads the indoor thermistor
        indoor_read_result = adc_oneshot_read(adc_handle, INDOOR_TEMP_CHANNEL, &indoor_temperature_raw);

        //Reads the outdoor thermistor
        outdoor_read_result = adc_oneshot_read(adc_handle, OUTDOOR_TEMP_CHANNEL, &outdoor_temperature_raw);

        //Checks if both readings have been received
        if (indoor_read_result == ESP_OK && outdoor_read_result == ESP_OK)
        {
            printf("Indoor ADC: %d, Outdoor ADC: %d \n", indoor_temperature_raw, outdoor_temperature_raw);
        }
        else{
            printf("Failed to read temperature sensors\n");
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}