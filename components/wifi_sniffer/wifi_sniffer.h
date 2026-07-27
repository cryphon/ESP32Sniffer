#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_wifi_types.h"
#include "esp_check.h"

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

typedef struct __attribute__((packed))
    {
        uint16_t sig_len;
        int8_t rssi;
        uint8_t channel;
        uint8_t type;
    } meta_hdr_t;

/* Called once per captured frame, with the meta-header + raw 802.11
 * frame already concat. into a single buf */
typedef void(*wifi_sniffer_frame_cb_t)(const uint8_t* buf, size_t len);

/* Registers the callback, sets the prom. filter , and enables prom. mode.
 * Does NOT touch the channel; call esp_wifi_set_channel() or via scan_strategy
 * before or after this depending on strategy */
esp_err_t wifi_sniffer_start(wifi_sniffer_frame_cb_t on_frame, wifi_promiscuous_filter_t filter);

/* default filter settings */
wifi_promiscuous_filter_t wifi_sniffer_default_filter(void);
