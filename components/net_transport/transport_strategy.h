#pragma once
#include "esp_check.h"

typedef struct {
    const char* name;
    esp_err_t (*init)(void);
    esp_err_t (*send)(const uint8_t* buf, size_t len);
    void (*deinit)(void);   /* USB/CDC teardown, socket close, etc. */
} transport_strategy_t;

