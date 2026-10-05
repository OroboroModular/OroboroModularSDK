// pffft's functions (include/pffft.h): the Oroboro Modular SDK's own FFT
// behind the interface Rack bundles. A radix-2 transform for powers of two,
// and Bluestein's chirp-z for any other length (on the next power of two at
// least 2N - 1), in double precision. The spectra are always ordered.

#include <pffft.h>

#include <cmath>
#include <cstdint>
#include <complex>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

typedef std::complex<double> Complex;

/// A complex transform of one power-of-two length, in place.
struct Radix2 {
	int n = 0;
	std::vector<Complex> twiddle; // e^(-2 pi i k / n)
	std::vector<int> reversed;

	explicit Radix2(int n) : n(n), twiddle(n / 2 + 1), reversed(n) {
		for (int k = 0; k <= n / 2; k++)
			twiddle[k] = std::polar(1.0, -2.0 * M_PI * k / n);
		int bits = 0;
		while ((1 << bits) < n)
			bits++;
		for (int i = 0; i < n; i++) {
			int r = 0;
			for (int b = 0; b < bits; b++)
				if (i & (1 << b))
					r |= 1 << (bits - 1 - b);
			reversed[i] = r;
		}
	}

	/// Unscaled either way.
	void transform(Complex* x, bool inverse) const {
		for (int i = 0; i < n; i++)
			if (i < reversed[i])
				std::swap(x[i], x[reversed[i]]);
		for (int len = 2; len <= n; len <<= 1) {
			int step = n / len;
			for (int i = 0; i < n; i += len) {
				for (int j = 0; j < len / 2; j++) {
					Complex w = twiddle[j * step];
					if (inverse)
						w = std::conj(w);
					Complex u = x[i + j];
					Complex v = x[i + j + len / 2] * w;
					x[i + j] = u + v;
					x[i + j + len / 2] = u - v;
				}
			}
		}
	}
};

bool isPow2(int n) { return n > 0 && (n & (n - 1)) == 0; }

/// A complex transform of any length.
struct AnyLength {
	int n;
	bool pow2;
	Radix2 direct;
	// Bluestein: x_k e^(-pi i k^2 / n), convolved with e^(pi i k^2 / n)
	int m = 0;
	Radix2* padded = nullptr;
	std::vector<Complex> chirp;      // e^(-pi i k^2 / n)
	std::vector<Complex> kernelFft;  // the transform of the conjugate chirp, wrapped
	std::vector<Complex> work;

	explicit AnyLength(int n) : n(n), pow2(isPow2(n)), direct(pow2 ? n : 1) {
		if (pow2)
			return;
		m = 1;
		while (m < 2 * n - 1)
			m <<= 1;
		padded = new Radix2(m);
		chirp.resize(n);
		for (int k = 0; k < n; k++) {
			// (k^2 mod 2n keeps the angle exact for large k)
			long long kk = ((long long) k * k) % (2LL * n);
			chirp[k] = std::polar(1.0, -M_PI * (double) kk / n);
		}
		kernelFft.assign(m, Complex(0.0));
		kernelFft[0] = std::conj(chirp[0]);
		for (int k = 1; k < n; k++)
			kernelFft[k] = kernelFft[m - k] = std::conj(chirp[k]);
		padded->transform(kernelFft.data(), false);
		work.resize(m);
	}
	~AnyLength() { delete padded; }
	AnyLength(const AnyLength&) = delete;
	AnyLength& operator=(const AnyLength&) = delete;

	/// Unscaled either way.
	void transform(Complex* x, bool inverse) {
		if (pow2) {
			direct.transform(x, inverse);
			return;
		}
		// the inverse as the forward transform of the conjugate
		std::fill(work.begin(), work.end(), Complex(0.0));
		for (int k = 0; k < n; k++)
			work[k] = (inverse ? std::conj(x[k]) : x[k]) * chirp[k];
		padded->transform(work.data(), false);
		for (int k = 0; k < m; k++)
			work[k] *= kernelFft[k];
		padded->transform(work.data(), true);
		for (int k = 0; k < n; k++) {
			Complex y = work[k] / (double) m * chirp[k];
			x[k] = inverse ? std::conj(y) : y;
		}
	}
};

} // namespace

