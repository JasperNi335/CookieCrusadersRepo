#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "config.h"
#include "audio.h"
#include "mfcc.h"

// ===== Hamming window =====
static float g_hamm[SAMPLES_PER_FRAME];
static void make_hamming(void) {
    for (int n = 0; n < SAMPLES_PER_FRAME; n++) {
        g_hamm[n] = 0.54f - 0.46f * cosf(2.f * (float)M_PI * n / (SAMPLES_PER_FRAME - 1));
    }
}

// ===== simple AGC =====
typedef struct {
    float gain;
    float env;
} agc_t;

static void agc_init(agc_t* a) {
    a->gain = 1.0f;
    a->env  = 1.0f;
}

static void agc_process(agc_t* a, float* x, int n) {
    float sum = 0.f;
    for (int i=0;i<n;i++) sum += x[i]*x[i];
    float rms = sqrtf(sum / (float)n);
    if (rms < 1e-6f) rms = 1e-6f;

    float target = AGC_TARGET_RMS / 32768.0f;
    float desired_gain = target / rms;
    if (desired_gain > AGC_MAX_GAIN) desired_gain = AGC_MAX_GAIN;

    float alpha = (desired_gain < a->gain) ? AGC_ATTACK : AGC_RELEASE;
    a->gain = a->gain + alpha * (desired_gain - a->gain);

    for (int i=0;i<n;i++) x[i] *= a->gain;
}

// ===== pre-emphasis =====
static void preemph(float* x, int n) {
    float prev = 0.f;
    for (int i=0;i<n;i++) {
        float cur = x[i];
        x[i] = cur - PREEMPHASIS_ALPHA * prev;
        prev = cur;
    }
}

// ===== simple energy VAD =====
typedef struct {
    float noise_db;
    int   in_speech;
    int   run;
} vad_t;

static void vad_init(vad_t* v) {
    v->noise_db = -40.f; // initial guess
    v->in_speech = 0;
    v->run = 0;
}

static int vad_update(vad_t* v, const float* x, int n) {
    float sum = 0.f;
    for (int i=0;i<n;i++) sum += x[i]*x[i];
    float rms = sqrtf(sum/(float)n);
    float db = 20.f * log10f(rms + 1e-9f);

    // noise tracker
    float beta = 0.99f;
    if (!v->in_speech) {
        v->noise_db = beta * v->noise_db + (1.f - beta) * db;
    }

    float margin = v->in_speech ? VAD_ENERGY_STOP : VAD_ENERGY_START;
    int speech = (db > v->noise_db + margin) ? 1 : 0;

    if (speech) {
        v->run++;
        if (v->run >= VAD_MIN_FRAMES) v->in_speech = 1;
    } else {
        v->run--;
        if (v->run <= 0) { v->run = 0; v->in_speech = 0; }
    }
    if (v->run > VAD_MAX_FRAMES) v->run = VAD_MAX_FRAMES;

    return v->in_speech;
}

// ===== template & cosine similarity =====
static float g_kws_template[MFCC_NUM_COEFFS] = {0};
static int   g_template_ready = 0;

static float cosine_sim(const float* a, const float* b, int n) {
    float dot=0.f, na=0.f, nb=0.f;
    for (int i=0;i<n;i++) {
        dot += a[i]*b[i];
        na  += a[i]*a[i];
        nb  += b[i]*b[i];
    }
    if (na <= 0.f || nb <= 0.f) return 0.f;
    return dot / (sqrtf(na)*sqrtf(nb));
}

