import argparse, sys, struct, time
import serial
import serial.tools.list_ports as lp

MAGIC = 0xA1B2C3D4  # must match device/config.py

def auto_port():
    # Prefer MicroPython Pico VID:PID 2E8A:0005
    for p in lp.comports():
        if (p.vid, p.pid) == (0x2E8A, 0x0005):
            return p.device
    # Fallback: last USB Serial Device / MicroPython
    cands = [p.device for p in lp.comports()
             if "USB Serial Device" in (p.description or "") or
                "MicroPython" in (p.description or "")]
    return cands[-1] if cands else None

def read_exact(ser, n, timeout_s=2.0):
    end = time.time() + timeout_s
    buf = bytearray()
    while len(buf) < n and time.time() < end:
        chunk = ser.read(n - len(buf))
        if chunk:
            buf += chunk
    return bytes(buf) if len(buf) == n else None

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", help="COM port (omit to auto-pick)")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--beep", action="store_true", help="send BEEP to device")
    ap.add_argument("--frames", type=int, default=0, help="read this many frames")
    args = ap.parse_args()

    port = args.port or auto_port()
    if not port:
        print("No Pico serial port found. Is it plugged in (not BOOTSEL) and is Thonny closed?")
        sys.exit(1)

    print(f"Opening {port} @ {args.baud}...")
    with serial.Serial(port, args.baud, timeout=1) as ser:
        time.sleep(0.15)
        if args.beep:
            ser.write(b"BEEP\n")
            ser.flush()
            print("Sent BEEP")

        for i in range(args.frames):
            hdr = read_exact(ser, 6, timeout_s=2.0)
            if not hdr:
                print("Timeout waiting for header")
                continue
            magic, n = struct.unpack("<IH", hdr)
            if magic != MAGIC:
                print(f"Bad magic 0x{magic:08X}; skipping")
                ser.reset_input_buffer()
                continue
            payload = read_exact(ser, n, timeout_s=2.0)
            if not payload:
                print("Timeout waiting for payload")
                continue
            print(f"Frame {i+1}: {n} bytes")

if __name__ == "__main__":
    main()