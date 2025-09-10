# main.py — trigger+playback + smart WAV recorder (RAM or streaming fallback)
import os, urandom, utime, gc
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

# ---------- simple 8-bit/8 kHz WAV playback (for your existing files) ----------
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

# ---------- WAV utils ----------
def _u32le(n): return bytes((n & 0xFF, (n>>8)&0xFF, (n>>16)&0xFF, (n>>24)&0xFF))
def _u16le(n): return bytes((n & 0xFF, (n>>8)&0xFF))

def _write_wav_header(f, rate, bits, channels, data_size):
    byte_rate   = rate * channels * (1 if bits == 8 else 2)
    block_align = channels * (1 if bits == 8 else 2)
    riff_size   = 36 + data_size
    f.write(b"RIFF")
    f.write(_u32le(riff_size))
    f.write(b"WAVE")
    f.write(b"fmt ")
    f.write(_u32le(16))              # PCM fmt chunk size
    f.write(_u16le(1))               # PCM
    f.write(_u16le(channels))
    f.write(_u32le(rate))
    f.write(_u32le(byte_rate))
    f.write(_u16le(block_align))
    f.write(_u16le(bits))
    f.write(b"data")
    f.write(_u32le(data_size))

def _patch_wav_sizes(f, data_size):
    riff_size = 36 + data_size
    f.seek(4)
    f.write(_u32le(riff_size))
    f.seek(40)
    f.write(_u32le(data_size))

# ---------- RAM recorder (best timing, needs contiguous heap) ----------
def _record_wav_ram(path, secs, rate, bits):
    channels = 1
    bps = 1 if bits == 8 else 2
    samples = secs * rate
    data_size = samples * channels * bps

    gc.collect()
    try:
        buf = bytearray(data_size)
    except Exception as e:
        print("REC,RAM,ERR,alloc," + str(e))
        return False

    print("REC,RAM,start,path=" + path + ",bytes=" + str(data_size))
    adc = ADC(Pin(AUDIO_ADC_PIN))
    period_us = 1_000_000 // rate
    t_next = utime.ticks_us()
    late = 0

    if bits == 8:
        for i in range(samples):
            v = adc.read_u16()
            buf[i] = v >> 8
            t_next = utime.ticks_add(t_next, period_us)
            while True:
                now = utime.ticks_us()
                if utime.ticks_diff(t_next, now) <= 0:
                    if utime.ticks_diff(now, t_next) > 5:
                        late += 1
                    break
    else:
        j = 0
        for _ in range(samples):
            v = adc.read_u16()
            s = v - 32768
            if s < -32768: s = -32768
            if s >  32767: s =  32767
            buf[j]   = s & 0xFF
            buf[j+1] = (s >> 8) & 0xFF
            j += 2
            t_next = utime.ticks_add(t_next, period_us)
            while True:
                now = utime.ticks_us()
                if utime.ticks_diff(t_next, now) <= 0:
                    if utime.ticks_diff(now, t_next) > 5:
                        late += 1
                    break

    if late:
        print("REC,RAM,late," + str(late))

    try:
        with open(path, "wb") as f:
            _write_wav_header(f, rate, bits, channels, data_size)
            f.write(buf)
        print("REC,RAM,done,path=" + path + ",bytes=" + str(44 + data_size))
        return True
    except Exception as e:
        print("REC,RAM,ERR,write," + str(e))
        return False

# ---------- Streaming recorder (small chunks; survives low RAM) ----------
def _record_wav_stream(path, secs, rate, bits, chunk_samples=512):
    """
    Stream to flash in chunks (default: 512 samples -> 1024 bytes at 16-bit).
    Timing is best-effort; will report 'late' overruns. If large, lower 'rate'.
    """
    channels = 1
    bps = 1 if bits == 8 else 2
    samples = secs * rate
    chunk_bytes = chunk_samples * bps
    try:
        buf = bytearray(chunk_bytes)
    except Exception as e:
        print("REC,STREAM,ERR,alloc_chunk," + str(e))
        return False

    # Placeholder header; patch sizes at the end
    print("REC,STREAM,start,path=" + path + ",secs=" + str(secs) + ",rate=" + str(rate) + ",bits=" + str(bits))
    try:
        f = open(path, "wb")
    except Exception as e:
        print("REC,STREAM,ERR,open," + str(e))
        return False

    try:
        _write_wav_header(f, rate, bits, channels, 0)
        adc = ADC(Pin(AUDIO_ADC_PIN))
        period_us = 1_000_000 // rate
        t_next = utime.ticks_us()
        late = 0
        written = 0
        filled = 0

        if bits == 8:
            for i in range(samples):
                v = adc.read_u16()
                buf[filled] = v >> 8
                filled += 1
                t_next = utime.ticks_add(t_next, period_us)
                while True:
                    now = utime.ticks_us()
                    if utime.ticks_diff(t_next, now) <= 0:
                        if utime.ticks_diff(now, t_next) > 5:
                            late += 1
                        break
                if filled >= chunk_samples:
                    f.write(buf)
                    written += chunk_samples
                    filled = 0
        else:
            j = 0
            for i in range(samples):
                v = adc.read_u16()
                s = v - 32768
                if s < -32768: s = -32768
                if s >  32767: s =  32767
                buf[j]   = s & 0xFF
                buf[j+1] = (s >> 8) & 0xFF
                j += 2
                filled += 1

                t_next = utime.ticks_add(t_next, period_us)
                while True:
                    now = utime.ticks_us()
                    if utime.ticks_diff(t_next, now) <= 0:
                        if utime.ticks_diff(now, t_next) > 5:
                            late += 1
                        break

                if filled >= chunk_samples:
                    f.write(buf)
                    written += chunk_samples
                    filled = 0
                    j = 0

        # flush tail
        if filled:
            if bits == 8:
                f.write(memoryview(buf)[:filled])
                written += filled
            else:
                f.write(memoryview(buf)[:filled * 2])
                written += filled

        data_size = written * bps
        _patch_wav_sizes(f, data_size)
        print("REC,STREAM,done,path=" + path + ",bytes=" + str(44 + data_size))
        if late:
            print("REC,STREAM,late," + str(late))
        return True

    except Exception as e:
        print("REC,STREAM,ERR," + str(e))
        return False
    finally:
        try:
            f.close()
        except:
            pass

# ---------- Smart recorder ----------
def record_wav(path="/rec_16k_s16.wav", secs=3, rate=16000, bits=16):
    """
    Try RAM capture first (perfect timing). If memory is tight, fall back to streaming.
    """
    print("REC,smart,try_RAM")
    if _record_wav_ram(path, secs, rate, bits):
        return
    print("REC,smart,falling_back_to_STREAM")
    ok = _record_wav_stream(path, secs, rate, bits, chunk_samples=512)
    if not ok:
        print("REC,smart,ERR,stream_failed")

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