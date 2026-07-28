# FightClub

An ESP322based 802.11 promiscuous-mode snuffer with configurable channel scanning (fixed-channel or channel-hopping) and pluggable uplink (WiFi socke , with USB as extensible option), paired with Python terminal dashboard for live station monitoring.

## Configuration
All runtime configuration lives in `menuconfig`, under **FightClub** top-level menu.
__Example__
```bash
idf.py menuconfig
```
Inside FightClub you'll find:

| Option | Description |
|---|---|
| **Scan Strategy** | Choose *Fixed channel* or *Channel hopping* |
| **Fixed channel number** | Channel to sit on (only shown if Fixed channel is selected) |
| **Hop dwell time (ms)** | Time spent per channel before hopping (only shown if Channel hopping is selected) |
| **Uplink Transport** | Choose *WiFi socket* (USB scaffolding present, not yet a drop-in option here) |
| **Ticks between uplink flushes** | For hop mode — how many hop cycles occur before the device briefly reconnects to flush buffered frames |
| **WiFi SSID / WiFi Password** | Your network credentials |
| **Socket Target IP / Port** | The IP address and port of the PC running `analyzer.py` |

> **Note:** WiFi credentials are stored in `sdkconfig`, which is gitignored — this file is not meant to be committed. If you're setting this up fresh, running `menuconfig` will prompt you to fill these in.


### How the two scan strategies behave
 
- **Fixed channel** — the device stays connected to your WiFi network the entire time and streams frames continuously as they're captured.
- **Channel hopping** — the device must disconnect from WiFi to freely hop channels (a WiFi radio can't stay associated to an AP and roam channels simultaneously). It buffers captured frames locally, then periodically reconnects for a short window to flush the buffer, before disconnecting and resuming the hop.

## Build & Flash
```
idf.py set-target esp32
idf.py menuconfig     # configure as above
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```
(Replace `/dev/ttyUSB0` with your board's actual serial port.)

## Running the analyzer
 
On your PC, make sure the target IP/port configured above points at this machine, then:
 
```
python analyzer.py
```
 
Optional flag:
```
python analyzer.py --censor   # masks the last 3 octets of MAC/BSSID addresses
```

The dashboard updates live as frames arrive:
<img width="1281" height="720" alt="image" src="https://github.com/user-attachments/assets/9cda84d8-ef52-434f-8ded-83d4fe6fec71" />

> Press v while the sniffer is running to toggle between the Leaderboard view (top 20 stations by packet count, best for fixed-channel mode) and the Full List view (every station discovered so far, sorted by first-seen — better suited for hop mode, where lower packet counts make the leaderboard less meaningful).
## Notes
 
- Channel-hop mode involves periodic reconnect windows and is inherently less real-time than fixed-channel mode — tune **Hop dwell time** and **Ticks between uplink flushes** to trade off channel coverage against uplink freshness.
- Frame size is capped (currently 1472 bytes) to stay within a single UDP datagram and avoid IP fragmentation; occasional oversized 802.11 data frames are dropped rather than fragmented.

