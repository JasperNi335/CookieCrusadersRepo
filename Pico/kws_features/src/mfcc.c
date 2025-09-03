#include "mfcc.h"
#include "config.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ----- Global state -----
static int g_sr, g_fft, g_frame, g_nmel, g_nmfcc;
static float *g_window = NULL;          // Hamming window [g_frame]
static float *g_mel_fb = NULL;          // [g_nmel * (g_fft/2+1)] row-major
static float *g_dct = NULL;             // [g_nmfcc * g_nmel]

// FFT scratch
static float *re = NULL;                // [g_fft]
static float *im = NULL;                // [g_fft]

// ---------- Helpers ----------
static float hz_to_mel(float f) {
    return 2595.0f * log10f(1.0f + f / 700.0f);
}
static float mel_to_hz(float m) {
    return 700.0f * (powf(10.0f, m / 2595.0f) - 1.0f);
}

static void build_hamming(int N, float *w) {
    for (int n = 0; n < N; ++n) {
        w[n] = 0.54f - 0.46f * cosf(2.0f * (float)M_PI * (float)n / (float)(N - 1));
    }
}

static void bit_reverse(float *xr, float *xi, int n) {
    int j = 0;
    for (int i = 0; i < n; ++i) {
        if (i < j) {
            float tr = xr[j], ti = xi[j];
            xr[j] = xr[i]; xi[j] = xi[i];
            xr[i] = tr;    xi[i] = ti;
        }
        int m = n >> 1;
        while (m >= 1 && j >= m) { j -= m; m >>= 1; }
        j += m;
    }
}

static void fft_radix2(float *xr, float *xi, int n) {
    bit_reverse(xr, xi, n);
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * (float)M_PI / (float)len;
        for (int i = 0; i < n; i += len) {
            for (int k = 0; k < len/2; ++k) {
                float c = cosf(ang * k);
                float s = sinf(ang * k);
                int i0 = i + k;
                int i1 = i + k + len/2;
                float tr =  xr[i1]*c - xi[i1]*s;
                float ti =  xr[i1]*s + xi[i1]*c;
                xr[i1] = xr[i0] - tr;
                xi[i1] = xi[i0] - ti;
                xr[i0] += tr;
                xi[i0] += ti;
            }
        }
    }
}

static void build_mel_filterbank(int sr, int fft, int nmel, float *fb) {
    int nfft2 = fft/2 + 1;
    float fmin = 300.0f;
    float fmax = (float)sr * 0.5f;

    float mmin = hz_to_mel(fmin);
    float mmax = hz_to_mel(fmax);

    // mel centers (nmel + 2) for triangles
    float *mel = (float *)malloc((nmel + 2) * sizeof(float));
    for (int i = 0; i < nmel + 2; ++i) {
        mel[i] = mmin + (mmax - mmin) * (float)i / (float)(nmel + 1);
    }
    // convert to Hz then bins
    float *hz = (float *)malloc((nmel + 2) * sizeof(float));
    int   *bin = (int *)malloc((nmel + 2) * sizeof(int));
    for (int i = 0; i < nmel + 2; ++i) {
        hz[i] = mel_to_hz(mel[i]);
        bin[i] = (int)floorf((fft * hz[i]) / (float)sr);
        if (bin[i] < 0) bin[i] = 0;
        if (bin[i] >= nfft2) bin[i] = nfft2 - 1;
    }

    memset(fb, 0, nmel * nfft2 * sizeof(float));
    for (int m = 0; m < nmel; ++m) {
        int b0 = bin[m], b1 = bin[m+1], b2 = bin[m+2];
        for (int k = b0; k < b1; ++k) {
            float w = (k - b0) / (float)(b1 - b0 + 1e-9f);
            fb[m*nfft2 + k] = w;
        }
        for (int k = b1; k < b2; ++k) {
            float w = (b2 - k) / (float)(b2 - b1 + 1e-9f);
            fb[m*nfft2 + k] = w;
        }
    }

    free(mel); free(hz); free(bin);
}

static void build_dct(int nmel, int nmfcc, float *dct) {
    // Type-II DCT matrix (row-orthonormal)
    const float scale0 = sqrtf(1.0f / (float)nmel);
    const float scale  = sqrtf(2.0f / (float)nmel);
    for (int i = 0; i < nmfcc; ++i) {
        for (int j = 0; j < nmel; ++j) {
            float s = (i == 0) ? scale0 : scale;
            dct[i*nmel + j] = s * cosf((float)M_PI * (j + 0.5f) * i / (float)nmel);
        }
    }
}

bool mfcc_init(int sample_rate, int fft_size, int frame_len, int num_mel, int num_mfcc) {
    g_sr = sample_rate; g_fft = fft_size; g_frame = frame_len; g_nmel = num_mel; g_nmfcc = num_mfcc;
    int nfft2 = g_fft/2 + 1;

    // Allocate
    g_window = (float*)malloc(g_frame * sizeof(float));
    g_mel_fb = (float*)malloc(g_nmel * nfft2 * sizeof(float));
    g_dct    = (float*)malloc(g_nmfcc * g_nmel * sizeof(float));
    re = (float*)malloc(g_fft * sizeof(float));
    im = (float*)malloc(g_fft * sizeof(float));

    if (!g_window || !g_mel_fb || !g_dct || !re || !im) return false;

    build_hamming(g_frame, g_window);
    build_mel_filterbank(g_sr, g_fft, g_nmel, g_mel_fb);
    build_dct(g_nmel, g_nmfcc, g_dct);
    return true;
}

void mfcc_compute_frame(const int16_t *pcm, float *out_mfcc) {
    // 1) Pre-emphasis + Window + Zero-pad to FFT_SIZE
    float prev = 0.0f;
    for (int n = 0; n < g_frame; ++n) {
        float x = (float)pcm[n] / 32768.0f;
        float y = x - PREEMPH_COEF * prev;
        prev = x;
        re[n] = y * g_window[n];
        im[n] = 0.0f;
    }
    for (int n = g_frame; n < g_fft; ++n) { re[n] = 0.0f; im[n] = 0.0f; }

    // 2) FFT -> power spectrum
    fft_radix2(re, im, g_fft);
    int nfft2 = g_fft/2 + 1;

    static float ps[FFT_SIZE/2 + 1];
    for (int k = 0; k < nfft2; ++k) {
        float rr = re[k], ii = im[k];
        ps[k] = rr*rr + ii*ii;
    }

    // 3) Mel filterbank energies (log)
    static float melE[NUM_MEL_BANDS];
    for (int m = 0; m < g_nmel; ++m) {
        float e = 0.0f;
        const float *w = &g_mel_fb[m*nfft2];
        for (int k = 0; k < nfft2; ++k) e += ps[k] * w[k];
        melE[m] = logf(e + 1e-10f);
    }

    // 4) DCT -> MFCCs
    for (int i = 0; i < g_nmfcc; ++i) {
        const float *row = &g_dct[i*g_nmel];
        float c = 0.0f;
        for (int j = 0; j < g_nmel; ++j) c += row[j] * melE[j];
        out_mfcc[i] = c;
    }
}