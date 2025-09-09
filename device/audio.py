# audio.py
import _thread, time, machine, array
from config import AUDIO_ADC_PIN, SAMPLE_RATE_HZ, PACKET_SAMPLES, QUEUE_MAX_PACKETS

_adc = None
_running = False
_queue = []
_lock = _thread.allocate_lock()

def _sampler_thread():
    global _queue, _running
    period_us = int(1_000_000 // SAMPLE_RATE_HZ)
    # Pre-allocate a packet buffer of 16-bit samples (little-endian later)
    while _running:
        buf = array.array('H', (0 for _ in range(PACKET_SAMPLES)))
        t_next = time.ticks_us()
        for i in range(PACKET_SAMPLES):
            # Busy-wait timing loop; simple but effective at ~8 kHz
            while time.ticks_diff(time.ticks_us(), t_next) < 0:
                pass
            buf[i] = _adc.read_u16()
            t_next = time.ticks_add(t_next, period_us)
        # push if there is room
        with _lock:
            if len(_queue) < QUEUE_MAX_PACKETS:
                _queue.append(buf)
            else:
                # drop oldest (backpressure) then push
                _queue.pop(0)
                _queue.append(buf)

def start():
    global _adc, _running
    if _running:
        return
    _adc = machine.ADC(AUDIO_ADC_PIN)
    _running = True
    _thread.start_new_thread(_sampler_thread, ())

def stop():
    global _running
    _running = False

def try_acquire_packet():
    """Returns a bytes payload (little-endian u16 stream) or None."""
    with _lock:
        if not _queue:
            return None
        buf = _queue.pop(0)
    # Convert array('H') to bytes (little-endian)
    # array('H') is native-endian; RP2040 is little-endian
    return memoryview(buf).tobytes()