// ===== Collect template via serial =====
static void capture_template_once(void) {
    puts("Press 't' in the serial terminal, then say your wake word...");
    while (true) {
        int ch = getchar_timeout_us(0);
        if (ch == 't') break;
        sleep_ms(10);
    }

    const int frames_to_avg = 50; // 50 * 10 ms = 0.5 s
    float acc[MFCC_NUM_COEFFS] = {0};
    int frames_got = 0;

    static int16_t frame_i16[SAMPLES_PER_FRAME];
    static float   f[SAMPLES_PER_FRAME];

    // wait for speech start
    vad_t vtmp; vad_init(&vtmp);
    while (!vtmp.in_speech) {
        audio_get_frame(frame_i16, SAMPLES_PER_HOP);
        audio_get_recent(frame_i16, SAMPLES_PER_FRAME);
        for (int i=0;i<SAMPLES_PER_FRAME;i++) f[i] = frame_i16[i] / 32768.0f;
        preemph(f, SAMPLES_PER_FRAME);
        if (vad_update(&vtmp, f, SAMPLES_PER_FRAME)) break;
    }

    while (frames_got < frames_to_avg) {
        audio_get_frame(frame_i16, SAMPLES_PER_HOP);
        audio_get_recent(frame_i16, SAMPLES_PER_FRAME);

        for (int i=0;i<SAMPLES_PER_FRAME;i++) f[i] = (frame_i16[i] / 32768.0f) * g_hamm[i];

        float mf[MFCC_NUM_COEFFS];
        mfcc_compute(f, SAMPLES_PER_FRAME, mf, MFCC_NUM_COEFFS, MFCC_USE_ENERGY);
        for (int k=0;k<MFCC_NUM_COEFFS;k++) acc[k]+=mf[k];
        frames_got++;
    }

    for (int k=0;k<MFCC_NUM_COEFFS;k++) g_kws_template[k] = acc[k] / (float)frames_to_avg;
    g_template_ready = 1;
    puts("Template captured!");
}

// ===== KWS state =====
typedef struct {
    int cooldown;
} kws_state_t;

static void kws_init(kws_state_t* s) { s->cooldown = 0; }

static bool kws_detect(kws_state_t* s, const float* mfcc) {
    if (!g_template_ready) return false;
    float cs = cosine_sim(mfcc, g_kws_template, MFCC_NUM_COEFFS);
    if (s->cooldown > 0) { s->cooldown--; return false; }
    if (cs >= KWS_THRESHOLD) { s->cooldown = KWS_COOLDOWN_FRAMES; return true; }
    return false;
}

// ===== main =====
int main() {
    stdio_init_all();
    sleep_ms(400); // let USB settle

    // init modules
    audio_init(); // <-- matches audio.h/audio.c signature
    mfcc_init(SAMPLE_RATE_HZ, MFCC_NUM_FBANKS, MFCC_NUM_COEFFS);
    make_hamming();

    agc_t agc; agc_init(&agc);
    vad_t vad; vad_init(&vad);
    kws_state_t kws; kws_init(&kws);

    // frame buffers
    static int16_t frame[SAMPLES_PER_FRAME];
    static int16_t hopbuf[SAMPLES_PER_HOP];
    static float   f[SAMPLES_PER_FRAME];

    puts("Ready. Press 't' to capture template, 'r' to reset template.");
    puts("Streaming…");

    while (true) {
        // get next hop
        audio_get_frame(hopbuf, SAMPLES_PER_HOP);
        // assemble sliding frame (last N samples)
        audio_get_recent(frame, SAMPLES_PER_FRAME);

        // convert to float
        for (int i=0;i<SAMPLES_PER_FRAME;i++) f[i] = (frame[i] / 32768.0f);

        // AGC + pre-emph + VAD
        agc_process(&agc, f, SAMPLES_PER_FRAME);
        preemph(f, SAMPLES_PER_FRAME);
        (void)vad_update(&vad, f, SAMPLES_PER_FRAME);

        // MFCC (always; or gate by vad.in_speech if you want)
        float mf[MFCC_NUM_COEFFS];
        for (int i=0;i<SAMPLES_PER_FRAME;i++) f[i] *= g_hamm[i];
        mfcc_compute(f, SAMPLES_PER_FRAME, mf, MFCC_NUM_COEFFS, MFCC_USE_ENERGY);

        // KWS
        if (kws_detect(&kws, mf)) {
            puts("[KWS] Wake word detected!");
            // TODO: trigger team action (GPIO, LED, etc.)
        }

        // serial commands
        int ch = getchar_timeout_us(0);
        if (ch == 't') capture_template_once();
        if (ch == 'r') { g_template_ready = 0; puts("Template cleared."); }
    }
    // not reached
    // audio_stop();
    // return 0;
}