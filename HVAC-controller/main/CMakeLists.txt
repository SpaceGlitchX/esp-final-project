idf_component_register(
    SRCS 
        "KY-026.c"
        
    INCLUDE_DIRS 
        "."
    REQUIRES 
        hvac_fsm 
        sensors
        hardware
        unity
        
        hvac_comms
        esp_adc
        esp_driver_ledc
        esp_driver_gpio
        esp_timer
        freertos
        esp_driver_pcnt
)