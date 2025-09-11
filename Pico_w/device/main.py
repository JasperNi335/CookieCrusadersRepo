import os, urandom, utime
from machine import Pin, PWM, ADC
import audio
from config import (
    LED_PIN, SPEAKER_PIN,
    ENV_THRESH, ENV_HANG_MS,
    PLOT_DEBUG, SOUNDS_DIR,
    WAV_CHUNK_BYTES, MAX_WAV_MS,
    AUDIO_ADC_PIN,
)

# ---------- hardware ----------
_led = Pin(LED_PIN, Pin.OUT)
def led(on): _led.value(1 if on else 0)
Pin(SPEAKER_PIN, Pin.OUT).value(0)  # hard-mute speaker when idle

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
    try:
        for n in os.listdir(SOUNDS_DIR):
            if n.lower().endswith(".wav"):
                files.append(SOUNDS_DIR + "/" + n)
    except:
        pass
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
    print("PLAY,open," + path)
    pwm = PWM(Pin(SPEAKER_PIN))
    pwm.freq(80_000)        # friend’s setting for cleaner audio
    pwm.duty_u16(0)

    deadline = utime.ticks_add(utime.ticks_ms(), MAX_WAV_MS)
    try:
        with open(path, "rb") as f:
            _ = f.read(44)  # skip standard PCM header
            while utime.ticks_diff(deadline, utime.ticks_ms()) > 0:
                buf = f.read(WAV_CHUNK_BYTES)
                if not buf:
                    break
                for b in buf:
                    pwm.duty_u16((b << 8) | b)
                    utime.sleep_us(125)   # 8000 Hz
        print("PLAY,done")
    except Exception as e:
        print("PLAY,ERR," + str(e))
    finally:
        pwm.deinit()
        Pin(SPEAKER_PIN, Pin.OUT).value(0)

# ---------- recorder: 16 kHz / 16-bit mono WAV ----------
def _u32le(n): return bytes((n & 0xFF, (n>>8)&0xFF, (n>>16)&0xFF, (n>>24)&0xFF))
def _u16le(n): return bytes((n & 0xFF, (n>>8)&0xFF))

def record_wav(path="/rec_16k_s16.wav", secs=3, rate=16000, bits=16):
    """
    Capture 'secs' seconds from ADC on AUDIO_ADC_PIN into a mono WAV.
    bits: 8 (unsigned) or 16 (signed little-endian)
    """
    if bits not in (8, 16):
        print("REC,ERR,bits_must_be_8_or_16")
        return

    channels = 1
    samples = secs * rate
    # bytes per sample (per channel)
    bps = 1 if bits == 8 else 2
    data_size = samples * channels * bps
    print("REC,start,path=" + path + ",secs=" + str(secs) + ",rate=" + str(rate) +
          ",bits=" + str(bits) + ",bytes=" + str(data_size))

    # Pre-allocate buffer in RAM (fast & uniform timing)
    try:
        buf = bytearray(data_size)
    except Exception as e:
        print("REC,ERR,alloc," + str(e))
        return

    adc = ADC(Pin(AUDIO_ADC_PIN))
    period_us = 1_000_000 // rate
    t_next = utime.ticks_us()

    # Sample loop
    if bits == 8:
        for i in range(samples):
            v = adc.read_u16()      # 0..65535
            buf[i] = v >> 8         # map to 0..255 unsigned
            t_next = utime.ticks_add(t_next, period_us)
            while utime.ticks_diff(t_next, utime.ticks_us()) > 0:
                pass
    else:
        j = 0
        for _ in range(samples):
            v = adc.read_u16()      # 0..65535
            s = v - 32768           # rough map to int16 range
            if s < -32768: s = -32768
            if s >  32767: s =  32767
            buf[j]   = s & 0xFF
            buf[j+1] = (s >> 8) & 0xFF
            j += 2
            t_next = utime.ticks_add(t_next, period_us)
            while utime.ticks_diff(t_next, utime.ticks_us()) > 0:
                pass

    # Build and write WAV header + data
    byte_rate   = rate * channels * bps
    block_align = channels * bps
    riff_size   = 36 + data_size

    try:
        with open(path, "wb") as f:
            f.write(b"RIFF")
            f.write(_u32le(riff_size))
            f.write(b"WAVE")
            f.write(b"fmt ")
            f.write(_u32le(16))              # PCM fmt chunk size
            f.write(_u16le(1))               # PCM
            f.write(_u16le(channels))        # mono
            f.write(_u32le(rate))
            f.write(_u32le(byte_rate))
            f.write(_u16le(block_align))
            f.write(_u16le(bits))
            f.write(b"data")
            f.write(_u32le(data_size))
            f.write(buf)
        print("REC,done,path=" + path + ",bytes=" + str(44 + data_size))
    except Exception as e:
        print("REC,ERR,write," + str(e))

# ---------- trigger app loop ----------
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