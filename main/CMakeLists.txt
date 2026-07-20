idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES app_config net_transport esp_wifi nvs_flash
)
