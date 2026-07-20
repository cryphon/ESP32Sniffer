#include "wifi_sniffer.h"

#include <string.h>
#include <stdio.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "defs.h"

static wifi_sniffer_frame_cb_t s_on_frame = NULL;

static void sniff_cb(void* buff, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t* packet = (wifi_promiscuous_pkt_t*)buff;
    wifi_pkt_rx_ctrl_t rx_ctrl = packet->rx_ctrl;

    if (rx_ctrl.sig_len < sizeof(wifi_ieee80211_mac_hdr_t))
    {
        return; /* Too short to even hold the base header */
    }

    if (s_on_frame == NULL) return;

    meta_hdr_t meta = {
        .sig_len = rx_ctrl.sig_len,
        .rssi    = rx_ctrl.rssi,
        .channel = rx_ctrl.channel,
        .type    = type,
    };

    const uint8_t* raw = packet->payload;
    const wifi_ieee80211_mac_hdr_t* hdr = (const wifi_ieee80211_mac_hdr_t*)raw;

    size_t hdr_len = sizeof(wifi_ieee80211_mac_hdr_t);

    /* WDS frame -> address 4 present */
    if (hdr->frame_control.to_ds && hdr->frame_control.from_ds)
    {
        hdr_len += 6;
    }

    /* QoS data -> 2-byte QoS control field present */
    bool is_qos = (hdr->frame_control.type == WIFI_FRAME_TYPE_DATA) &&
                  (hdr->frame_control.subtype & 0x08);
    if (is_qos)
    {
        hdr_len += 2;
    }
    (void)hdr_len; /* currently unused beyond validation; kept for future parsing */

    if (rx_ctrl.sig_len > 512)
    {
        printf("[WARNING] guard sendbuf triggered\n");
        return; /* Guard sendbuf size */
    }

    uint8_t sendbuf[sizeof(meta_hdr_t) + 512];
    memcpy(sendbuf, &meta, sizeof(meta_hdr_t));
    memcpy(sendbuf + sizeof(meta_hdr_t), raw, rx_ctrl.sig_len);

    s_on_frame(sendbuf, sizeof(meta_hdr_t) + rx_ctrl.sig_len);
}

esp_err_t wifi_sniffer_start(wifi_sniffer_frame_cb_t on_frame,
                              wifi_promiscuous_filter_t filter)
{
    if (on_frame == NULL)
    {
        ESP_LOGE(TAG, "on_frame callback must not be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    s_on_frame = on_frame;

    esp_err_t err = esp_wifi_set_promiscuous(true);
    if (err != ESP_OK) return err;

    err = esp_wifi_set_promiscuous_rx_cb(&sniff_cb);
    if (err != ESP_OK) return err;

    return esp_wifi_set_promiscuous_filter(&filter);
}

wifi_promiscuous_filter_t wifi_sniffer_default_filter(void)
{
    return (wifi_promiscuous_filter_t){
        .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA
    };
}

