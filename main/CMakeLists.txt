idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES frame_queue net_transport wifi_conn wifi_sniffer scan_strategy nvs_flash
)
