# ------------ Pins ------------
LED_PIN        = 25      # onboard LED
SPEAKER_PIN    = 27      # <-- friend’s working pin; set to 18 if your wire is on GP18
AUDIO_ADC_PIN  = 26      # mic input (ADC0 = GP26)

# ------------ Sampling ------------
SAMPLE_RATE_HZ    = 8000
PACKET_SAMPLES    = 256
QUEUE_MAX_PACKETS = 6

# ------------ Envelope / trigger ------------
ENV_THRESH   = 3000      # speak/clap above this to trigger
ENV_HANG_MS  = 1500      # cooldown after a trigger (ms)

# ------------ Debug ------------
PLOT_DEBUG   = True      # prints: "env,<value>" each frame

# ------------ WAV playback ------------
SOUNDS_DIR       = "/sounds"  # preferred folder on Pico; falls back to root "/"
WAV_CHUNK_BYTES  = 1024       # stream chunk size
MAX_WAV_MS       = 7000       # safety cap per sound

# Expectation: mono 8-bit unsigned @ 8000 Hz WAV files.
# If yours are different, re-encode (e.g., with ffmpeg):
# ffmpeg -y -i in.wav -ac 1 -ar 8000 -sample_fmt u8 out_u8_8k.wav