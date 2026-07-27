#include "scan_strategy.h"

#include "esp_wifi.h"
#include <string.h>

#define MAX_HOP_CHANNELS 14

static uint8_t  s_channels[MAX_HOP_CHANNELS];
static size_t   s_channel_count;
static size_t   s_idx;


static esp_err_t channel_hop_init(void)
{
    s_idx = 0;
    return esp_wifi_set_channel(s_channels[s_idx], WIFI_SECOND_CHAN_NONE);
}

static void channel_hop_tick(void)
{
    s_idx = (s_idx + 1) % s_channel_count;
    esp_wifi_set_channel(s_channels[s_idx], WIFI_SECOND_CHAN_NONE);
}

static scan_strategy_t s_strategy = {
    .name             = "channel_hop",
    .init             = channel_hop_init,
    .on_tick          = channel_hop_tick,
    .tick_interval_ms = 0,
};

scan_strategy_t* strategy_channel_hop_get(
        const uint8_t* channels,
        size_t channel_count,
        uint32_t dwell_ms)
{
    if(channel_count > MAX_HOP_CHANNELS)
    {
        channel_count = MAX_HOP_CHANNELS;
    }
    memcpy(s_channels, channels, channel_count * sizeof(uint8_t));
    s_channel_count = channel_count;
    s_strategy.tick_interval_ms = dwell_ms;
    return &s_strategy;
}

