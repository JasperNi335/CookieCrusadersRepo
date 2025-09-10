# main.py — voice trigger -> play random WAV
import os, utime
from machine import Pin, PWM
import urandom

from config import (
    LED_PIN, SPEAKER_PIN,
    ENV_THRESH, ENV_HANG_MS,
    PLOT_DEBUG, SOUNDS_DIR, SOUND_FILES,
    MAX_WAV_MS, WAV_CHUNK_BYTES
)
import audio

# ----------------- hardware: LED + hard-mute speaker when idle -----------------
_led = Pin(LED_PIN, Pin.OUT)
def led(on): _led.value(1 if on else 0)

# Drive speaker line LOW when idle so the amp input is not floating (kills hiss).
_idle_spk = Pin(SPEAKER_PIN, Pin.OUT)
_idle_spk.value(0)

# ----------------- small utils -----------------
def rand_choice(seq):
    if not seq:
        return None
    idx = urandom.getrandbits(30) % len(seq)
    return seq[idx]

def compute_envelope_u16le(frame_bytes):
    """
    Auto-centered average absolute deviation:
    1) compute mean of the frame
    2) average |sample - mean|
    """
    n = len(frame_bytes) // 2
    if n == 0:
        return 0
    s = 0
    # mean
    for i in range(0, len(frame_bytes), 2):
        v = frame_bytes[i] | (frame_bytes[i+1] << 8)
        s += v
    mean = s // n
    # avg abs deviation
    acc = 0
    for i in range(0, len(frame_bytes), 2):
        v = frame_bytes[i] | (frame_bytes[i+1] << 8)
        acc += (v - mean) if v >= mean else (mean - v)
    return acc // n

# ----------------- WAV playback -----------------
def _read_exact(f, n):
    b = f.read(n)
    if b is None or len(b) < n:
        return None
    return b

def _parse_wav_header(f):
    # minimal RIFF/WAVE parser for PCM mono 8/16-bit
    # Returns (sample_rate, bits_per_sample, channels, data_start, data_size)
    if _read_exact(f, 4) != b"RIFF":
        raise ValueError("Not RIFF")
    _ = _read_exact(f, 4)  # file size (unused)
    if _read_exact(f, 4) != b"WAVE":
        raise ValueError("Not WAVE")

    fmt_sr = None
    fmt_bps = None
    fmt_ch  = None
    data_size = None
    data_pos  = None

    while True:
        chunk_id = f.read(4)
        if not chunk_id:
            break
        size_b = _read_exact(f, 4)
        if size_b is None:
            break
        size = (size_b[0] | (size_b[1] << 8) | (size_b[2] << 16) | (size_b[3] << 24))

        if chunk_id == b"fmt ":
            fmt = _read_exact(f, size)
            if fmt is None or len(fmt) < 16:
                raise ValueError("fmt too short")
            audio_fmt = fmt[0] | (fmt[1] << 8)      # 1 = PCM
            fmt_ch = fmt[2] | (fmt[3] << 8)
            fmt_sr = (fmt[4] | (fmt[5] << 8) | (fmt[6] << 16) | (fmt[7] << 24))
            # skip byte rate (4) + block align (2)
            fmt_bps = fmt[14] | (fmt[15] << 8)
            if audio_fmt != 1:
                raise ValueError("Only PCM supported")
        elif chunk_id == b"data":
            data_pos = f.tell()
            data_size = size
            break
        else:
            f.seek(size, 1)

    if fmt_sr is None or fmt_bps is None or fmt_ch is None:
        raise ValueError("Missing fmt")
    if fmt_ch != 1:
        raise ValueError("Only mono supported")
    if fmt_bps not in (8, 16):
        raise ValueError("Only 8/16-bit PCM supported")
    if data_pos is None or data_size is None:
        raise ValueError("Missing data chunk")

    return fmt_sr, fmt_bps, fmt_ch, data_pos, data_size

