idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES app_config net_transport wifi_conn wifi_sniffer scan_strategy nvs_flash
)
