# ------------ Pins ------------
LED_PIN        = 25     # onboard LED
SPEAKER_PIN    = 18     # PWM audio out (connect amp/speaker here)
AUDIO_ADC_PIN  = 26     # mic input (ADC0 = GP26)

# ------------ Sampling ------------
SAMPLE_RATE_HZ    = 8000
PACKET_SAMPLES    = 256
QUEUE_MAX_PACKETS = 6

# ------------ Envelope / trigger ------------
# With auto-centering envelope, start around 3000–10000 and tune.
ENV_THRESH   = 3000      # raise if it triggers too easily
ENV_HANG_MS  = 1500      # cooldown after trigger (ms)

# ------------ Streaming / debug ------------
ENABLE_USB_STREAM = False
PLOT_DEBUG        = True   # prints "env,<value>" to REPL

# ------------ WAV playback ------------
SOUNDS_DIR       = "/sounds"  # files copied to the Pico here
SOUND_FILES      = []         # leave empty to auto-scan SOUNDS_DIR
WAV_CHUNK_BYTES  = 256
MAX_WAV_MS       = 6000       # safety cap per sound (ms)

# ------------ Framing (for USB stream) ------------
FRAMING_MAGIC = 0x534B5743  # 'SKWC'