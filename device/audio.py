from typing import List, Optional, cast
import utime                      # MicroPython time module (has ticks_*)
from machine import ADC, Pin
import _thread

from config import (
    AUDIO_ADC_PIN, SAMPLE_RATE_HZ, PACKET_SAMPLES, QUEUE_MAX_PACKETS
)

_adc: Optional[ADC] = None
_running: bool = False
_lock = _thread.allocate_lock()
_queue: List[bytes] = []  # FIFO of PCM frames (u16 LE)
_period_us: int = int(1_000_000 // SAMPLE_RATE_HZ)


def _sampler() -> None:
    """
    Sample at ~SAMPLE_RATE_HZ into PACKET_SAMPLES-sized frames of u16 LE.
    Push bytes objects into the _queue (bounded by QUEUE_MAX_PACKETS).
    """
    # Pylance: _adc is Optional, but start() sets it before we start the thread.
    adc = cast(ADC, _adc)

    buf = bytearray(PACKET_SAMPLES * 2)
    i = 0
    t_next = utime.ticks_us()

    while _running:
        # Defensive: if something stopped ADC mid-flight, bail.
        if adc is None:
            return

        v = adc.read_u16()  # 0..65535
        j = i * 2
        buf[j] = v & 0xFF
        buf[j + 1] = (v >> 8) & 0xFF
        i += 1

        # Next sample time (fixed-step)
        t_next = utime.ticks_add(t_next, _period_us)

        # When frame full, enqueue a copy
        if i >= PACKET_SAMPLES:
            frame = bytes(buf)
            with _lock:
                if len(_queue) >= QUEUE_MAX_PACKETS:
                    _queue.pop(0)
                _queue.append(frame)
            i = 0

        # Pace to target sample rate
        while utime.ticks_diff(t_next, utime.ticks_us()) > 0:
            pass


def start() -> None:
    """Init ADC and start the sampler thread."""
    global _adc, _running
    if _running:
        return
    _adc = ADC(Pin(AUDIO_ADC_PIN))
    _running = True
    _thread.start_new_thread(_sampler, ())


def stop() -> None:
    """Stop the sampler thread."""
    global _running
    _running = False


def try_acquire_packet() -> Optional[bytes]:
    """Pop the oldest frame if available (or None)."""
    with _lock:
        if _queue:
            return _queue.pop(0)
    return None