idf_component_register(
    SRCS 
        "main.c"
        
    INCLUDE_DIRS 
        "."
    REQUIRES 
        hvac_fsm 
        sensors
        hardware
        unity
)