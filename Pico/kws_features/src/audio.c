#include "audio.h"
#include "config.h"

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/util/queue.h"
#include <string.h>

#define ADC_GPIO     MIC_GPIO
#define ADC_INPUT    MIC_ADC_INPUT

typedef struct {
    uint8_t  bytes[AUDIO_HEADER_BYTES + SAMPLES_PER_PKT * sizeof(int16_t)];
} packet_buf_t;

static packet_buf_t pktA, pktB;
static packet_buf_t* write_pkt;
static int16_t* wr_samples;
static uint32_t seq_counter = 0;

static int dma_chan;
static queue_t ready_q;

// DC blocker for cheap mic modules
static float dc_prev_in = 0.f, dc_prev_out = 0.f;
static inline int16_t dc_block(int16_t x) {
    const float R = 0.995f; // ~16 Hz cutoff @ 16k
    float xn = (float)x;
    float y = (xn - dc_prev_in) + R * dc_prev_out;
    dc_prev_in = xn;
    dc_prev_out = y;
    if (y > 32767.f) y = 32767.f;
    if (y < -32768.f) y = -32768.f;
    return (int16_t)y;
}

static void fill_header(packet_buf_t* p, uint32_t seq) {
    uint32_t* p32 = (uint32_t*)p->bytes;
    p32[0] = seq;                 // seq
    p32[1] = SAMPLE_RATE_HZ;      // sample_rate
    uint16_t* p16 = (uint16_t*)&p->bytes[sizeof(uint32_t)*2];
    p16[0] = (uint16_t)SAMPLES_PER_PKT; // n
}

static int16_t* payload_ptr(packet_buf_t* p) {
    return (int16_t*)(p->bytes + AUDIO_HEADER_BYTES);
}

static void __isr dma_handler(void) {
    dma_hw->ints0 = 1u << dma_chan;

    int16_t* s = payload_ptr(write_pkt);
    // ADC FIFO is 12-bit unsigned in 16-bit. Center+scale, then DC-block.
    for (int i = 0; i < SAMPLES_PER_PKT; ++i) {
        uint16_t raw = (uint16_t)s[i];
        int16_t x = ((int)raw - 2048) << 4;
        s[i] = dc_block(x);
    }

    uint8_t* pbytes = write_pkt->bytes;
    queue_try_add(&ready_q, &pbytes);

    write_pkt = (write_pkt == &pktA) ? &pktB : &pktA;
    fill_header(write_pkt, ++seq_counter);
    wr_samples = payload_ptr(write_pkt);

    dma_channel_set_write_addr(dma_chan, wr_samples, false);
    dma_channel_set_trans_count(dma_chan, SAMPLES_PER_PKT, true);
}

void audio_init(void) {
    queue_init(&ready_q, sizeof(uint8_t*), 8);

    write_pkt = &pktA;
    fill_header(write_pkt, seq_counter);
    wr_samples = payload_ptr(write_pkt);

    adc_init();
    adc_gpio_init(ADC_GPIO);
    adc_select_input(ADC_INPUT);

    float div = 48000000.0f / (float)SAMPLE_RATE_HZ;
    adc_set_clkdiv(div);

    adc_fifo_setup(
        true,   // enable FIFO
        true,   // DMA DREQ
        1,      // threshold
        false,  // err bit
        false   // keep 12-bit in 16-bit
    );

    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_ADC);

    dma_channel_configure(
        dma_chan, &c,
        wr_samples,
        &adc_hw->fifo,
        SAMPLES_PER_PKT,
        false
    );

    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

void audio_start(void) {
    adc_run(true);
    dma_channel_start(dma_chan);
}

const uint8_t* audio_try_acquire_packet(uint16_t* out_len) {
    uint8_t* pbytes = NULL;
    if (queue_try_remove(&ready_q, &pbytes)) {
        if (out_len) *out_len = AUDIO_HEADER_BYTES + SAMPLES_PER_PKT * sizeof(int16_t);
        return pbytes;
    }
    return NULL;
}

void audio_release_packet(const uint8_t* ptr) { (void)ptr; }