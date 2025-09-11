# ------------ Pins ------------
LED_PIN        = 25      # onboard LED
SPEAKER_PIN    = 27      # your speaker on GP27
AUDIO_ADC_PIN  = 26      # your mic on GP26 (ADC0)

# ------------ Sampling (for trigger loop) ------------
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

# Expectation for your current files: mono 8-bit unsigned @ 8000 Hz.
# Re-encode if needed:
#   ffmpeg -y -i in.wav -ac 1 -ar 8000 -sample_fmt u8 out_u8_8k.wav