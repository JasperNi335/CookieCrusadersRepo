import serial
import time
import os

PORT = "COM5"
BAUD = 115200
OUTFILE = "Test/Data/latest.jpg"

# Tunables
CHUNK = 2048
OPEN_DELAY_S = 0.5

def find_marker(buf: bytearray, marker: bytes, start: int = 0):
    """Find JPEG marker in buf starting at 'start'. Returns index or -1."""
    return buf.find(marker, start)

with serial.Serial(PORT, BAUD, timeout=1, xonxoff=False, rtscts=False, dsrdtr=False) as ser, open(OUTFILE):
    # keep ESP32 out of reset/bootloader
    ser.setDTR(False)
    ser.setRTS(False)
    time.sleep(OPEN_DELAY_S)
    ser.reset_input_buffer()

    print(f"Listening on {PORT} @ {BAUD} — Ctrl+C to stop.")
    buffer = bytearray()
    capturing = False
    f = None

    try:
        while True:
            chunk = ser.read(CHUNK)
            if not chunk:
                continue

            buffer.extend(chunk)

            # If not capturing yet, look for SOI anywhere in buffer
            if not capturing:
                soi = find_marker(buffer, b"\xff\xd8")
                if soi != -1:
                    # Start capture at SOI
                    capturing = True
                    # Open (truncate) output file
                    f = open(OUTFILE, "wb")
                    # Write from SOI onward (not any logs before it)
                    f.write(buffer[soi:])
                    # Keep only what we've written for further EOI search
                    buffer = buffer[soi:]
                else:
                    # No SOI yet—don't let buffer grow unbounded. Keep last 1 KB in case
                    if len(buffer) > 1024:
                        del buffer[:-1024]
                    continue

            # If capturing, look for EOI; it might be in the middle or split
            eoi = find_marker(buffer, b"\xff\xd9")
            if eoi != -1 and capturing and f is not None:
                # Write up to and including EOI to file
                # (we may have already written these bytes once; ensure we only append new ones)
                # Compute how many NEW bytes to append: everything not yet on disk
                # A simpler approach is: just ensure the file ends at EOI.
                # So, truncate buffer after EOI, write buffer to disk freshly.
                # But that would re-write a lot. Instead, append the tail since last write:
                # We'll track how many bytes are already on disk by file position.
                already = f.tell()
                if len(buffer) > already:
                    f.write(buffer[already:eoi + 2])

                # Close and finalize file
                f.flush()
                os.fsync(f.fileno())
                f.close()
                print(f"[Image saved: {OUTFILE} ({eoi + 2} bytes)]")

                # Prepare for next image:
                # Keep anything AFTER the EOI in buffer (it might include logs or next SOI)
                buffer = buffer[eoi + 2:]
                capturing = False
                f = None

                # Optional: if there’s another SOI immediately after, loop will catch it
            else:
                # Still capturing but no EOI yet—append only the NEW bytes to the file
                if capturing and f is not None:
                    already = f.tell()
                    if len(buffer) > already:
                        f.write(buffer[already:])

                # Trim buffer growth for long images (keep last 1 MB to search markers)
                if len(buffer) > 1024 * 1024:
                    # Keep last 1 MB to not miss an EOI spanning boundary
                    start_keep = len(buffer) - 1024 * 1024
                    # We also need to keep file position coherent; since we append increments,
                    # trimming buffer is safe. Just drop older bytes already written.
                    del buffer[:start_keep]

    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        if f and not f.closed:
            f.close()