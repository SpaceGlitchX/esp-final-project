idf_component_register(SRCS "hvac_hardware.c" "hvac_state_machine.c" "sensor_manager.c" "main.c"
"components/sensor_manager.h"
"components/sensor_manger.c"
"components/hvac_hardware.h"
"components/hvac_hardware.c"

                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS "")
