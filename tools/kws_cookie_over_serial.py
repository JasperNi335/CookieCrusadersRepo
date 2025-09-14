import os, sys, time, json, struct
import serial
from serial.tools import list_ports
from vosk import Model, KaldiRecognizer

# ---------- CONFIG ----------
DEFAULT_COM   = "COM5"
BAUD          = 115200
COOLDOWN_S    = 1.5
KEYWORD       = "cookie"
GRAMMAR       = json.dumps([KEYWORD])  # restrict vocab

# Model search order
CANDIDATE_MODELS = [
    os.environ.get("VOSK_MODEL", ""),
    os.path.join(os.path.dirname(os.path.dirname(__file__)),
                 "models", "vosk", "vosk-model-small-en-us-0.15"),
    r"C:\models\vosk-model-small-en-us-0.15",
]

MAGIC = 0xAA55AA55

# ---------- small utils ----------
def pick_model_path():
    tried = []
    for p in CANDIDATE_MODELS:
        if not p: 
            continue
        tried.append(p)
        if os.path.isdir(p) and os.path.isdir(os.path.join(p, "conf")):
            return p
    print("\nERROR: Could not find a valid Vosk model folder.")
    print("Tried:")
    for t in tried: print("  -", t)
    print("\nPlace model at:")
    print(r"  CookieCrusadersRepo\models\vosk\vosk-model-small-en-us-0.15")
    print("or set VOSK_MODEL to an absolute path.")
    sys.exit(1)

def open_serial(com):
    try:
        ser = serial.Serial(com, BAUD, timeout=1)
        ser.reset_input_buffer(); ser.reset_output_buffer()
        return ser
    except serial.SerialException as e:
        print(f"\nERROR: Could not open {com}: {e}")
        print("Available COM ports:")
        for p in list_ports.comports():
            print(" ", p.device, "-", p.description)
        sys.exit(1)

def read_exact(ser, n):
    data = b""
    while len(data) < n:
        chunk = ser.read(n - len(data))
        if not chunk: return None
        data += chunk
    return data

# Fast RMS in pure Python (no numpy dependency)
def rms_int16_le(buf):
    # buf length is even
    it = struct.iter_unpack("<h", buf)
    s2 = 0
    count = 0
    for (v,) in it:
        s2 += v*v
        count += 1
    if count == 0: 
        return 0.0
    return (s2 / count) ** 0.5

def dbfs_from_rms(rms):
    if rms <= 1e-9: 
        return -120.0
    # int16 full-scale is 32768
    import math
    return 20.0 * math.log10(rms / 32768.0)

def vu_bar(dbfs, width=20):
    # Map -60..0 dBFS into 0..width
    clamped = max(-60.0, min(0.0, dbfs))
    n = int((clamped + 60.0) / 60.0 * width)
    return "[" + ("#"*n).ljust(width) + "]"

def main():
    # --- model ---
    model_path = pick_model_path()
    print("Using model:", model_path)
    model = Model(model_path)
    rec = KaldiRecognizer(model, 16000, GRAMMAR)

    # --- serial ---
    com = os.environ.get("PICO_COM", DEFAULT_COM)
    if len(sys.argv) > 1 and sys.argv[1].upper().startswith("COM"):
        com = sys.argv[1]
    ser = open_serial(com)

    print(f"Listening on {com} for framed PCM; keyword: {KEYWORD}")
    last_fire = 0.0
    sync = b""

    # dBFS display rate
    last_vu = 0.0
    VU_INTERVAL = 0.5  # seconds

    try:
        while True:
            # sync to MAGIC
            while True:
                b1 = ser.read(1)
                if not b1: 
                    continue
                sync += b1
                if len(sync) >= 4:
                    word = struct.unpack_from("<I", sync[-4:])[0]
                    if word == MAGIC:
                        break
                    sync = sync[-3:]

            # read length
            hdr = read_exact(ser, 2)
            if hdr is None: 
                continue
            (length,) = struct.unpack("<H", hdr)
            if length == 0 or length > 8192:
                continue

            payload = read_exact(ser, length)
            if payload is None or len(payload) < 10:
                continue

            seq, sr, n = struct.unpack_from("<IIH", payload, 0)
            pcm = payload[10:]
            if sr != 16000 or len(pcm) != n*2:
                continue

            # --- dBFS meter (prints ~2x/sec) ---
            now = time.time()
            if now - last_vu >= VU_INTERVAL:
                rms = rms_int16_le(pcm)
                db = dbfs_from_rms(rms)
                print(f"dBFS={db:6.1f} {vu_bar(db)}")
                last_vu = now

            # --- KWS ---
            trig = False
            if rec.AcceptWaveform(pcm):
                res = json.loads(rec.Result())
                if res.get("text") == KEYWORD:
                    trig = True
                    print("[KWS] cookie (final)")
            else:
                pres = json.loads(rec.PartialResult())
                if pres.get("partial") == KEYWORD:
                    trig = True
                    print("[KWS] cookie (partial)")

            if trig and (time.time() - last_fire > COOLDOWN_S):
                ser.write(b"BEEP\n")
                last_fire = time.time()

    except KeyboardInterrupt:
        pass
    finally:
        ser.close()

if __name__ == "__main__":
    main()