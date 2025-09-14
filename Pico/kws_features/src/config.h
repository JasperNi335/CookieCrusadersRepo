#ifndef CONFIG_H
#define CONFIG_H

// Audio sampling
#define SAMPLE_RATE_HZ      16000
#define SAMPLES_PER_PKT     1024    // ~64 ms

// Mic (XC4438) -> ADC0 (GPIO26, phys pin 31)
#define MIC_GPIO        26
#define MIC_ADC_INPUT   0

// Speaker/amp (XC3744) -> PWM on GPIO27 (phys pin 32)
#define SPEAKER_PIN     27

// Framing for USB binary stream (host reads this)
#define FRAMING_MAGIC   0xAA55AA55u

// Melody/cooldown for command “BEEP”
#define BEEP_TONE_MS           150
#define BEEP_GAP_MS            60
#define COOLDOWN_MS            700

#endif