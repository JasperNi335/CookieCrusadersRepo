# udp_rx.py
import socket, struct
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", 5555))
print("listening on UDP :5555")
while True:
    data, addr = sock.recvfrom(4096)
    if len(data) >= 4 and data[0]==0x5A and data[1]==0xA5:
        typ = data[2]
        if typ == 1:
            seq = int.from_bytes(data[4:6], "little")
            ns  = int.from_bytes(data[6:8], "little")
            sr  = int.from_bytes(data[8:10], "little")
            print(f"PCM seq={seq} ns={ns} sr={sr} bytes={len(data)} from {addr}")
        elif typ == 2:
            ts  = int.from_bytes(data[4:8], "little")
            score = struct.unpack("<f", data[8:12])[0]
            print(f"KWS event ts={ts} score={score} from {addr}")
    else:
        print(len(data), "bytes from", addr)