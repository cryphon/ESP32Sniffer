#include <stdint.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"


typedef struct __attribute((packed))
{
    uint16_t protocol_version:      2;
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

static const char* TAG = "SNIF";

/* Fixed channel for now, can hop later */
#define SNIFF_CHANNEL 1

static void sniff_cb(void* buff, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t* packet = (wifi_promiscuous_pkt_t*)buff;
    wifi_pkt_rx_ctrl_t rx_ctrl = packet->rx_ctrl;
    
    if(rx_ctrl.sig_len < sizeof(wifi_ieee80211_mac_hdr_t))
    {
        return; /* Too short to even hold the base header */
    }


    const uint8_t* raw = packet->payload;
    const wifi_ieee80211_mac_hdr_t* hdr = (const wifi_ieee80211_mac_hdr_t*)raw;

    size_t hdr_len = sizeof(wifi_ieee80211_mac_hdr_t);

    /* WDS frame -> address 4 present */
    if(hdr->frame_control.to_ds && hdr->frame_control.from_ds)
    {
        hdr_len += 6;
    }

    /* QoS data ->2-byte QoS control field present */
    bool is_qos = (hdr->frame_control.type == WIFI_FRAME_TYPE_DATA) &&
        (hdr->frame_control.subtype & 0x08);

    if(is_qos)
    {
        hdr_len += 2;
    }
    

    printf("---- frame len=%d rssi=%d channel=%d type=%d ----\n",
           rx_ctrl.sig_len, rx_ctrl.rssi, rx_ctrl.channel, type);

    printf("fc: ver=%d type=%d subtype=%d toDS=%d fromDS=%d\n",
           hdr->frame_control.protocol_version,
           hdr->frame_control.type,
           hdr->frame_control.subtype,
           hdr->frame_control.to_ds,
           hdr->frame_control.from_ds);

    printf("addr1=%02x:%02x:%02x:%02x:%02x:%02x\n",
            hdr->addr1[0], hdr->addr1[1], hdr->addr1[2],
            hdr->addr1[3], hdr->addr1[4], hdr->addr1[5]);
    printf("addr2=%02x:%02x:%02x:%02x:%02x:%02x\n",
            hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],
            hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]);
    printf("addr3=%02x:%02x:%02x:%02x:%02x:%02x\n",
            hdr->addr3[0], hdr->addr3[1], hdr->addr3[2],
            hdr->addr3[3], hdr->addr3[4], hdr->addr3[5]);


    if (rx_ctrl.sig_len > hdr_len) {
        const uint8_t* body = raw + hdr_len;
        size_t body_len = rx_ctrl.sig_len - hdr_len;
        // hex-dump body as before, or hand off to (not-yet-created) NDP parser
    }



}


void app_main(void)
{
    printf("[INFO]\tStarting wifi SNIFF\n");

    /* NVS required by WIFI driver for calibration data storage */
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* No AP mode needed for radio, prom. just needs radio up */
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&sniff_cb));

    wifi_promiscuous_filter_t filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA };
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));

    ESP_ERROR_CHECK(esp_wifi_set_channel(SNIFF_CHANNEL, WIFI_SECOND_CHAN_NONE));

    ESP_LOGI(TAG, "Prom. sniffer running on channel %d", SNIFF_CHANNEL);
}
