# audio.py — MicroPython-safe sampler (no typing)
import utime
from machine import ADC, Pin
import _thread
from config import AUDIO_ADC_PIN, SAMPLE_RATE_HZ, PACKET_SAMPLES, QUEUE_MAX_PACKETS

# Public API (helps readers and some tools)
__all__ = ["start", "stop", "try_acquire_packet"]

_adc = None
_running = False
_lock = _thread.allocate_lock()
_queue = []                 # FIFO of PCM frames (u16 LE)
_period_us = 1_000_000 // SAMPLE_RATE_HZ

def _sampler():
    global _adc
    adc = _adc
    buf = bytearray(PACKET_SAMPLES * 2)
    i = 0
    t_next = utime.ticks_us()

    while _running:
        if adc is None:
            return

        v = adc.read_u16()      # 0..65535
        j = i * 2
        buf[j] = v & 0xFF
        buf[j + 1] = (v >> 8) & 0xFF
        i += 1

        t_next = utime.ticks_add(t_next, _period_us)

        if i >= PACKET_SAMPLES:
            frame = bytes(buf)
            with _lock:
                if len(_queue) >= QUEUE_MAX_PACKETS:
                    _queue.pop(0)
                _queue.append(frame)
            i = 0

        # fixed-step sampling delay
        while utime.ticks_diff(t_next, utime.ticks_us()) > 0:
            pass

def start():
    """Init ADC and start the sampler thread."""
    global _adc, _running
    if _running:
        return
    _adc = ADC(Pin(AUDIO_ADC_PIN))
    _running = True
    _thread.start_new_thread(_sampler, ())

def stop():
    """Stop the sampler thread."""
    global _running
    _running = False

def try_acquire_packet():
    """Pop the oldest frame if available (or None)."""
    with _lock:
        if _queue:
            return _queue.pop(0)
    return None

# NOTE: Do not import this module inside itself (no `import audio` here).
# If you want to test from PC/REPL, use `main.py` or call start()/stop() directly
# under a guard like below:
if __name__ == "__main__":
    # minimal self-test (optional; comment out if you prefer)
    start()
    utime.sleep_ms(50)
    _ = try_acquire_packet()
    stop()