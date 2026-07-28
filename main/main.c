#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include <string.h>

/* Own includes */
#include "wifi_conn.h"
#include "wifi_sniffer.h"
#include "scan_strategy.h"
#include "transport_factory.h"
#include "frame_queue.h"

#define TAG "FightClub"


/* -------------------------------------------------------------- */
static void frame_capture_handler(const uint8_t* buf, size_t len)
{
    static captured_frame_t frame; /* (.bss instead) */
    frame.len = (len > FRAME_MAX_LEN) ? FRAME_MAX_LEN : len;
    memcpy(frame.data, buf, frame.len);
    xQueueSend(s_frame_queue, & frame, 0);
}

static void uplink_task(void* arg)
{
    const transport_strategy_t* transport = (const transport_strategy_t*)arg;
    captured_frame_t frame;

    while (1) {
        if (xQueueReceive(s_frame_queue, &frame, portMAX_DELAY) == pdTRUE) {
            transport->send(frame.data, frame.len);
        }
    }
}

#if CONFIG_SCAN_STRATEGY_HOP
static void uplink_flush(const transport_strategy_t* transport)
{
    esp_wifi_set_promiscuous(false);

    if (wifi_conn_reconnect(5000) != ESP_OK) {
        esp_wifi_set_promiscuous(true);
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(150));  // let ARP resolve before bulk-sending
    transport->init();

    captured_frame_t frame;
    while (xQueueReceive(s_frame_queue, &frame, 0) == pdTRUE) {
        transport->send(frame.data, frame.len);
    }

    transport->deinit();
    wifi_conn_disconnect();
    esp_wifi_set_promiscuous(true);
}
#endif




void app_main(void)
{
    /* NVS required by WIFI driver for calibration data storage */
    ESP_ERROR_CHECK(nvs_flash_init());
    frame_queue_init();

#if CONFIG_TRANSPORT_SOCKET
    wifi_conn_init_and_connect();
#endif

    const transport_strategy_t* transport = transport_get_active();
    
#if CONFIG_SCAN_STRATEGY_FIXED
    transport->init();
    scan_strategy_t* strategy = strategy_fixed_channel_get(CONFIG_SNIFF_CHANNEL);
    xTaskCreate(uplink_task, "uplink_task", 4096, (void*)transport, 5, NULL);
#else
    static const uint8_t hop_channels[] = {1, 6, 11};
    scan_strategy_t* strategy = strategy_channel_hop_get(hop_channels, sizeof(hop_channels), 300);
    wifi_conn_disconnect(); /* release AP before hopping starts */
#endif


    strategy->init();
    wifi_sniffer_start(frame_capture_handler, wifi_sniffer_default_filter());

    uint32_t tick_count = 0;
    while (1) {
    if (strategy->on_tick) strategy->on_tick();
    tick_count++;

#if CONFIG_SCAN_STRATEGY_HOP
    if (tick_count >= CONFIG_UPLINK_FLUSH_TICKS) {
    tick_count = 0;
    ESP_LOGI(TAG, "flush requested; parking on home channel");
    strategy->pause();

    uplink_flush(transport);   // owns connect/send/disconnect internally now
    vTaskDelay(pdMS_TO_TICKS(200));

    strategy->resume();
}
#endif

    uint32_t interval = strategy->tick_interval_ms ? strategy->tick_interval_ms : 1000;
    vTaskDelay(pdMS_TO_TICKS(interval));
    }
}
