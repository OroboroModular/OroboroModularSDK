/* pffft's interface, as Rack bundles it (Rack's FFTs, and some plugins, use
   it directly): the Oroboro Modular SDK's own code to the same functions
   (rack/src/pffft.cpp), on an FFT of its own. Any length works; the
   spectra are always in the ordered layout, so pffft_transform and
   pffft_transform_ordered do the same, and pffft_zreorder copies.
   Transforms aren't scaled: backward(forward(x)) = N * x. */
#ifndef PFFFT_H
#define PFFFT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PFFFT_Setup PFFFT_Setup;
typedef enum { PFFFT_FORWARD, PFFFT_BACKWARD } pffft_direction_t;
typedef enum { PFFFT_REAL, PFFFT_COMPLEX } pffft_transform_t;

/* A transform of N real numbers (its spectrum: F(0), F(N/2), then the
   real and imaginary parts of F(1) … F(N/2 - 1)), or of N complex ones
   (interleaved real and imaginary parts). */
PFFFT_Setup* pffft_new_setup(int N, pffft_transform_t transform);
void pffft_destroy_setup(PFFFT_Setup* setup);
/* `work` isn't needed (it may be NULL); input and output may be the same. */
void pffft_transform(PFFFT_Setup* setup, const float* input, float* output, float* work, pffft_direction_t direction);
void pffft_transform_ordered(PFFFT_Setup* setup, const float* input, float* output, float* work, pffft_direction_t direction);
void pffft_zreorder(PFFFT_Setup* setup, const float* input, float* output, pffft_direction_t direction);
/* dft_ab += dft_a * dft_b * scaling, frequency by frequency. */
void pffft_zconvolve_accumulate(PFFFT_Setup* setup, const float* dft_a, const float* dft_b, float* dft_ab, float scaling);
void* pffft_aligned_malloc(size_t nb_bytes);
void pffft_aligned_free(void* p);
int pffft_simd_size(void);

#ifdef __cplusplus
}
#endif

#endif /* PFFFT_H */
