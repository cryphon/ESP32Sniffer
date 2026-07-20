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
const scan_strategy_t* strategy_fixed_channel_get(uint8_t channel);
