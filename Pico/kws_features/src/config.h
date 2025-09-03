#ifndef CONFIG_H
#define CONFIG_H

// -------- Audio config --------
#define SAMPLE_RATE_HZ       16000   // 16 kHz sample rate
#define FRAME_LEN_SAMPLES    400     // 25 ms window at 16 kHz
#define HOP_LEN_SAMPLES      160     // 10 ms hop

// -------- FFT / MEL / MFCC --------
#define FFT_SIZE             512     // next power-of-two >= FRAME_LEN
#define NUM_MEL_BANDS        32
#define NUM_MFCC             13

// -------- Mic / ADC pinning --------
// Using analog mic output into ADC1 on GPIO27
#define MIC_ADC_INPUT        1       // ADC1
#define MIC_GPIO             27      // GPIO27

// -------- Pre-emphasis (speech) ----
#define PREEMPH_COEF         0.97f

#endif // CONFIG_H