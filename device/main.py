from typing import Optional, Any, BinaryIO, cast
import sys
import struct
import utime                    # MicroPython time module (has sleep_ms, ticks_*)
from machine import Pin, PWM

from config import (
    FRAMING_MAGIC, ENABLE_USB_STREAM,
    LED_PIN, SPEAKER_PIN,
    BEEP_TONE_HZ, BEEP_MS, BEEP_GAP_MS,
    ENV_CENTER, ENV_THRESH, ENV_HANG_MS,
)
import audio


# ---------- PWM tone helpers ----------
def pwm_tone_start(pin_num: int, freq_hz: int) -> PWM:
    pwm = PWM(Pin(pin_num))
    pwm.freq(int(freq_hz))
    pwm.duty_u16(32768)  # ~50%
    return pwm


def pwm_tone_stop(pwm: PWM) -> None:
    pwm.deinit()


def play_melody(pin_num: int) -> None:
    seq = (BEEP_TONE_HZ, int(BEEP_TONE_HZ * 1.5), BEEP_TONE_HZ)
    for idx, f in enumerate(seq):
        p = pwm_tone_start(pin_num, f)
        utime.sleep_ms(BEEP_MS)
        pwm_tone_stop(p)
        if idx != len(seq) - 1:
            utime.sleep_ms(BEEP_GAP_MS)


# ---------- USB binary write (Pylance-safe) ----------
def _stdout_binary() -> Optional[BinaryIO]:
    # getattr(...) is typed as object|None; cast it to BinaryIO|None for Pylance.
    buf: Any = getattr(sys.stdout, "buffer", None)
    return cast(Optional[BinaryIO], buf)

def usb_write_all(b: bytes) -> None:
    bio = _stdout_binary()
    if bio is not None:
        bio.write(b)        # bytes -> binary stream
        bio.flush()
    else:
        # Fallback for text-only stdout
        sys.stdout.write(b.decode("latin-1"))
        sys.stdout.flush()


# ---------- Non-blocking line read (ASCII) ----------
def read_cmd_nonblock() -> Optional[str]:
    try:
        import uselect as select  # MicroPython
    except ImportError:
        try:
            import select         # CPython fallback (rarely used here)
        except ImportError:
            return None

    res: Any = select.select([sys.stdin], [], [], 0)
    # Some stubs mark select() as possibly returning None; guard to appease Pyright.
    if not res:
        return None
    r, _, _ = res  # type: ignore[assignment]
    if r:
        try:
            line = sys.stdin.readline()
            if not line:
                return None
            return line.strip()
        except Exception:
            return None
    return None


# ---------- Simple envelope detector ----------
def envelope_u16_le(payload: bytes) -> int:
    """
    payload: little-endian u16 PCM (0..65535).
    Return mean absolute deviation around ENV_CENTER.
    """
    n = len(payload) // 2
    if n == 0:
        return 0
    acc = 0
    mv = memoryview(payload)
    for i in range(0, len(mv), 2):
        s = mv[i] | (mv[i + 1] << 8)
        acc += abs(s - ENV_CENTER)
    return acc // n


def main() -> None:
    led = Pin(LED_PIN, Pin.OUT)
    led.value(0)

    audio.start()

    next_ok_ms = 0  # cooldown end time (ms)
    hb_next = utime.ticks_add(utime.ticks_ms(), 500)

    while True:
        # 1) Host command?
        cmd = read_cmd_nonblock()
        if cmd == "BEEP":
            now = utime.ticks_ms()
            if utime.ticks_diff(now, next_ok_ms) >= 0:
                play_melody(SPEAKER_PIN)
                next_ok_ms = utime.ticks_add(now, ENV_HANG_MS)

        # 2) Audio packet: stream (optional) + detect
        pkt = audio.try_acquire_packet()
        if pkt is not None:
            if ENABLE_USB_STREAM:
                hdr = struct.pack("<IH", FRAMING_MAGIC, len(pkt))
                usb_write_all(hdr)
                usb_write_all(pkt)

            env = envelope_u16_le(pkt)
            if env >= ENV_THRESH:
                now = utime.ticks_ms()
                if utime.ticks_diff(now, next_ok_ms) >= 0:
                    play_melody(SPEAKER_PIN)
                    next_ok_ms = utime.ticks_add(now, ENV_HANG_MS)

        # 3) Heartbeat LED (2 Hz)
        now = utime.ticks_ms()
        if utime.ticks_diff(now, hb_next) >= 0:
            led.toggle()
            hb_next = utime.ticks_add(now, 500)

        utime.sleep_ms(1)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass