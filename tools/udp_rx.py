import socket, struct, wave

HOST = "0.0.0.0"
PORT = 33333
OUT  = "capture.wav"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((HOST, PORT))
sock.settimeout(5)

wf = None
expected_seq = None
print(f"Listening on UDP {HOST}:{PORT}")

try:
    while True:
        data, addr = sock.recvfrom(65536)
        if len(data) < 10:  # seq(4)+sr(4)+n(2)
            continue
        seq, sr, n = struct.unpack_from("<IIH", data, 0)
        pcm = data[10:]
        if len(pcm) != n*2:
            continue

        if wf is None:
            wf = wave.open(OUT, "wb")
            wf.setnchannels(1)
            wf.setsampwidth(2)
            wf.setframerate(sr)
            print(f"Started WAV {OUT} at {sr} Hz from {addr}")

        if expected_seq is not None and seq != expected_seq:
            print(f"Packet gap/reorder: got {seq}, expected {expected_seq}")
        expected_seq = seq + 1

        wf.writeframesraw(pcm)

except KeyboardInterrupt:
    pass
finally:
    if wf: wf.close()
    sock.close()
    print("Done, wrote", OUT)