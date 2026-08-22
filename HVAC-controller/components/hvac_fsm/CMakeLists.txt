idf_component_register(
    SRCS
        "hvac_state_machine.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_driver_uart
        freertos
        log
        sensors
        hardware
        
)