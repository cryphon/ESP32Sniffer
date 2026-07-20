import socket
import struct
import time
import argparse
from collections import defaultdict
from rich.live import Live
from rich.table import Table
from rich.console import Console
from manuf import manuf
# -------------------------------------------------------------------------------#

parser_arg = argparse.ArgumentParser()
parser_arg.add_argument(
    "--censor", action="store_true",
    help="Mask the last 3 octets of MAC/BSSID (keeps OUI visible) for demo/screenshot use"
)
args = parser_arg.parse_args()



# Matches meta_hdr_t: siglen (u16), rssi (i8), channel (u8), type (u8)
META_FMT = "<H b B B"
META_LEN = struct.calcsize(META_FMT)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", 5005))

# Manufactoring parser
parser = manuf.MacParser()

# -------------------------------------------------------------------------------#

def parse_frame_control(fc_raw: int):
    # Mirror frame_control bitfield, but unpacked from raw u16 
    return {
        "version":  fc_raw & 0x3,
        "type":     (fc_raw >> 2) & 0x3,
        "subtype":  (fc_raw >> 4) & 0xF,
        "to_ds":    (fc_raw >> 8) & 0x1,
        "from_ds":  (fc_raw >> 9) & 0x1,
        "more_frag":(fc_raw >> 10) & 0x1,
        "retry":    (fc_raw >> 11) & 0x1,
        "pwr_mgmt": (fc_raw >> 12) & 0x1,
        "more_data":(fc_raw >> 13) & 0x1,
        "protected":(fc_raw >> 14) & 0x1,
        "order":    (fc_raw >> 15) & 0x1,
    }

# -------------------------------------------------------------------------------#

console = Console()

SUBTYPE_NAMES = {
    0x0: "Data", 0x4: "Null", 0x8: "QoS Data", 0xC: "QoS Null",
}
TYPE_NAMES = {0: "Mgmt", 1: "Ctrl", 2: "Data"}

stations = defaultdict(lambda: {
    "count": 0,
    "last_seen": 0.0,
    "vendor": "",
    "rssi": 0,
    "channel": 0,
    "subtype": "",
    "pwr": 0,
    "bssid": "",
})

def make_table():
    table = Table(title="WiFi Sniffer — Station Leaderboard")
    table.add_column("MAC", style="cyan")
    table.add_column("BSSID", style="dim")
    table.add_column("Pkts", justify="right", style="green")
    table.add_column("RSSI", justify="right")
    table.add_column("Ch", justify="right")
    table.add_column("Type", justify="center")
    table.add_column("Pwr", justify="center")
    table.add_column("Last seen", justify="right")
    table.add_column("Vendor", style="magenta", overflow="ellipsis")

    now = time.time()
    # Leaderboard: sort by packet count, most active first
    sorted_stations = sorted(stations.items(), key=lambda kv: -kv[1]["count"])

    for mac, s in sorted_stations[:20]:  # cap display to top 20
        age = now - s["last_seen"]
        pwr_str = "[yellow]sleep[/]" if s["pwr"] else "[green]awake[/]"
        rssi_str = f"{s['rssi']} dBm"
        if s["rssi"] > -60:
            rssi_style = "green"
        elif s["rssi"] > -80:
            rssi_style = "yellow"
        else:
            rssi_style = "red"

        display_mac = censor_mac(mac) if args.censor else mac
        display_bssid = censor_mac(s["bssid"]) if args.censor and s["bssid"] else s["bssid"]

        table.add_row(
            display_mac,
            display_bssid,
            str(s["count"]),
            f"[{rssi_style}]{rssi_str}[/{rssi_style}]",
            str(s["channel"]),
            s["subtype"],
            pwr_str,
            f"{age:.1f}s ago",
            s["vendor"],
        )
    return table

# -------------------------------------------------------------------------------#

def is_randomized(mac_str: str) -> bool:
    first_octet = int(mac_str.split(':')[0], 16)
    return bool(first_octet & 0x02)


def censor_mac(mac_str: str) -> str:
    # keep first 3 octets (OUI/vendor prefix), mask the rest
    parts = mac_str.split(':')
    return ":".join(parts[:3] + ["xx"] * (len(parts) - 3))

# -------------------------------------------------------------------------------#

if __name__ == '__main__':
    with Live(make_table(), console=console, refresh_per_second=4) as live:
        while True:
            data, addr = sock.recvfrom(1024)

            sig_len, rssi, channel, ptype = struct.unpack(META_FMT, data[:META_LEN])
            frame = data[META_LEN:META_LEN + sig_len]

            if len(frame) < 24:
                continue

            fc_raw, duration = struct.unpack("<HH", frame[0:4])
            fc = parse_frame_control(fc_raw)
            addr1 = frame[4:10].hex(':')
            addr2 = frame[10:16].hex(':')
            addr3 = frame[16:22].hex(':')

            randomized = is_randomized(addr2)
            info = parser.get_all(addr2)
            val = ""
            if randomized:
                vendor = "(randomized)"
            elif info:
                vendor = (info.comment or info.manuf or "?")
            else:
                vendor = "?"

            subtype_name = SUBTYPE_NAMES.get(fc['subtype'], f"0x{fc['subtype']:x}")
            type_name = TYPE_NAMES.get(fc['type'], "?")

            s = stations[addr2]
            s["count"] += 1
            s["last_seen"] = time.time()
            s["vendor"] = vendor
            s["rssi"] = rssi
            s["channel"] = channel
            s["subtype"] = f"{type_name}/{subtype_name}"
            s["pwr"] = fc['pwr_mgmt']
            s["bssid"] = addr1 if fc['to_ds'] else addr3

            live.update(make_table())
