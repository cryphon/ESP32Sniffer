#include "wifi_conn.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"

#define WIFI_CONNECTED_BIT BIT0
#define TAG "FightClub"

static EventGroupHandle_t wifi_event_group;
static bool s_manual_disconnect_expected = false;

static void ip_event_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data)
{
    if(event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t* disc = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGW(TAG, "Disconnected, reason: %d", disc->reason);
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);

        if (!s_manual_disconnect_expected) {
            esp_wifi_connect();  /* only auto-retry on unexpected drops */
        }
    }
}
void wifi_conn_init_and_connect()
{
    printf("[INFO]\tStarting wifi...\n");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "create default wifi STA...");
    esp_netif_create_default_wifi_sta();

    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                &ip_event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    /* STA mode + connect, get IP and route to the PC */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    /* Block here until an IP has been retrieved */
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
            pdFALSE, pdTRUE, portMAX_DELAY);

}

void wifi_conn_disconnect(void)
{
    s_manual_disconnect_expected = true;
    esp_wifi_disconnect();
}

esp_err_t wifi_conn_reconnect(uint32_t timeout_ms)
{
    s_manual_disconnect_expected = false;
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    esp_wifi_connect();

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                                           pdFALSE, pdTRUE, pdMS_TO_TICKS(timeout_ms));
    if (!(bits & WIFI_CONNECTED_BIT)) {
        ESP_LOGW(TAG, "reconnect timed out after %lu ms", timeout_ms);
    }
    return (bits & WIFI_CONNECTED_BIT) ? ESP_OK : ESP_FAIL;
}
