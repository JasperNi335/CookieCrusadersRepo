import serial

PORT = "COM5"
BAUD = 115200
OUTFILE = "Test/Data/log.log"
CHUNK = 4096

with serial.Serial(PORT, BAUD, timeout=1, xonxoff=False, rtscts=False, dsrdtr=False) as ser, open(OUTFILE, "ab", buffering=0) as f:
    # keep ESP32 out of reset/bootloader
    ser.setDTR(False)
    ser.setRTS(False)
    print(f"Listening on {PORT} @ {BAUD} — Ctrl+C to stop.")
    
    try:
        while True:
            chunk = ser.read(CHUNK)
            if not chunk: continue
            
            f.write(chunk)
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        if f and not f.closed:
            f.close()