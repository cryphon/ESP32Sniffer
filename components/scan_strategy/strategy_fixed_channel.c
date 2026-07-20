#include "scan_strategy.h"

#include "esp_wifi.h"
#include "esp_log.h"

static uint8_t s_channel = 1; /* default, overwritten by _get() */

static esp_err_t fixed_channel_init(void)
{
    ESP_LOGI("STRAT_FIXED_CH", "Setting fixed channel %d", s_channel);
    return esp_wifi_set_channel(s_channel, WIFI_SECOND_CHAN_NONE);
}

static const scan_strategy_t s_strategy = {
    .name             = "fixed_channel",
    .init             = fixed_channel_init,
    .on_tick          = NULL,
    .tick_interval_ms = 0,
};

const scan_strategy_t* strategy_fixed_channel_get(uint8_t channel)
{
    s_channel = channel;
    return &s_strategy;
}
