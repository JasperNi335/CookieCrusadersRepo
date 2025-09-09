import argparse, struct, sys, time
import serial, serial.tools.list_ports as lp

def auto_port():
    # Try to pick the second MicroPython CDC if present
    cands = [p.device for p in lp.comports() if "USB Serial Device" in (p.description or "") or "MicroPython" in (p.description or "")]
    if len(cands) >= 2:
        return cands[-1]  # heuristic: last one is often 'data'
    return None

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", "-p", help="COM port for data CDC (e.g., COM7)")
    ap.add_argument("--baud", "-b", type=int, default=115200)
    ap.add_argument("--beep", action="store_true", help="Send BEEP then exit")
    ap.add_argument("--frames", type=int, default=1, help="How many frames to read (0 = infinite)")
    ap.add_argument("--magic", type=lambda x:int(x,0), default=0xA1B2C3D4, help="Framing magic (0x...)")
    args = ap.parse_args()

    port = args.port or auto_port()
    if not port:
        print("Could not auto-pick a port. Use --port COMx (see tools/list_ports.py).")
        sys.exit(2)

    print(f"Opening {port} @ {args.baud}...")
    ser = serial.Serial(port, args.baud, timeout=2)

    # Optional command
    if args.beep:
        ser.write(b"BEEP\n")
        ser.flush()
        print("Sent BEEP")

    # Read frames
    seen = 0
    while args.frames == 0 or seen < args.frames:
        hdr = ser.read(6)
        if len(hdr) < 6:
            print("Timeout waiting for header")
            continue
        magic, length = struct.unpack("<IH", hdr)
        if magic != args.magic:
            print(f"Bad magic: got 0x{magic:08X}, expected 0x{args.magic:08X}")
            # try to resync by skipping one byte
            continue
        payload = ser.read(length)
        if len(payload) != length:
            print("Short read on payload")
            continue
        seen += 1
        print(f"Frame {seen}: {length} bytes")

    ser.close()

if __name__ == "__main__":
    main()