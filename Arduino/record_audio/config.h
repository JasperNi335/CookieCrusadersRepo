// config.h — Arduino port of MicroPython config.py

#ifndef CONFIG_H
#define CONFIG_H

// ------------ Pins ------------
#define LED_PIN         25    // onboard LED
#define SPEAKER_PIN     27    // your speaker on GP27
#define AUDIO_ADC_PIN   26    // your mic on GP26 (ADC0)

// ------------ Sampling (for trigger loop) ------------
#define SAMPLE_RATE_HZ     8000
#define PACKET_SAMPLES     256
#define QUEUE_MAX_PACKETS  6

// ------------ Envelope / trigger ------------
#define ENV_THRESH   200     // speak/clap above this to trigger
#define ENV_HANG_MS  1500     // cooldown after a trigger (ms)

// ------------ Debug ------------
#define PLOT_DEBUG   1        // 1 = true, 0 = false

// ------------ WAV playback ------------
#define SOUNDS_DIR       "/sounds"   // not used directly on Arduino FS
#define WAV_CHUNK_BYTES  1024
#define MAX_WAV_MS       7000        // safety cap per sound (ms)

// Expectation for your current files: mono 8-bit unsigned @ 8000 Hz.
// To re-encode with ffmpeg:
//   ffmpeg -y -i in.wav -ac 1 -ar 8000 -sample_fmt u8 out_u8_8k.wav

#endif // CONFIG_H