struct PFFFT_Setup {
	int n;
	pffft_transform_t kind;
	AnyLength fft;
	std::vector<Complex> buffer;

	PFFFT_Setup(int n, pffft_transform_t kind) : n(n), kind(kind), fft(n), buffer(n) {}
};

extern "C" {

PFFFT_Setup* pffft_new_setup(int N, pffft_transform_t transform) {
	if (N <= 0)
		return nullptr;
	return new PFFFT_Setup(N, transform);
}

void pffft_destroy_setup(PFFFT_Setup* setup) { delete setup; }

void pffft_transform_ordered(PFFFT_Setup* setup, const float* input, float* output, float* work,
	pffft_direction_t direction) {
	(void) work;
	int n = setup->n;
	std::vector<Complex>& x = setup->buffer;
	bool forward = direction == PFFFT_FORWARD;
	if (setup->kind == PFFFT_COMPLEX) {
		for (int k = 0; k < n; k++)
			x[k] = Complex(input[2 * k], input[2 * k + 1]);
		setup->fft.transform(x.data(), !forward);
		for (int k = 0; k < n; k++) {
			output[2 * k] = (float) x[k].real();
			output[2 * k + 1] = (float) x[k].imag();
		}
		return;
	}
	if (forward) {
		for (int k = 0; k < n; k++)
			x[k] = Complex(input[k], 0.0);
		setup->fft.transform(x.data(), false);
		output[0] = (float) x[0].real();
		if (n > 1)
			output[1] = (float) x[n / 2].real();
		for (int k = 1; k < n / 2; k++) {
			output[2 * k] = (float) x[k].real();
			output[2 * k + 1] = (float) x[k].imag();
		}
	} else {
		x[0] = Complex(input[0], 0.0);
		if (n > 1)
			x[n / 2] = Complex(input[1], 0.0);
		for (int k = 1; k < n / 2; k++) {
			x[k] = Complex(input[2 * k], input[2 * k + 1]);
			x[n - k] = std::conj(x[k]);
		}
		setup->fft.transform(x.data(), true);
		for (int k = 0; k < n; k++)
			output[k] = (float) x[k].real();
	}
}

void pffft_transform(PFFFT_Setup* setup, const float* input, float* output, float* work, pffft_direction_t direction) {
	pffft_transform_ordered(setup, input, output, work, direction);
}

void pffft_zreorder(PFFFT_Setup* setup, const float* input, float* output, pffft_direction_t direction) {
	(void) direction;
	size_t floats = setup->kind == PFFFT_COMPLEX ? 2 * (size_t) setup->n : (size_t) setup->n;
	if (input != output)
		std::memmove(output, input, floats * sizeof(float));
}

void pffft_zconvolve_accumulate(PFFFT_Setup* setup, const float* a, const float* b, float* ab, float scaling) {
	int n = setup->n;
	int pairs = n;
	int first = 0;
	if (setup->kind == PFFFT_REAL) {
		// F(0) and F(N/2), real, in the first pair
		ab[0] += a[0] * b[0] * scaling;
		ab[1] += a[1] * b[1] * scaling;
		pairs = n / 2;
		first = 1;
	}
	for (int k = first; k < pairs; k++) {
		float ar = a[2 * k], ai = a[2 * k + 1];
		float br = b[2 * k], bi = b[2 * k + 1];
		ab[2 * k] += (ar * br - ai * bi) * scaling;
		ab[2 * k + 1] += (ar * bi + ai * br) * scaling;
	}
}

void* pffft_aligned_malloc(size_t nb_bytes) {
	// room in front for the pointer the memory really starts at
	void* raw = std::malloc(nb_bytes + 64 + sizeof(void*));
	if (!raw)
		return nullptr;
	uintptr_t start = ((uintptr_t) raw + sizeof(void*) + 63) & ~(uintptr_t) 63;
	((void**) start)[-1] = raw;
	return (void*) start;
}

void pffft_aligned_free(void* p) {
	if (p)
		std::free(((void**) p)[-1]);
}

int pffft_simd_size(void) { return 4; }

} // extern "C"
