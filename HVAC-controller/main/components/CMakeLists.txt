idf_component_register(
    SRCS
        "sensor_manager.c"
        "hvac_hardware.c"
        "hvac_state_machine.c"
        "flame_sensor.h"
        "flame_sensor.c"
        "hardware_manager.c"
        "hardware_manager.h"
        "hvac_state_machine.h"
        "hvac_states.h"
        "sensor_manager.h"
        "tach_sensor.h"
        "tach_sensor.c"
        
    INCLUDE_DIRS   "."
    REQUIRES esp_driver_gpio esp_timer esp_driver_pcnt
)