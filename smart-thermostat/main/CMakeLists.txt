idf_component_register(SRCS 
                        "main.c"
                        "thermo_sensors.h"
                        "thermo_ui.h"
                        "thermo_comms.h"
                        PRIV_REQUIRES spi_flash
                        INCLUDE_DIRS "")
