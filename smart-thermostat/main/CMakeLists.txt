idf_component_register(
	SRCS
		"thermo_sensors.c"
		"thermo_logic.c"
		"thermo_comms.c"
		"user_input.c"
		"ui.c"

	REQUIRES
		"driver"
		"esp_driver_gpio"
		"esp_adc"
		"esp_timer"

	INCLUDE_DIRS
		"."
)