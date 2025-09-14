#include "mfcc.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// lightweight MFCC (FFT via radix-2 DIT, Hamming -> power spectrum -> mel -> log -> DCT)

#define MAX_FBANKS   40
#define MAX_COEFFS   20
#define MAX_N        512

static int g_sr = 16000;
static int g_num_fb = 26;
static int g_num_ceps = 13;

static float g_hann[MAX_N];
static int   g_fft_N = 512;

static float g_mel_fb[MAX_FBANKS][MAX_N/2+1];
static int   g_bin_lo[MAX_FBANKS], g_bin_hi[MAX_FBANKS];

static float g_dct[MAX_COEFFS][MAX_FBANKS];

static inline float hz_to_mel(float hz)   { return 2595.0f * log10f(1.0f + hz/700.0f); }
static inline float mel_to_hz(float mel)  { return 700.0f * (powf(10.0f, mel/2595.0f) - 1.0f); }

// ==== tiny FFT (real -> complex) ====
typedef struct { float r, i; } cpx;

static void fft(cpx *a, int n) {
    // bit-reverse
    for (int i=1, j=0; i<n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j &= ~bit;
        j |= bit;
        if (i < j) { cpx t = a[i]; a[i] = a[j]; a[j] = t; }
    }
    // stages
    for (int len=2; len<=n; len<<=1) {
        float ang = -2.f * (float)M_PI / (float)len;
        cpx wlen = { cosf(ang), sinf(ang) };
        for (int i=0; i<n; i+=len) {
            cpx w = {1.f, 0.f};
            for (int j=0; j<len/2; j++) {
                cpx u = a[i+j];
                cpx v = { a[i+j+len/2].r * w.r - a[i+j+len/2].i * w.i,
                          a[i+j+len/2].r * w.i + a[i+j+len/2].i * w.r };
                a[i+j].r = u.r + v.r; a[i+j].i = u.i + v.i;
                a[i+j+len/2].r = u.r - v.r; a[i+j+len/2].i = u.i - v.i;
                float wr = w.r * wlen.r - w.i * wlen.i;
                float wi = w.r * wlen.i + w.i * wlen.r;
                w.r = wr; w.i = wi;
            }
        }
    }
}

static int next_pow2(int x) { int n=1; while(n<x) n<<=1; return n; }

void mfcc_init(int sample_rate, int num_fbank, int num_coeffs) {
    g_sr = sample_rate;
    g_num_fb = num_fbank;     if (g_num_fb > MAX_FBANKS) g_num_fb = MAX_FBANKS;
    g_num_ceps = num_coeffs;  if (g_num_ceps > MAX_COEFFS) g_num_ceps = MAX_COEFFS;

    g_fft_N = 512;
    if (g_fft_N > MAX_N) g_fft_N = MAX_N;

    // Hamming (actually Hann here; close enough for our use)
    for (int n=0; n<g_fft_N; n++) {
        g_hann[n] = 0.54f - 0.46f * cosf(2.f*(float)M_PI*n/(g_fft_N-1));
    }

    // mel filterbank
    float f_min = 0.0f, f_max = 0.5f * g_sr;
    float m_min = hz_to_mel(f_min);
    float m_max = hz_to_mel(f_max);
    float dm = (m_max - m_min) / (g_num_fb + 1);
    float mel_pts[MAX_FBANKS+2];
    for (int i=0;i<g_num_fb+2;i++) mel_pts[i] = m_min + dm * i;

    float f_pts[MAX_FBANKS+2];
    for (int i=0;i<g_num_fb+2;i++) f_pts[i] = mel_to_hz(mel_pts[i]);

    int N2 = g_fft_N/2;
    float bin_hz = (float)g_sr / (float)g_fft_N;

    for (int i=0;i<g_num_fb;i++) {
        float f_lo = f_pts[i], f_c = f_pts[i+1], f_hi = f_pts[i+2];
        int b_lo = (int)floorf(f_lo / bin_hz);
        int b_c  = (int)floorf(f_c  / bin_hz);
        int b_hi = (int)floorf(f_hi / bin_hz);
        if (b_lo < 0) b_lo = 0; if (b_hi > N2) b_hi = N2;
        g_bin_lo[i] = b_lo; g_bin_hi[i] = b_hi;
        for (int b=b_lo;b<=b_hi;b++) g_mel_fb[i][b] = 0.f;

        for (int b=b_lo; b<=b_c; b++) {
            float w = (b - b_lo) / (float)(b_c - b_lo + 1e-9f);
            if (w<0) w=0; if (w>1) w=1;
            g_mel_fb[i][b] = w;
        }
        for (int b=b_c; b<=b_hi; b++) {
            float w = (b_hi - b) / (float)(b_hi - b_c + 1e-9f);
            if (w<0) w=0; if (w>1) w=1;
            if (g_mel_fb[i][b] < w) g_mel_fb[i][b] = w;
        }
    }

    // DCT matrix (type-II, orthonormal-ish)
    for (int k=0;k<g_num_ceps;k++) {
        for (int n=0;n<g_num_fb;n++) {
            g_dct[k][n] = cosf((float)M_PI * k * (2*n+1) / (2.f * g_num_fb));
        }
    }
}

void mfcc_compute(const float* frame, int N, float* mfcc_out,
                  int num_coeffs, bool include_c0) {
    // window + zero-pad
    int L = N;
    if (L > g_fft_N) L = g_fft_N;

    static cpx X[MAX_N];
    for (int i=0;i<g_fft_N;i++) { X[i].r = 0.f; X[i].i = 0.f; }

    for (int i=0;i<L;i++) {
        float w = g_hann[i];
        X[i].r = frame[i] * w;
    }

    fft(X, g_fft_N);

    int N2 = g_fft_N/2;
    static float P[MAX_N/2+1];
    for (int b=0;b<=N2;b++) {
        float re = X[b].r, im = X[b].i;
        P[b] = re*re + im*im; // power
    }

    // mel energies
    static float E[MAX_FBANKS];
    for (int m=0;m<g_num_fb;m++) {
        float e = 0.f;
        int lo = g_bin_lo[m], hi = g_bin_hi[m];
        if (hi> N2) hi=N2;
        for (int b=lo;b<=hi;b++) e += P[b] * g_mel_fb[m][b];
        if (e < 1e-10f) e = 1e-10f;
        E[m] = logf(e);
    }

    // DCT -> cepstra
    int want = num_coeffs;
    if (want > g_num_ceps) want = g_num_ceps;

    int k0 = include_c0 ? 0 : 1; // skip C0 if requested
    int out_idx = 0;
    for (int k=k0; k<k0+want; k++) {
        float c = 0.f;
        for (int n=0;n<g_num_fb;n++) c += g_dct[k][n] * E[n];
        mfcc_out[out_idx++] = c;
    }
}