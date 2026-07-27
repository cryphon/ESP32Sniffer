#pragma once
#include "esp_check.h"

void wifi_conn_init_and_connect();
void wifi_conn_disconnect(void);
esp_err_t wifi_conn_reconnect(uint32_t timeout_ms);
