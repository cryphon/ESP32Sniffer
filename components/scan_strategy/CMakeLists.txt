idf_component_register(
    SRCS "strategy_fixed_channel.c" "strategy_channel_hop.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi
)
