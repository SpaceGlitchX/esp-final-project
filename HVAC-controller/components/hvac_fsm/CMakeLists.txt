idf_component_register(
    SRCS 
        "hvac_state_machine.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        freertos 
        esp_timer 
        sensors      
        esp_driver_gpio      # Depends on sensors component
)