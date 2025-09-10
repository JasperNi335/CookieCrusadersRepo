# main.py — mic trigger + simple 8-bit/8 kHz WAV playback (friend-fused)
import os, urandom, utime
from machine import Pin, PWM
import audio
from config import (
    LED_PIN, SPEAKER_PIN,
    ENV_THRESH, ENV_HANG_MS,
    PLOT_DEBUG, SOUNDS_DIR,
    WAV_CHUNK_BYTES, MAX_WAV_MS,
)

# ---------- hardware ----------
_led = Pin(LED_PIN, Pin.OUT)
def led(on): _led.value(1 if on else 0)
# keep speaker line low when idle (kills hiss)
Pin(SPEAKER_PIN, Pin.OUT).value(0)

# ---------- envelope ----------
def env_u16le(frame):
    """Auto-centered avg abs deviation."""
    n = len(frame) // 2
    if n == 0: return 0
    s = 0
    for i in range(0, len(frame), 2):
        v = frame[i] | (frame[i+1] << 8)
        s += v
    mean = s // n
    acc = 0
    for i in range(0, len(frame), 2):
        v = frame[i] | (frame[i+1] << 8)
        acc += (v - mean) if v >= mean else (mean - v)
    return acc // n

# ---------- wav discovery ----------
def list_wavs():
    files = []
    # prefer /sounds
    try:
        for n in os.listdir(SOUNDS_DIR):
            if n.lower().endswith(".wav"):
                files.append(SOUNDS_DIR + "/" + n)
    except:
        pass
    # fall back to root
    try:
        for n in os.listdir("/"):
            if n.lower().endswith(".wav"):
                files.append("/" + n)
    except:
        pass
    return files

def pick(files):
    if not files: return None
    idx = urandom.getrandbits(30) % len(files)
    return files[idx]

# ---------- simple 8-bit/8 kHz WAV playback ----------
def play_wav_u8_8k(path):
    """
    Very simple player: skip 44-byte header, then stream bytes as 8-bit PCM
    at ~8 kHz using PWM duty. Expects mono u8 @ 8000 Hz.
    """
    print("PLAY,open," + path)
    pwm = PWM(Pin(SPEAKER_PIN))
    pwm.freq(80_000)        # friend’s setting for cleaner audio
    pwm.duty_u16(0)

    deadline = utime.ticks_add(utime.ticks_ms(), MAX_WAV_MS)
    try:
        with open(path, "rb") as f:
            # skip standard 44-byte PCM header (matches your files)
            _ = f.read(44)
            while utime.ticks_diff(deadline, utime.ticks_ms()) > 0:
                buf = f.read(WAV_CHUNK_BYTES)
                if not buf:
                    break
                for b in buf:
                    # map 0..255 -> 0..65535
                    pwm.duty_u16((b << 8) | b)
                    utime.sleep_us(125)   # 8000 Hz sample period
        print("PLAY,done")
    except Exception as e:
        print("PLAY,ERR," + str(e))
    finally:
        pwm.deinit()
        Pin(SPEAKER_PIN, Pin.OUT).value(0)  # hard mute when idle

# ---------- app loop ----------
def run():
    audio.start()
    files = list_wavs()
    print("SOUNDS,count," + str(len(files)))
    for fn in files:
        print("SOUND," + fn)

    last_ms = 0
    try:
        while True:
            frame = audio.try_acquire_packet()
            if frame is None:
                utime.sleep_ms(5)
                continue

            e = env_u16le(frame)
            if PLOT_DEBUG:
                print("env," + str(e))

            now = utime.ticks_ms()
            if e >= ENV_THRESH and utime.ticks_diff(now, last_ms) >= ENV_HANG_MS:
                snd = pick(files)
                if snd:
                    print("TRIG," + str(e) + ",PLAY," + snd)
                    led(True)
                    play_wav_u8_8k(snd)
                    led(False)
                else:
                    print("TRIG," + str(e) + ",NOSOUNDS")
                last_ms = now
    finally:
        audio.stop()
        Pin(SPEAKER_PIN, Pin.OUT).value(0)

# Quick pin sanity
def beep_test(ms=600, freq=1000, duty=22000, pin=SPEAKER_PIN):
    print("BEEP," + str(pin))
    p = PWM(Pin(pin)); p.freq(freq); p.duty_u16(duty)
    utime.sleep_ms(ms); p.deinit()
    Pin(pin, Pin.OUT).value(0)

if __name__ == "__main__":
    run()