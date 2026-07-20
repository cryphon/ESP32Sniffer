#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

/* Own includes */
#include "net_transport.h"
#include "wifi_conn.h"
#include "wifi_sniffer.h"
#include "scan_strategy.h"
#include "defs.h"

/* -------------------------------------------------------------- */

static void on_frame_captured(const uint8_t* buf, size_t len) {
    net_transport_send(buf, len);
}

void app_main(void)
{
    /* NVS required by WIFI driver for calibration data storage */
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_conn_init_and_connect();
    net_transport_init();

    const scan_strategy_t* strategy = strategy_fixed_channel_get(SNIFF_CHANNEL);

    strategy->init();
    wifi_sniffer_start(on_frame_captured, wifi_sniffer_default_filter());

    while (1) {
    if (strategy->on_tick) strategy->on_tick();

    uint32_t interval = strategy->tick_interval_ms ? strategy->tick_interval_ms : 1000;
    vTaskDelay(pdMS_TO_TICKS(interval));
    }
}
