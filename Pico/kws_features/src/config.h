#ifndef CONFIG_H
#define CONFIG_H

// ===== Audio sampling =====
#define SAMPLE_RATE_HZ      16000
#define SAMPLES_PER_FRAME   400     // 25 ms @ 16 kHz
#define SAMPLES_PER_HOP     160     // 10 ms hop

// ===== Microphone pin/ADC =====
// LM386 module → connect OUT pin to Pico ADC0 (GPIO26)
#define MIC_GPIO        26
#define MIC_ADC_INPUT   0

// ===== Automatic Gain Control (AGC) =====
#define AGC_TARGET_RMS      2000.0f   // target RMS level
#define AGC_MAX_GAIN        50.0f     // clamp max gain
#define AGC_ATTACK          0.05f     // how fast gain increases
#define AGC_RELEASE         0.005f    // how fast gain decreases

// ===== Pre-emphasis =====
#define PREEMPHASIS_ALPHA   0.97f

// ===== Voice Activity Detection (VAD) =====
#define VAD_ENERGY_START    6.0f    // dB above noise floor to trigger speech
#define VAD_ENERGY_STOP     3.0f    // dB margin to drop speech
#define VAD_MIN_FRAMES      3       // consecutive frames before speech=true
#define VAD_MAX_FRAMES      50      // cap run length

// ===== MFCC parameters =====
#define MFCC_NUM_FBANKS     26
#define MFCC_NUM_COEFFS     13
#define MFCC_USE_ENERGY     1       // include log energy as coefficient 0

// ===== Keyword spotting (KWS) =====
#define KWS_THRESHOLD       0.85f   // cosine similarity threshold
#define KWS_COOLDOWN_FRAMES 50      // cooldown after detection

#endif // CONFIG_H