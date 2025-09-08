#ifndef CONFIG_H
#define CONFIG_H

// ===== Audio sampling =====
#define SAMPLE_RATE_HZ      16000
#define SAMPLES_PER_PKT     1024    // 64 ms @ 16 kHz

// ===== Microphone pin/ADC =====
#define MIC_GPIO        26          // ADC0 on GPIO26
#define MIC_ADC_INPUT   0

// ===== UART link to ESP =====
#define UART_INST       uart0
#define UART_BAUD       921600      // fast & reliable
#define UART_TX_PIN     0           // Pico UART0 TX = GPIO0
#define UART_RX_PIN     1           // Pico UART0 RX = GPIO1 (not used here but init anyway)

// ===== Framing (Pico -> ESP) =====
// We add a small 6-byte framing header before each audio packet:
// magic (0xAA55AA55, uint32), len (uint16) for the following payload bytes.
// The payload starts with: seq(uint32), sample_rate(uint32), n(uint16), then PCM16[n].
#define FRAMING_MAGIC   0xAA55AA55u

#endif // CONFIG_H