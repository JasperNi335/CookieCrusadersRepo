# Stream + audio settings used by main.py and audio.py

# Serial framing (must match host)
FRAMING_MAGIC = 0xA1B2C3D4

# Enable binary frame streaming over the same CDC port
ENABLE_USB_STREAM = True

# Audio & DSP
AUDIO_ADC_PIN = 26        # GP26 = ADC0
SAMPLE_RATE_HZ = 8000     # 8 kHz
PACKET_SAMPLES = 256      # samples per packet
QUEUE_MAX_PACKETS = 8

# Simple sound detection
ENV_CENTER = 32768        # mid of 16-bit ADC
ENV_THRESH = 1500         # tweak for your mic; start here
ENV_HANG_MS = 600         # cooldown before another beep

# LED / Speaker
LED_PIN = "LED"           # works on Pico & Pico W
SPEAKER_PIN = 18          # PWM-capable pin wired to speaker/amp
BEEP_TONE_HZ = 2000
BEEP_MS = 120
BEEP_GAP_MS = 60