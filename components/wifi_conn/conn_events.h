#pragma once
#include "esp_event.h"

#define WIFI_CONNECTED_BIT BIT0
extern EventGroupHandle_t g_conn_events;

void conn_events_init(void);
