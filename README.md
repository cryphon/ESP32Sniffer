# Espressif's wrapper format

The `rx_ctrl` struct provides the physical layer data (like RSSI).<br>
The `payload` array contains the raw standard network bytes (***IEEE 802.11***).
```c
typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl; // Metadata (RSSI, channel, rate, timestamp)
    uint8_t payload[];          // The actual raw 802.11 Wi-Fi frame bytes
} wifi_promiscuous_pkt_t;
```

## rx_ctrl
```c
typedef struct {
    signed rssi: 8;
    unsigned rate: 5;
    unsinged : 1;
    unsigned sig_mode: 2;
    // ...
    unsinged channel: 4;
    // etc...
} wifi_pkt_rx_ctrl_t;
```

## Payload
The data inside `payload` is strictly based on the ***IEEE 802.11 (Wi-Fi) standard***
## 802.11 MAC Header Layout

| Bytes         | Field            | Size    | Description                              |
|---------------|------------------|---------|-------------------------------------------|
| `payload[0:1]`   | Frame Control    | 2 Bytes | Holds packet Type, Subtype, and flags     |
| `payload[2:3]`   | Duration / ID    | 2 Bytes | Channel allocation time                   |
| `payload[4:9]`   | Address 1        | 6 Bytes | Receiver / Destination MAC Address        |
| `payload[10:15]` | Address 2        | 6 Bytes | Transmitter / Source MAC Address          |
| `payload[16:21]` | Address 3        | 6 Bytes | Filtering Address (BSSID / Router MAC)    |
| `payload[22:23]` | Sequence Control | 2 Bytes | Fragment and sequence numbers             |

> **Note:** This layout covers the standard 3-address header (management frames, and data frames with only one of `To DS`/`From DS` set). Data frames with **both** `To DS` and `From DS` flags set (WDS/mesh links) include a **4th Address field** (6 bytes) inserted immediately after Sequence Control, shifting the start of the frame body by 6 bytes. Check the `To DS`/`From DS` bits in Frame Control before assuming a fixed 24-byte header.
