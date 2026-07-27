#pragma once
#include "esp_check.h"
#include <stdint.h>

typedef struct
{
    const char* name;
    esp_err_t (*init)(void);    /* Called once before wifi_sniffer_start() */
    void (*on_tick)(void);      /* Called periodically from main's loop; may be NULL */
    uint32_t tick_interval_ms;  /* Ignored if on_tick is NULL */
} scan_strategy_t;

/* Returns strategy that sets the radio to a single fixed channel once */
scan_strategy_t* strategy_fixed_channel_get(uint8_t channel);

/* channels: array of 802.11 channel numbers to hop accross, e.g. {1,6,11}
 * channel_count: number of entries in channels
 * dwell_ms: how long to sit on each channel before hopping */
scan_strategy_t* strategy_channel_hop_get(
        const uint8_t* channels,
        size_t channel_count,
        uint32_t dwell_ms);

