#ifndef CONFIG_H
#define CONFIG_H

// ===== Audio sampling =====
#define SAMPLE_RATE_HZ      16000
#define SAMPLES_PER_PKT     1024    // ~64 ms @ 16 kHz

// ===== Microphone (XC4438 analog A0) -> ADC0 on GPIO26 (physical pin 31) =====
#define MIC_GPIO        26
#define MIC_ADC_INPUT   0

// ===== PWM speaker/amp output on GPIO27 (physical pin 32) =====
#define SPEAKER_PIN     27

// ===== Detection tuning =====
#define VOICE_THRESH_DBFS   (-30.0f)  // raise toward -25 if too sensitive
#define BEEP_MS             (400)     // beep length
#define COOLDOWN_MS         (800)     // min gap between beeps

#endif // CONFIG_H