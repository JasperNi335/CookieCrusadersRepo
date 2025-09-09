# config.py

# ---------- Pins ----------
LED_PIN_NAME = "LED"   # Pico onboard LED alias
AUDIO_ADC_PIN = 26     # GP26/ADC0; change if your mic is on another ADC
SPEAKER_PIN   = 18     # PWM-capable pin wired to your speaker/buzzer

# ---------- Audio ----------
SAMPLE_RATE_HZ   = 8000      # keep modest in MicroPython; 8 kHz is realistic
PACKET_SAMPLES   = 256       # 256 samples -> 512-byte payload
QUEUE_MAX_PACKETS = 12       # backpressure limit

# ---------- Protocol ----------
# 6-byte header: [u32 magic][u16 payload_len]
# Set to the SAME value your host reader expects.
FRAMING_MAGIC = 0xA1B2C3D4   # <— CHANGE if your PC tool expects a different magic

# ---------- Commands ----------
BEEP_TONE_MS  = 120
BEEP_GAP_MS   = 60
COOLDOWN_MS   = 1200