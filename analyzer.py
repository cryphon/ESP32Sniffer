import socket
import struct


# Matches meta_hdr_t: siglen (u16), rssi (i8), channel (u8), type (u8)
META_FMT = "<H b B B"
META_LEN = struct.calcsize(META_FMT)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", 5005))

# Mirror frame_control bitfield, but unpacked from raw u16 
def parse_frame_control(fc_raw: int):
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


if __name__ == '__main__':
    print("[INFO]: starting analyzer...")
    while True:
        print("[INFO]: Listening...")
        data, addr = sock.recvfrom(1024)

        sig_len, rssi, channel, ptype = struct.unpack(META_FMT, data[:META_LEN])
        frame = data[META_LEN:META_LEN + sig_len]

        if len(frame) < 24:
            continue # too short for a 30addr header, skip

        fc_raw, duration = struct.unpack("<HH", frame[0:4])
        fc = parse_frame_control(fc_raw)
        addr1 = frame[4:10].hex(':')
        addr2 = frame[10:16].hex(':')
        addr3 = frame[16:22].hex(':')
        seq_control = struct.unpack("<H", frame[22:24])[0]
        print(f"rssi={rssi} ch={channel} type={fc['type']} subtype={fc['subtype']} "
               f"toDS={fc['to_ds']} fromDS={fc['from_ds']} pwr={fc['pwr_mgmt']} "
               f"addr1={addr1} addr2={addr2} addr3={addr3}")



