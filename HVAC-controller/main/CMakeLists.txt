idf_component_register(
    SRCS 
        
        "main.c"
        "test_harness.c"
        
    INCLUDE_DIRS 
        "."
    REQUIRES 
        hvac_fsm 
        sensors
        hardware
        unity
        esp_driver_spi
        hvac_comms
        esp_adc
        esp_driver_ledc
        esp_driver_gpio
        esp_timer
        freertos
        esp_driver_pcnt
        console
)