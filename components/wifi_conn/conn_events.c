#include "conn_events.h"
#include "esp_wifi.h"

EventGroupHandle_t g_conn_events;

static void ip_event_handler(void* arg, esp_event_base_t, int32_t id, void* data)
{
    if(id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(g_conn_events, WIFI_CONNECTED_BIT);
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    if(id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(g_conn_events, WIFI_CONNECTED_BIT);
    }
}


void conn_events_init(void)
{
    g_conn_events = xEventGroupCreate();
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_event_handler, NULL);
}
