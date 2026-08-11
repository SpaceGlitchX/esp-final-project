idf_component_register(
    SRCS 
        "flame_sensor.c"
        "tach_sensor.c"
        "sensor_manager.c"
    
        INCLUDE_DIRS 
        "include"
    REQUIRES 
        freertos 
        driver
        hardware
        sensors
        hvac_fsm
        esp_adc
        esp_driver_pcnt
        esp_driver_gpio
)