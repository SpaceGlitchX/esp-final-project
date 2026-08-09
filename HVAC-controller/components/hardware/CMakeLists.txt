idf_component_register(
    SRCS 
        "hardware_manager.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        freertos 
        esp_timer 
        sensors 
        driver      
        esp_driver_gpio     # Depends on sensors component
)