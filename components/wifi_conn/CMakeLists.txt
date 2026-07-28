idf_component_register(
    SRCS "wifi_conn.c" "conn_events.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi
)
