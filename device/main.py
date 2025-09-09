# main.py
import sys, time, struct, random
from machine import Pin, PWM
import audio
import config

# ----- LED -----
led = Pin(config.LED_PIN_NAME, Pin.OUT)

# ----- Speaker via PWM -----
spk_pwm = PWM(Pin(config.SPEAKER_PIN))
spk_pwm.duty_u16(0)

def pwm_tone_start(freq_hz: int):
    if freq_hz <= 0:
        spk_pwm.duty_u16(0)
        return
    spk_pwm.freq(int(freq_hz))
    spk_pwm.duty_u16(32768)  # ~50%

def pwm_tone_stop():
    spk_pwm.duty_u16(0)

def play_melody():
    # 3 tones, choose one of 4 patterns
    patterns = [
        (1000, 2000, 3000),
        (1500, 1000, 2500),
        (2000, 1200, 3000),
        (3000, 2000, 1000),
    ]
    pat = random.choice(patterns)
    for i, f in enumerate(pat):
        pwm_tone_start(f)
        time.sleep_ms(config.BEEP_TONE_MS)
        pwm_tone_stop()
        if i != len(pat) - 1:
            time.sleep_ms(config.BEEP_GAP_MS)

# ----- USB data channel (separate from REPL if boot.py enabled it) -----
usb_out = None
usb_in  = None
try:
    import usb_cdc
    usb_out = usb_cdc.data  # binary stream out/in
    usb_in  = usb_cdc.data
except Exception:
    # Fallback: use sys.stdout/sys.stdin (will share with REPL; not ideal)
    usb_out = sys.stdout.buffer
    usb_in  = sys.stdin.buffer

def usb_write_all(b: bytes):
    # write in chunks to avoid large-buffer hiccups
    mv = memoryview(b)
    total = 0
    while total < len(b):
        n = usb_out.write(mv[total:total+1024])
        if n is None:
            n = 0
        total += n

def read_line_nonblocking(maxlen=32):
    """Read ASCII line (without blocking) from usb_in; returns str or None."""
    if not hasattr(usb_in, "in_waiting"):
        # simple fallback: non-portable; try a small timeout
        return None
    if usb_in.in_waiting <= 0:
        return None
    # Read available and look for '\n'
    data = bytearray()
    deadline = time.ticks_add(time.ticks_ms(), 2)
    while time.ticks_diff(deadline, time.ticks_ms()) > 0 and len(data) < maxlen:
        if usb_in.in_waiting > 0:
            ch = usb_in.read(1)
            if not ch:
                break
            if ch == b'\r':
                continue
            if ch == b'\n':
                break
            data += ch
        else:
            time.sleep_ms(1)
    if not data:
        return None
    try:
        return data.decode('ascii', 'ignore')
    except:
        return None

def main():
    # Start audio sampler
    audio.start()

    next_allowed_ms = 0
    hb_next = time.ticks_add(time.ticks_ms(), 500)

    while True:
        # 1) STREAM AUDIO PACKETS (binary) with header [u32 magic][u16 len]
        pay = audio.try_acquire_packet()
        if pay:
            hdr = struct.pack("<IH", config.FRAMING_MAGIC, len(pay))
            usb_write_all(hdr)
            usb_write_all(pay)

        # 2) COMMANDS: ASCII "BEEP\n" over the data USB CDC
        line = read_line_nonblocking()
        if line:
            if line.strip().upper() == "BEEP":
                now = time.ticks_ms()
                if time.ticks_diff(now, next_allowed_ms) >= 0:
                    play_melody()
                    next_allowed_ms = time.ticks_add(now, config.COOLDOWN_MS)

        # 3) LED heartbeat (slow blink)
        if time.ticks_diff(time.ticks_ms(), hb_next) >= 0:
            led.toggle()
            hb_next = time.ticks_add(time.ticks_ms(), 500)

        # 4) cooperative yield
        time.sleep_ms(1)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
    finally:
        pwm_tone_stop()
        audio.stop()