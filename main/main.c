#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

/* Own includes */
#include "net_transport.h"
#include "wifi_conn.h"
#include "defs.h"

typedef struct __attribute((packed))
{
    uint16_t protocol_version:  2;
    uint16_t type:              2;
    uint16_t subtype:           4;
    uint16_t to_ds:             1;
    uint16_t from_ds:           1;
    uint16_t more_frag:         1;
    uint16_t retry:             1;
    uint16_t more_data:         1;
    uint16_t protected_frame:   1;
    uint16_t order:             1;
} wifi_frame_control_t;

/* Seq. control (2 bytes) */
typedef struct __attribute((packed))
{
    uint16_t frag_num: 4;
    uint16_t seq_num: 12;
} wifi_seq_control_t;

/* Generic 802.11 MAC header */
typedef struct __attribute((packed))
{
    wifi_frame_control_t frame_control;
    uint16_t duration_id;
    uint8_t addr1[6]; /* Receiver (RA) */
    uint8_t addr2[6]; /* Transmitter (TA) */
    uint8_t addr3[6]; /* BSSID / DA / SA depending on to_ds/from_ds */
    wifi_seq_control_t seq_control;
    /* QoS control (2 bytes) follows here ONLY if subtype indicated QoS data
     * Address 4(6 bytes) follows here ONLY if to_ds && from_ds (WDS) */
} wifi_ieee80211_mac_hdr_t;

/* Frame type values */
enum
{
    WIFI_FRAME_TYPE_MGMT = 0,
    WIFI_FRAME_TYPE_CTRL = 1,
    WIFI_FRAME_TYPE_DATA = 2,
};

/* useful subtypes when type == DATA */
enum
{
    WIFI_FRAME_SUBTYPE_DATA = 0x0,
    WIFI_FRAME_SUBTYPE_QOS_DATA = 0x8,
    WIFI_FRAME_SUBTYPE_QOS_NULL = 0xC,
};


/* -------------------------------------------------------------- */
static void sniff_cb(void* buff, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t* packet = (wifi_promiscuous_pkt_t*)buff;
    wifi_pkt_rx_ctrl_t rx_ctrl = packet->rx_ctrl;
    
    if(rx_ctrl.sig_len < sizeof(wifi_ieee80211_mac_hdr_t))
    {
        return; /* Too short to even hold the base header */
    }

    /* Prepend fixed header with metadata Python needs,
     * then the raw 802.11 frame bytes. */
    typedef struct __attribute__((packed))
    {
        uint16_t sig_len;
        int8_t rssi;
        uint8_t channel;
        uint8_t type;
    } meta_hdr_t;

    meta_hdr_t meta = {
        .sig_len = rx_ctrl.sig_len,
        .rssi = rx_ctrl.rssi,
        .channel = rx_ctrl.channel,
        .type = type,
    };

    const uint8_t* raw = packet->payload;
    const wifi_ieee80211_mac_hdr_t* hdr = (const wifi_ieee80211_mac_hdr_t*)raw;

    size_t hdr_len = sizeof(wifi_ieee80211_mac_hdr_t);

    /* WDS frame -> address 4 present */
    if(hdr->frame_control.to_ds && hdr->frame_control.from_ds)
    {
        hdr_len += 6; /* addr 4 */
    }

    /* QoS data ->2-byte QoS control field present */
    bool is_qos = (hdr->frame_control.type == WIFI_FRAME_TYPE_DATA) &&
        (hdr->frame_control.subtype & 0x08);

    if(is_qos)
    {
        hdr_len += 2;
    }
    
    if(rx_ctrl.sig_len > 512)
    {
        printf("[WARNING] guard sendbuf triggered\n");
        return; /* Guard sendbuf size */
    }

    uint8_t sendbuf[sizeof(meta_hdr_t) + 512];
    memcpy(sendbuf, &meta, sizeof(meta_hdr_t));
    memcpy(sendbuf + sizeof(meta_hdr_t), raw, rx_ctrl.sig_len);

    net_transport_send(sendbuf, sizeof(meta_hdr_t) + rx_ctrl.sig_len);

}

/* -------------------------------------------------------------- */
void app_main(void)
{
    printf("[INFO]\tStarting wifi SNIFF\n");


    /* NVS required by WIFI driver for calibration data storage */
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_conn_init_and_connect();

    net_transport_init();

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&sniff_cb));

    wifi_promiscuous_filter_t filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA };
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));
    ESP_LOGI(TAG, "Connected on channel 6, sniffing here (no channel override)");
    ESP_LOGI(TAG, "Prom. sniffer running on channel %d", SNIFF_CHANNEL);
}