def play_wav_file(path, pin=SPEAKER_PIN):
    """
    Stream PCM to PWM at the WAV's sample rate.
    Supports mono 8-bit unsigned and 16-bit signed little-endian PCM.
    """
    pwm = PWM(Pin(pin))
    pwm.freq(100_000)  # ultrasonic carrier to reduce audible whine
    pwm.duty_u16(0)

    try:
        with open(path, "rb") as f:
            sr, bps, ch, data_pos, data_size = _parse_wav_header(f)
            f.seek(data_pos)
            period_us = int(1_000_000 // sr) if sr > 0 else 125

            deadline = utime.ticks_add(utime.ticks_ms(), MAX_WAV_MS)
            bytes_left = data_size

            while bytes_left > 0 and utime.ticks_diff(deadline, utime.ticks_ms()) > 0:
                to_read = WAV_CHUNK_BYTES
                if bps == 16 and (to_read & 1):
                    to_read += 1  # align to 2 bytes
                chunk = f.read(to_read)
                if not chunk:
                    break
                bytes_in_chunk = len(chunk)
                bytes_left -= bytes_in_chunk

                if bps == 8:
                    for i in range(bytes_in_chunk):
                        s = chunk[i]  # 0..255 unsigned
                        pwm.duty_u16(s << 8)  # expand to 16-bit
                        t_next = utime.ticks_add(utime.ticks_us(), period_us)
                        while utime.ticks_diff(t_next, utime.ticks_us()) > 0:
                            pass
                else:  # 16-bit signed little-endian
                    i = 0
                    while i + 1 < bytes_in_chunk:
                        s = chunk[i] | (chunk[i+1] << 8)
                        if s & 0x8000:
                            s -= 0x10000  # sign extend
                        # map -32768..32767 -> 0..65535
                        y = s + 32768
                        if y < 0: y = 0
                        elif y > 65535: y = 65535
                        pwm.duty_u16(y)
                        i += 2
                        t_next = utime.ticks_add(utime.ticks_us(), period_us)
                        while utime.ticks_diff(t_next, utime.ticks_us()) > 0:
                            pass
    finally:
        pwm.deinit()
        # ensure amp input is driven LOW when idle
        Pin(pin, Pin.OUT).value(0)

# ----------------- sounds list -----------------
def _discover_sounds():
    files = []
    try:
        for name in os.listdir(SOUNDS_DIR):
            nm = name.lower()
            if nm.endswith(".wav"):
                files.append(SOUNDS_DIR + "/" + name)
    except Exception:
        pass
    return files

def _pick_sound():
    files = SOUND_FILES[:] if SOUND_FILES else _discover_sounds()
    return rand_choice(files)

# ----------------- app loop -----------------
def run():
    audio.start()
    last_fire_ms = 0
    try:
        while True:
            frame = audio.try_acquire_packet()
            if frame is None:
                utime.sleep_ms(5)
                continue

            env = compute_envelope_u16le(frame)
            if PLOT_DEBUG:
                print("env," + str(env))

            now = utime.ticks_ms()
            if env >= ENV_THRESH and utime.ticks_diff(now, last_fire_ms) >= ENV_HANG_MS:
                snd = _pick_sound()
                if snd:
                    led(True)
                    play_wav_file(snd, SPEAKER_PIN)
                    led(False)
                last_fire_ms = now
    finally:
        audio.stop()
        # keep speaker muted on exit
        Pin(SPEAKER_PIN, Pin.OUT).value(0)

# ------------- handy tests from REPL -------------
def beep_test(pin=SPEAKER_PIN, ms=500, freq=1000, duty=20000):
    p = PWM(Pin(pin))
    p.freq(freq)
    p.duty_u16(duty)
    utime.sleep_ms(ms)
    p.deinit()
    Pin(pin, Pin.OUT).value(0)

# Auto-run on reset; comment out if you prefer manual start.
if __name__ == "__main__":
    run()