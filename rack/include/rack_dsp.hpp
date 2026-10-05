// SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-Oroboro-Rack-Bridge-Exception (rack/LICENSE-EXCEPTION.md)
// `rack::dsp`: the helpers Rack modules reach for (triggers, pulses,
// dividers, one-pole and biquad filters, slew limiters, ring buffers, a
// real FFT, a minBLEP generator, windows, resampling). The SDK's own code, to the same
// interface as VCV Rack 2's (docs/rack-modules.md).
#pragma once

#include "rack_core.hpp"
#include "rack_simd.hpp"
#include "pffft.h"
#include <speex/speex_resampler.h>

namespace rack {
namespace dsp {

static const float FREQ_C4 = 261.6256f;
static const float FREQ_A4 = 440.0000f;
static const float FREQ_SEMITONE = 1.0594630943592953f;

inline float sinc(float x) {
	if (x == 0.f)
		return 1.f;
	x *= (float) M_PI;
	return std::sin(x) / x;
}
// (for a float, or four at once: a float_4)
template <typename T>
T quadraticBipolar(T x) { return x * simd::fabs(x); }
template <typename T>
T cubic(T x) { return x * x * x; }
template <typename T>
T quarticBipolar(T x) { return x * x * x * simd::fabs(x); }
template <typename T>
T quintic(T x) { return x * x * x * x * x; }
template <typename T>
T sqrtBipolar(T x) { return simd::sgn(x) * simd::sqrt(simd::fabs(x)); }
/// A curve through (0, 0) and (1, 1) that bends more the further `b` is from 1.
template <typename B, typename T>
T exponentialBipolar(B b, T x) { return (simd::pow(b, x) - simd::pow(b, -x)) / (b - 1.f / b); }
template <typename T>
T amplitudeToDb(T amp) { return simd::log10(amp) * 20.f; }
template <typename T>
T dbToAmplitude(T db) { return simd::pow(10.f, db / 20.f); }

template <size_t CHANNELS, typename T = float>
struct Frame {
	T samples[CHANNELS];
};

// ---- approximations ------------------------------------------------------------------------

/// 2^x, close enough for pitch (within about a cent over many octaves);
/// of a float or of a SIMD vector of them (`simd::float_4`, whose `floor`
/// and `pow` are found by the argument's type), as Rack's is.
template <typename T>
T approxExp2_taylor5(T x) {
	using std::floor;
	using std::pow;
	// whole octaves by scaling, the rest by a fifth-order polynomial
	T xi = floor(x);
	T xf = x - xi;
	const T ln2 = (T) M_LN2;
	T y = xf * ln2;
	T r = (T) 1 + y * ((T) 1 + y * ((T) 0.5 + y * ((T) (1.0 / 6) + y * ((T) (1.0 / 24) + y * (T) (1.0 / 120)))));
	return r * pow((T) 2, xi);
}
template <typename T>
T exp2_taylor5(T x) {
	return approxExp2_taylor5(x);
}

/// a[0] + a[1] x + a[2] x^2 + …, term by term.
template <typename T, size_t N>
T polyDirect(const T (&a)[N], T x) {
	T y = 0;
	T xn = 1;
	for (size_t n = 0; n < N; n++) {
		y += a[n] * xn;
		xn *= x;
	}
	return y;
}
/// The same by Horner's method.
template <typename T, size_t N>
T polyHorner(const T (&a)[N], T x) {
	if (N == 0)
		return 0;
	T y = a[N - 1];
	for (size_t n = 1; n < N; n++)
		y = a[N - 1 - n] + y * x;
	return y;
}
/// The same by Estrin's scheme (pairs of terms at once).
template <typename T, size_t N>
T polyEstrin(const T (&a)[N], T x) {
	if (N == 0)
		return 0;
	if (N == 1)
		return a[0];
	const size_t M = (N + 1) / 2;
	T b[M];
	for (size_t i = 0; i < M; i++) {
		b[i] = a[2 * i];
		if (2 * i + 1 < N)
			b[i] += a[2 * i + 1] * x;
	}
	return polyEstrin(b, x * x);
}

/// 2^floor(x) from its bits (x from -127 up), and x's fraction in `xf`.
template <typename T>
T exp2Floor(T x, T* xf);
template <>
inline float exp2Floor(float x, float* xf) {
	x += 127;
	int32_t xi = (int32_t) x;
	if (xf)
		*xf = x - xi;
	int32_t bits = xi << 23;
	float y;
	std::memcpy(&y, &bits, sizeof(y));
	return y;
}
template <>
inline simd::float_4 exp2Floor(simd::float_4 x, simd::float_4* xf) {
	simd::float_4 y;
	for (int i = 0; i < 4; i++) {
		float f;
		y.s[i] = exp2Floor(x.s[i], xf ? &f : nullptr);
		if (xf)
			xf->s[i] = f;
	}
	return y;
}
template <typename T>
T approxExp2Floor(T x, T* xf) {
	return exp2Floor(x, xf);
}

// ---- ODE solvers -----------------------------------------------------------------------------
// One step of `dt` of x' = f(t, x): `f(t, x, dxdt)`, `len` variables.

template <typename T, typename F>
void stepEuler(T t, T dt, T x[], int len, F f) {
	std::vector<T> k(len);
	f(t, x, k.data());
	for (int i = 0; i < len; i++)
		x[i] += dt * k[i];
}
template <typename T, typename F>
void stepRK2(T t, T dt, T x[], int len, F f) {
	std::vector<T> k1(len), k2(len), yi(len);
	f(t, x, k1.data());
	for (int i = 0; i < len; i++)
		yi[i] = x[i] + k1[i] * dt / T(2);
	f(t + dt / T(2), yi.data(), k2.data());
	for (int i = 0; i < len; i++)
		x[i] += dt * k2[i];
}
template <typename T, typename F>
void stepRK4(T t, T dt, T x[], int len, F f) {
	std::vector<T> k1(len), k2(len), k3(len), k4(len), yi(len);
	f(t, x, k1.data());
	for (int i = 0; i < len; i++)
		yi[i] = x[i] + k1[i] * dt / T(2);
	f(t + dt / T(2), yi.data(), k2.data());
	for (int i = 0; i < len; i++)
		yi[i] = x[i] + k2[i] * dt / T(2);
	f(t + dt / T(2), yi.data(), k3.data());
	for (int i = 0; i < len; i++)
		yi[i] = x[i] + k3[i] * dt;
	f(t + dt, yi.data(), k4.data());
	for (int i = 0; i < len; i++)
		x[i] += dt * (k1[i] + T(2) * k2[i] + T(2) * k3[i] + k4[i]) / T(6);
}

// ---- sample formats --------------------------------------------------------------------------

/// A 24-bit integer, three bytes.
struct __attribute__((packed, aligned(1)
#ifndef __clang__
	, gcc_struct
#endif
	)) Int24 {
	int32_t i : 24;
	Int24() {}
	Int24(int32_t i) : i(i) {}
	operator int32_t() { return i; }
};
static_assert(sizeof(Int24) == 3, "Int24 is three bytes");

/// A sample from one format to another (full scale is ±1 as a float).
template <typename To, typename From>
To convert(From x) = delete;
template <>
inline float convert(float x) { return x; }
template <>
inline float convert(int8_t x) { return x / 128.f; }
template <>
inline float convert(int16_t x) { return x / 32768.f; }
template <>
inline float convert(Int24 x) { return x / 8388608.f; }
template <>
inline float convert(int32_t x) { return x / 2147483648.f; }
template <>
inline float convert(int64_t x) { return x / 9223372036854775808.f; }
template <>
inline int8_t convert(float x) { return std::min(std::llround(x * 128.f), 127LL); }
template <>
inline int16_t convert(float x) { return std::min(std::llround(x * 32768.f), 32767LL); }
template <>
inline Int24 convert(float x) { return (int32_t) std::min(std::llround(x * 8388608.f), 8388607LL); }
template <>
inline int32_t convert(float x) { return std::min(std::llround(x * 2147483648.f), 2147483647LL); }
template <>
inline int64_t convert(float x) { return std::min(std::llround(x * 9223372036854775808.f), 9223372036854775807LL); }
template <typename To, typename From>
void convert(const From* in, To* out, size_t len) {
	for (size_t i = 0; i < len; i++)
		out[i] = convert<To, From>(in[i]);
}

// ---- digital -------------------------------------------------------------------------------

/// True once when its input turns true (not when it starts true).
struct BooleanTrigger {
	enum State : uint8_t { LOW, HIGH, UNINITIALIZED };
	union {
		State s = UNINITIALIZED;
		bool state;
	};

	void reset() { s = UNINITIALIZED; }
	bool process(bool in) {
		bool triggered = (s == LOW) && in;
		s = in ? HIGH : LOW;
		return triggered;
	}
	enum Event { NONE = 0, TRIGGERED = 1, UNTRIGGERED = -1 };
	/// Whether it turned true, turned false, or neither.
	Event processEvent(bool in) {
		Event event = NONE;
		if (s == LOW && in)
			event = TRIGGERED;
		else if (s == HIGH && !in)
			event = UNTRIGGERED;
		s = in ? HIGH : LOW;
		return event;
	}
	bool isHigh() { return s == HIGH; }
};

/// Of a SIMD vector (`simd::float_4`), as Rack's: each lane its own
/// trigger; `process` gives a mask, set in the lanes that triggered. A lane
/// starts high, so one already high when it starts triggers nothing.
template <typename T = float>
struct TSchmittTrigger {
	T state = T::mask();

	void reset() { state = T::mask(); }
	T process(T in, T lowThreshold = 0.f, T highThreshold = 1.f) {
		T on = (in >= highThreshold);
		T off = (in <= lowThreshold);
		T triggered = ~state & on;
		state = on | (state & ~off);
		return triggered;
	}
	T isHigh() { return state; }
};

/// True once when its input rises past the high threshold, having been at
/// or under the low one.
template <>
struct TSchmittTrigger<float> {
	enum State : uint8_t { LOW, HIGH, UNINITIALIZED };
	union {
		State s = UNINITIALIZED;
		/// (Rack 1's: whether it's high)
		bool state;
	};

	void reset() { s = UNINITIALIZED; }
	bool process(float in, float lowThreshold = 0.f, float highThreshold = 1.f) {
		return processEvent(in, lowThreshold, highThreshold) == TRIGGERED;
	}
	enum Event { NONE = 0, TRIGGERED = 1, UNTRIGGERED = -1 };
	/// Whether it went high, went low, or neither. Until it has been one or
	/// the other, its first input only says which it is.
	Event processEvent(float in, float lowThreshold = 0.f, float highThreshold = 1.f) {
		State was = s;
		if (in >= highThreshold && was != HIGH)
			s = HIGH;
		else if (in <= lowThreshold && was != LOW)
			s = LOW;
		if (was == UNINITIALIZED || s == was)
			return NONE;
		return s == HIGH ? TRIGGERED : UNTRIGGERED;
	}
	bool isHigh() { return s == HIGH; }
};
typedef TSchmittTrigger<> SchmittTrigger;

/// High for a while after it's triggered.
struct PulseGenerator {
	float remaining = 0.f;

	void reset() { remaining = 0.f; }
	bool process(float deltaTime) {
		if (remaining > 0.f) {
			remaining -= deltaTime;
			return true;
		}
		return false;
	}
	void trigger(float duration = 1e-3f) {
		if (duration > remaining)
			remaining = duration;
	}
	bool isHigh() { return remaining > 0.f; }
};

template <typename T = float>
struct TTimer {
	T time = 0.f;

	void reset() { time = 0.f; }
	T process(T deltaTime) {
		time += deltaTime;
		return time;
	}
	T getTime() { return time; }
};
typedef TTimer<> Timer;

/// True every `division`th call.
struct ClockDivider {
	uint32_t clock = 0;
	uint32_t division = 1;

	void reset() { clock = 0; }
	void setDivision(uint32_t division) { this->division = division; }
	uint32_t getDivision() { return division; }
	uint32_t getClock() { return clock; }
	bool process() {
		clock++;
		if (clock >= division) {
			clock = 0;
			return true;
		}
		return false;
	}
};

// ---- filters -------------------------------------------------------------------------------

/// A one-pole low-pass with its high-pass, by the bilinear transform.
template <typename T = float>
struct TRCFilter {
	T c = 0.f;
	T xstate[1];
	T ystate[1];

	TRCFilter() { reset(); }
	void reset() {
		xstate[0] = 0.f;
		ystate[0] = 0.f;
	}
	/// `r`: the cutoff over the sample rate.
	void setCutoff(T r) { c = 2.f / r; }
	void setCutoffFreq(T fc) { c = 1.f / simd::tan((T) (float) M_PI * fc); }
	void process(T x) {
		T y = (x + xstate[0] - ystate[0] * (1 - c)) / (1 + c);
		xstate[0] = x;
		ystate[0] = y;
	}
	T lowpass() { return ystate[0]; }
	T highpass() { return xstate[0] - ystate[0]; }
};
typedef TRCFilter<> RCFilter;

/// Moves towards its input at a rate of `lambda` a second.
template <typename T = float>
struct TExponentialFilter {
	T out = 0.f;
	T lambda = 0.f;

	void reset() { out = 0.f; }
	void setLambda(T lambda) { this->lambda = lambda; }
	void setTau(T tau) { this->lambda = 1.f / tau; }
	T process(T deltaTime, T in) {
		T y = out + (in - out) * lambda * deltaTime;
		// never past its input, however long the step
		out = simd::ifelse(in >= out, simd::fmin(y, in), simd::fmax(y, in));
		out = simd::ifelse(lambda * deltaTime < 1.f, out, in);
		return out;
	}
	T process(T in) { return process(1.f, in); }
};
typedef TExponentialFilter<> ExponentialFilter;

/// Follows peaks at once and falls from them at `lambda` a second.
template <typename T = float>
struct TPeakFilter {
	T out = 0.f;
	T lambda = 0.f;

	void reset() { out = 0.f; }
	void setLambda(T lambda) { this->lambda = lambda; }
	void setTau(T tau) { this->lambda = 1.f / tau; }
	T process(T deltaTime, T in) {
		T y = out + (in - out) * lambda * deltaTime;
		out = simd::fmax(y, in);
		return out;
	}
	T process(T in) { return process(1.f, in); }
	DEPRECATED T peak() { return out; }
	DEPRECATED void setRate(T r) { lambda = 1.f - r; }
};
typedef TPeakFilter<> PeakFilter;

/// Follows its input at most `rise` up and `fall` down a second.
template <typename T = float>
struct TSlewLimiter {
	T out = 0.f;
	T rise = 0.f;
	T fall = 0.f;

	void reset() { out = 0.f; }
	void setRiseFall(T rise, T fall) {
		this->rise = rise;
		this->fall = fall;
	}
	T process(T deltaTime, T in) {
		out = simd::fmax(simd::fmin(in, out + rise * deltaTime), out - fall * deltaTime);
		return out;
	}
	T process(T in) { return process(1.f, in); }
};
typedef TSlewLimiter<> SlewLimiter;

template <typename T = float>
struct TExponentialSlewLimiter {
	T out = 0.f;
	T riseLambda = 0.f;
	T fallLambda = 0.f;

	void reset() { out = 0.f; }
	void setRiseFall(T riseLambda, T fallLambda) {
		this->riseLambda = riseLambda;
		this->fallLambda = fallLambda;
	}
	void setRiseFallTau(T riseTau, T fallTau) {
		this->riseLambda = 1.f / riseTau;
		this->fallLambda = 1.f / fallTau;
	}
	T process(T deltaTime, T in) {
		T lambda = simd::ifelse(in > out, riseLambda, fallLambda);
		T y = out + (in - out) * lambda * deltaTime;
		// never past its input, however long the step
		out = simd::ifelse(in >= out, simd::fmin(y, in), simd::fmax(y, in));
		out = simd::ifelse(lambda * deltaTime < 1.f, out, in);
		return out;
	}
	T process(T in) { return process(1.f, in); }
};
typedef TExponentialSlewLimiter<> ExponentialSlewLimiter;

/// A filter by its coefficients: `b` over `a` (a[0] is a1).
template <int B_ORDER, int A_ORDER, typename T = float>
struct IIRFilter {
	T b[B_ORDER] = {};
	T a[A_ORDER - 1] = {};
	T x[B_ORDER - 1];
	T y[A_ORDER - 1];

	IIRFilter() { reset(); }
	void reset() {
		for (int i = 0; i < B_ORDER - 1; i++)
			x[i] = 0.f;
		for (int i = 0; i < A_ORDER - 1; i++)
			y[i] = 0.f;
	}
	void setCoefficients(const T* b, const T* a) {
		for (int i = 0; i < B_ORDER; i++)
			this->b[i] = b[i];
		for (int i = 0; i < A_ORDER - 1; i++)
			this->a[i] = a[i];
	}
	T process(T in) {
		T out = b[0] * in;
		for (int i = 1; i < B_ORDER; i++)
			out += b[i] * x[i - 1];
		for (int i = 1; i < A_ORDER; i++)
			out -= a[i - 1] * y[i - 1];
		for (int i = B_ORDER - 2; i >= 1; i--)
			x[i] = x[i - 1];
		if (B_ORDER > 1)
			x[0] = in;
		for (int i = A_ORDER - 2; i >= 1; i--)
			y[i] = y[i - 1];
		if (A_ORDER > 1)
			y[0] = out;
		return out;
	}
	/// The response at `f` (a frequency over the sample rate).
	std::complex<T> getTransferFunction(T f) {
		std::complex<T> bSum(b[0], 0), aSum(1, 0);
		for (int i = 1; i < std::max(B_ORDER, A_ORDER); i++) {
			T p = -2 * (T) M_PI * i * f;
			std::complex<T> z(std::cos(p), std::sin(p));
			if (i < B_ORDER)
				bSum += b[i] * z;
			if (i < A_ORDER)
				aSum += a[i - 1] * z;
		}
		return bSum / aSum;
	}
	T getFrequencyResponse(T f) { return std::abs(getTransferFunction(f)); }
	T getFrequencyPhase(T f) { return std::arg(getTransferFunction(f)); }
};

/// The usual second-order filters, by kind, frequency (over the sample
/// rate), Q and gain.
template <typename T = float>
struct TBiquadFilter : IIRFilter<3, 3, T> {
	enum Type {
		LOWPASS_1POLE,
		HIGHPASS_1POLE,
		LOWPASS,
		HIGHPASS,
		LOWSHELF,
		HIGHSHELF,
		BANDPASS,
		PEAK,
		NOTCH,
		NUM_TYPES
	};

	TBiquadFilter() { setParameters(LOWPASS, 0.f, 0.f, 1.f); }

	/// `f`: the frequency over the sample rate; `V`: the gain, for shelves and peaks.
	void setParameters(Type type, float f, float Q, float V) {
		float K = std::tan((float) M_PI * f);
		switch (type) {
			case LOWPASS_1POLE: {
				this->a[0] = -std::exp(-2.f * (float) M_PI * f);
				this->a[1] = 0.f;
				this->b[0] = 1.f + this->a[0];
				this->b[1] = 0.f;
				this->b[2] = 0.f;
			} break;
			case HIGHPASS_1POLE: {
				this->a[0] = std::exp(-2.f * (float) M_PI * (0.5f - f));
				this->a[1] = 0.f;
				this->b[0] = 1.f - this->a[0];
				this->b[1] = 0.f;
				this->b[2] = 0.f;
			} break;
			case LOWPASS: {
				float norm = 1.f / (1.f + K / Q + K * K);
				this->b[0] = K * K * norm;
				this->b[1] = 2.f * this->b[0];
				this->b[2] = this->b[0];
				this->a[0] = 2.f * (K * K - 1.f) * norm;
				this->a[1] = (1.f - K / Q + K * K) * norm;
			} break;
			case HIGHPASS: {
				float norm = 1.f / (1.f + K / Q + K * K);
				this->b[0] = norm;
				this->b[1] = -2.f * this->b[0];
				this->b[2] = this->b[0];
				this->a[0] = 2.f * (K * K - 1.f) * norm;
				this->a[1] = (1.f - K / Q + K * K) * norm;
			} break;
			case LOWSHELF: {
				float sqrtV = std::sqrt(V);
				if (V >= 1.f) {
					float norm = 1.f / (1.f + (float) M_SQRT2 * K + K * K);
					this->b[0] = (1.f + (float) M_SQRT2 * sqrtV * K + V * K * K) * norm;
					this->b[1] = 2.f * (V * K * K - 1.f) * norm;
					this->b[2] = (1.f - (float) M_SQRT2 * sqrtV * K + V * K * K) * norm;
					this->a[0] = 2.f * (K * K - 1.f) * norm;
					this->a[1] = (1.f - (float) M_SQRT2 * K + K * K) * norm;
				}
				else {
					float norm = 1.f / (1.f + (float) M_SQRT2 / sqrtV * K + K * K / V);
					this->b[0] = (1.f + (float) M_SQRT2 * K + K * K) * norm;
					this->b[1] = 2.f * (K * K - 1) * norm;
					this->b[2] = (1.f - (float) M_SQRT2 * K + K * K) * norm;
					this->a[0] = 2.f * (K * K / V - 1.f) * norm;
					this->a[1] = (1.f - (float) M_SQRT2 / sqrtV * K + K * K / V) * norm;
				}
			} break;
			case HIGHSHELF: {
				float sqrtV = std::sqrt(V);
				if (V >= 1.f) {
					float norm = 1.f / (1.f + (float) M_SQRT2 * K + K * K);
					this->b[0] = (V + (float) M_SQRT2 * sqrtV * K + K * K) * norm;
					this->b[1] = 2.f * (K * K - V) * norm;
					this->b[2] = (V - (float) M_SQRT2 * sqrtV * K + K * K) * norm;
					this->a[0] = 2.f * (K * K - 1.f) * norm;
					this->a[1] = (1.f - (float) M_SQRT2 * K + K * K) * norm;
				}
				else {
					float norm = 1.f / (1.f / V + (float) M_SQRT2 / sqrtV * K + K * K);
					this->b[0] = (1.f + (float) M_SQRT2 * K + K * K) * norm;
					this->b[1] = 2.f * (K * K - 1.f) * norm;
					this->b[2] = (1.f - (float) M_SQRT2 * K + K * K) * norm;
					this->a[0] = 2.f * (K * K - 1.f / V) * norm;
					this->a[1] = (1.f / V - (float) M_SQRT2 / sqrtV * K + K * K) * norm;
				}
			} break;
			case BANDPASS: {
				float norm = 1.f / (1.f + K / Q + K * K);
				this->b[0] = K / Q * norm;
				this->b[1] = 0.f;
				this->b[2] = -this->b[0];
				this->a[0] = 2.f * (K * K - 1.f) * norm;
				this->a[1] = (1.f - K / Q + K * K) * norm;
			} break;
			case PEAK: {
				if (V >= 1.f) {
					float norm = 1.f / (1.f + K / Q + K * K);
					this->b[0] = (1.f + K / Q * V + K * K) * norm;
					this->b[1] = 2.f * (K * K - 1.f) * norm;
					this->b[2] = (1.f - K / Q * V + K * K) * norm;
					this->a[0] = this->b[1];
					this->a[1] = (1.f - K / Q + K * K) * norm;
				}
				else {
					float norm = 1.f / (1.f + K / Q / V + K * K);
					this->b[0] = (1.f + K / Q + K * K) * norm;
					this->b[1] = 2.f * (K * K - 1.f) * norm;
					this->b[2] = (1.f - K / Q + K * K) * norm;
					this->a[0] = this->b[1];
					this->a[1] = (1.f - K / Q / V + K * K) * norm;
				}
			} break;
			case NOTCH: {
				float norm = 1.f / (1.f + K / Q + K * K);
				this->b[0] = (1.f + K * K) * norm;
				this->b[1] = 2.f * (K * K - 1.f) * norm;
				this->b[2] = this->b[0];
				this->a[0] = this->b[1];
				this->a[1] = (1.f - K / Q + K * K) * norm;
			} break;
			default: break;
		}
	}
};
typedef TBiquadFilter<> BiquadFilter;

// ---- ring buffers --------------------------------------------------------------------------

/// A queue of `S` items (a power of two), one thread pushing and one shifting.
template <typename T, size_t S>
struct RingBuffer {
	std::atomic<size_t> start{0};
	std::atomic<size_t> end{0};
	T data[S];

	void push(T t) {
		size_t i = end % S;
		data[i] = t;
		end++;
	}
	void pushBuffer(const T* t, int n) {
		for (int i = 0; i < n; i++)
			push(t[i]);
	}
	T shift() {
		size_t i = start % S;
		T t = data[i];
		start++;
		return t;
	}
	void shiftBuffer(T* t, size_t n) {
		for (size_t i = 0; i < n; i++)
			t[i] = shift();
	}
	void clear() { start = end.load(); }
	bool empty() const { return start >= end; }
	bool full() const { return end - start >= S; }
	size_t size() const { return end - start; }
	size_t capacity() const { return S - size(); }
};

/// A queue whose contents are always one contiguous run (it keeps `S`
/// spare items after the first `S`, and moves down when it runs out).
template <typename T, size_t S>
struct DoubleRingBuffer {
	std::atomic<size_t> start{0};
	std::atomic<size_t> end{0};
	T data[2 * S];

	void push(T t) {
		size_t i = end % S;
		data[i] = t;
		data[i + S] = t;
		end++;
	}
	T shift() {
		size_t i = start % S;
		T t = data[i];
		start++;
		return t;
	}
	void clear() { start = end.load(); }
	bool empty() const { return start >= end; }
	bool full() const { return end - start >= S; }
	size_t size() const { return end - start; }
	size_t capacity() const { return S - size(); }
	/// Where to write `n` items in a row, then `endIncr(n)`.
	T* endData() { return &data[end % S]; }
	void endIncr(size_t n) {
		size_t e = end % S;
		size_t e1 = e + n;
		size_t e2 = (e1 < S) ? e1 : S;
		// what was written, mirrored into the other half
		std::memcpy(&data[S + e], &data[e], sizeof(T) * (e2 - e));
		if (e1 > S)
			std::memcpy(data, &data[S], sizeof(T) * (e1 - S));
		end += n;
	}
	/// Where to read `n` items in a row, then `startIncr(n)`.
	const T* startData() const { return &data[start % S]; }
	void startIncr(size_t n) { start += n; }
};

// ---- VU meters -----------------------------------------------------------------------------

/// Lights by decibels: light 0 is the clip light, the others each
/// `dBInterval` lower.
struct VuMeter {
	float dBInterval = 3.0;
	float dBScaled;
	void setValue(float v) { dBScaled = std::log10(std::fabs(v)) * 20.0 / dBInterval; }
	float getBrightness(int i) {
		if (i == 0)
			return (dBScaled >= 0.0) ? 1.0 : 0.0;
		return math::clamp(dBScaled + i, 0.f, 1.f);
	}
};
DEPRECATED typedef VuMeter VUMeter;

/// A level meter, of peaks or of the RMS, falling at `lambda` a second.
struct VuMeter2 {
	enum Mode { PEAK, RMS };
	Mode mode = PEAK;
	float v = 0.f;
	float lambda = 30.f;

	void reset() { v = 0.f; }
	void process(float deltaTime, float value) {
		if (mode == RMS) {
			value = value * value;
			v += (value - v) * lambda * deltaTime;
		} else {
			value = std::fabs(value);
			if (value >= v)
				v = value;
			else
				v += (value - v) * lambda * deltaTime;
		}
	}
	/// 0 at dbMin and under, 1 at dbMax and over.
	float getBrightness(float dbMin, float dbMax) {
		float db = amplitudeToDb((mode == RMS) ? std::sqrt(v) : v);
		if (db >= dbMax)
			return 1.f;
		if (db <= dbMin)
			return 0.f;
		return math::rescale(db, dbMin, dbMax, 0.f, 1.f);
	}
};

// ---- windows -------------------------------------------------------------------------------

inline float hann(float p) { return 0.5f * (1.f - std::cos(2 * (float) M_PI * p)); }
inline void hannWindow(float* x, int len) {
	for (int i = 0; i < len; i++)
		x[i] *= hann((float) i / (len - 1));
}
inline float blackman(float alpha, float p) {
	return (1 - alpha) / 2.f - 1 / 2.f * std::cos(2 * (float) M_PI * p) + alpha / 2.f * std::cos(4 * (float) M_PI * p);
}
inline void blackmanWindow(float alpha, float* x, int len) {
	for (int i = 0; i < len; i++)
		x[i] *= blackman(alpha, (float) i / (len - 1));
}
inline float blackmanNuttall(float p) {
	return 0.3635819f - 0.4891775f * std::cos(2 * (float) M_PI * p) + 0.1365995f * std::cos(4 * (float) M_PI * p)
		- 0.0106411f * std::cos(6 * (float) M_PI * p);
}
inline void blackmanNuttallWindow(float* x, int len) {
	for (int i = 0; i < len; i++)
		x[i] *= blackmanNuttall((float) i / (len - 1));
}
inline float blackmanHarris(float p) {
	return 0.35875f - 0.48829f * std::cos(2 * (float) M_PI * p) + 0.14128f * std::cos(4 * (float) M_PI * p)
		- 0.01168f * std::cos(6 * (float) M_PI * p);
}
inline void blackmanHarrisWindow(float* x, int len) {
	for (int i = 0; i < len; i++)
		x[i] *= blackmanHarris((float) i / (len - 1));
}

// ---- resampling ----------------------------------------------------------------------------

/// A low-pass filter's impulse response, `len` taps: the sinc of an ideal
/// low-pass cutting off at `cutoff` (a fraction of the sample rate),
/// centred on the middle tap, not windowed.
inline void boxcarLowpassIR(float* out, int len, float cutoff = 0.5f) {
	for (int i = 0; i < len; i++) {
		float t = i - (len - 1) / 2.f;
		out[i] = 2 * cutoff * sinc(2 * cutoff * t);
	}
}

/// A windowed (Blackman-Harris) low-pass kernel of `len` taps cutting off at
/// `cutoff` (a fraction of the sample rate), its taps summing to `gain`.
inline void lowpassKernel(float* kernel, int len, float cutoff, float gain = 1.f) {
	boxcarLowpassIR(kernel, len, cutoff);
	blackmanHarrisWindow(kernel, len);
	float sum = 0.f;
	for (int i = 0; i < len; i++)
		sum += kernel[i];
	for (int i = 0; i < len; i++)
		kernel[i] *= gain / sum;
}

/// Takes a signal at OVERSAMPLE times the rate back down: each `process`
/// takes OVERSAMPLE samples and gives one, low-passed first (QUALITY taps
/// for each output sample; `cutoff` a fraction of the output's Nyquist).
template <int OVERSAMPLE, int QUALITY, typename T = float>
struct Decimator {
	static const int LENGTH = OVERSAMPLE * QUALITY;
	T inBuffer[LENGTH];
	float kernel[LENGTH];
	int inIndex = 0;

	Decimator(float cutoff = 0.9f) {
		lowpassKernel(kernel, LENGTH, cutoff * 0.5f / OVERSAMPLE);
		reset();
	}
	void reset() {
		inIndex = 0;
		for (int i = 0; i < LENGTH; i++)
			inBuffer[i] = T(0);
	}
	/// One sample from the next OVERSAMPLE of the oversampled signal.
	T process(T* in) {
		for (int i = 0; i < OVERSAMPLE; i++) {
			inBuffer[inIndex] = in[i];
			inIndex = (inIndex + 1) % LENGTH;
		}
		T out = T(0);
		for (int i = 0; i < LENGTH; i++)
			out += inBuffer[(inIndex + i) % LENGTH] * kernel[i];
		return out;
	}
};

/// Takes a signal up to OVERSAMPLE times its rate: each `process` takes
/// one sample and gives OVERSAMPLE, filtered between them (QUALITY taps a
/// phase; `cutoff` a fraction of the input's Nyquist).
template <int OVERSAMPLE, int QUALITY, typename T = float>
struct Upsampler {
	T inBuffer[QUALITY];
	float kernel[OVERSAMPLE * QUALITY];
	int inIndex = 0;

	Upsampler(float cutoff = 0.9f) {
		// (each input stands for OVERSAMPLE samples, so the filter's gain is that)
		lowpassKernel(kernel, OVERSAMPLE * QUALITY, cutoff * 0.5f / OVERSAMPLE, (float) OVERSAMPLE);
		reset();
	}
	void reset() {
		inIndex = 0;
		for (int i = 0; i < QUALITY; i++)
			inBuffer[i] = T(0);
	}
	/// OVERSAMPLE samples into `out` for the next sample `in`.
	void process(T in, T* out) {
		inBuffer[inIndex] = in;
		inIndex = (inIndex + 1) % QUALITY;
		for (int k = 0; k < OVERSAMPLE; k++) {
			T y = T(0);
			// the input j samples back is at the oversampled tap j * OVERSAMPLE + k
			for (int j = 0; j < QUALITY; j++)
				y += inBuffer[(inIndex - 1 - j + 2 * QUALITY) % QUALITY] * kernel[j * OVERSAMPLE + k];
			out[k] = y;
		}
	}
};

/// Converts frames of CHANNELS channels from one sample rate to another, any
/// two: a windowed-sinc filter, low-passed below the lower rate's Nyquist,
/// read between its taps. `process` takes what input it needs for the
/// output it has room for, and says how much of each it took and gave
/// (the rest of the input is for the next call). Same rates: a copy.
/// `setQuality` (0 to 10) sets the filter's length: longer is cleaner and
/// later (16 to 36 input samples, half of it the latency).
template <int CHANNELS>
struct SampleRateConverter {
	static const int MOST_HALF = 18;
	static const int RING = 64;
	static const int TABLE_STEPS = 256;
	int channels = CHANNELS;
	int quality = 4;
	int inRate = 44100;
	int outRate = 44100;
	// the filter, half of it (it's symmetric), TABLE_STEPS points an input sample
	std::vector<float> table;
	int half = 16;
	double step = 1.0;
	// the inputs taken, the latest RING of them, and when the next output is
	long long taken = 0;
	double next = 0.0;
	float ring[RING][CHANNELS] = {};

	SampleRateConverter() { refreshState(); }

	void setChannels(int channels) {
		this->channels = std::max(1, std::min(channels, CHANNELS));
	}
	void setQuality(int quality) {
		this->quality = std::max(0, std::min(quality, 10));
		refreshState();
	}
	void setRates(int inRate, int outRate) {
		this->inRate = std::max(1, inRate);
		this->outRate = std::max(1, outRate);
		refreshState();
	}
	void refreshState() {
		half = 8 + quality;
		step = (double) inRate / outRate;
		// the cutoff, in cycles an input sample: below the lower Nyquist
		float cutoff = 0.5f * std::min(1.f, (float) outRate / inRate) * 0.95f;
		table.assign(half * TABLE_STEPS + 2, 0.f);
		for (int i = 0; i <= half * TABLE_STEPS; i++) {
			float x = (float) i / TABLE_STEPS;
			table[i] = 2 * cutoff * sinc(2 * cutoff * x) * blackmanHarris(0.5f + 0.5f * x / half);
		}
		taken = 0;
		next = 0.0;
		for (int i = 0; i < RING; i++)
			for (int c = 0; c < CHANNELS; c++)
				ring[i][c] = 0.f;
	}

	/// Frames of `in` (each `inStride` floats apart) into `out` (each
	/// `outStride` apart): `*inFrames` and `*outFrames` say how many there
	/// are and room for, and come back as how many were taken and given.
	void process(const float* in, int inStride, int* inFrames, float* out, int outStride, int* outFrames) {
		if (inRate == outRate) {
			int n = std::min(*inFrames, *outFrames);
			for (int i = 0; i < n; i++)
				for (int c = 0; c < channels; c++)
					out[i * outStride + c] = in[i * inStride + c];
			*inFrames = *outFrames = n;
			return;
		}
		int took = 0, gave = 0;
		while (gave < *outFrames) {
			long long centre = (long long) std::floor(next);
			// the inputs it needs: up to `half` past the time of the output
			while (taken <= centre + half && took < *inFrames) {
				float* to = ring[taken % RING];
				for (int c = 0; c < channels; c++)
					to[c] = in[took * inStride + c];
				taken++;
				took++;
			}
			if (taken <= centre + half)
				break;
			float sum[CHANNELS] = {};
			for (long long n = centre - half + 1; n <= centre + half; n++) {
				if (n < 0)
					continue;
				float at = (float) std::fabs(next - (double) n) * TABLE_STEPS;
				int i = (int) at;
				if (i >= half * TABLE_STEPS)
					continue;
				float w = table[i] + (table[i + 1] - table[i]) * (at - i);
				const float* from = ring[n % RING];
				for (int c = 0; c < channels; c++)
					sum[c] += w * from[c];
			}
			for (int c = 0; c < channels; c++)
				out[gave * outStride + c] = sum[c];
			gave++;
			next += step;
		}
		*inFrames = took;
		*outFrames = gave;
	}
	void process(const Frame<CHANNELS>* in, int* inFrames, Frame<CHANNELS>* out, int* outFrames) {
		process((const float*) in, CHANNELS, inFrames, (float*) out, CHANNELS, outFrames);
	}
};

// ---- the real FFT --------------------------------------------------------------------------

/// Memory that starts on a 32-byte boundary, and its release.
void* alignedAllocate(size_t bytes);
void alignedRelease(void* p);
template <typename T>
T* alignedNew(size_t length) {
	return (T*) alignedAllocate(length * sizeof(T));
}
template <typename T>
void alignedDelete(T* p) {
	alignedRelease(p);
}

/// The FFT of `length` real samples. Its spectrum is `length` floats:
/// `[0]` the DC bin, `[1]` the Nyquist bin (both real), then each bin k of
/// 1 to length/2 − 1 as `[2k]` real and `[2k + 1]` imaginary. `irfft`
/// takes that back to `length` samples, times `length`.
struct RealFFT {
	int length;
	struct Plan;
	Plan* plan;

	RealFFT(size_t length);
	~RealFFT();
	RealFFT(const RealFFT&) = delete;
	RealFFT& operator=(const RealFFT&) = delete;

	void rfft(const float* input, float* output);
	void irfft(const float* input, float* output);
	/// The same here: this FFT has one order.
	void rfftUnordered(const float* input, float* output) { rfft(input, output); }
	void irfftUnordered(const float* input, float* output) { irfft(input, output); }
	/// Scales by 1/length, so that `irfft(rfft(x))` then `scale` gives `x`.
	void scale(float* x) {
		float a = 1.f / length;
		for (int i = 0; i < length; i++)
			x[i] *= a;
	}
};

/// The FFT of `length` complex numbers (interleaved real and imaginary
/// parts); `ifft(fft(x))` is `length` times x.
struct ComplexFFT {
	PFFFT_Setup* setup;
	int length;

	ComplexFFT(size_t length) : setup(pffft_new_setup((int) length, PFFFT_COMPLEX)), length((int) length) {}
	~ComplexFFT() { pffft_destroy_setup(setup); }
	ComplexFFT(const ComplexFFT&) = delete;
	ComplexFFT& operator=(const ComplexFFT&) = delete;

	void fftUnordered(const float* input, float* output) { pffft_transform(setup, input, output, NULL, PFFFT_FORWARD); }
	void ifftUnordered(const float* input, float* output) { pffft_transform(setup, input, output, NULL, PFFFT_BACKWARD); }
	void fft(const float* input, float* output) { pffft_transform_ordered(setup, input, output, NULL, PFFFT_FORWARD); }
	void ifft(const float* input, float* output) { pffft_transform_ordered(setup, input, output, NULL, PFFFT_BACKWARD); }
	void scale(float* x) {
		float a = 1.f / length;
		for (int i = 0; i < 2 * length; i++)
			x[i] *= a;
	}
};

/// `in`'s last `len` samples (the newest last) through `kernel`.
inline float convolveNaive(const float* in, const float* kernel, int len) {
	float y = 0.f;
	for (int i = 0; i < len; i++)
		y += in[len - 1 - i] * kernel[i];
	return y;
}

/// Convolution by blocks of `blockSize`, a block's latency: the kernel in
/// blocks of the same size, each block's spectrum times the input's of as
/// many blocks ago, added up (overlap-add).
struct RealTimeConvolver {
	float* kernelFfts = NULL;
	float* inputFfts = NULL;
	float* outputTail = NULL;
	float* tmpBlock = NULL;
	size_t blockSize;
	size_t kernelBlocks = 0;
	size_t inputPos = 0;
	PFFFT_Setup* pffft;

	RealTimeConvolver(size_t blockSize) : blockSize(blockSize) {
		pffft = pffft_new_setup((int) (blockSize * 2), PFFFT_REAL);
		outputTail = new float[blockSize]();
		tmpBlock = new float[blockSize * 2]();
	}
	~RealTimeConvolver() {
		setKernel(NULL, 0);
		delete[] outputTail;
		delete[] tmpBlock;
		pffft_destroy_setup(pffft);
	}
	RealTimeConvolver(const RealTimeConvolver&) = delete;
	RealTimeConvolver& operator=(const RealTimeConvolver&) = delete;

	void setKernel(const float* kernel, size_t length) {
		pffft_aligned_free(kernelFfts);
		pffft_aligned_free(inputFfts);
		kernelFfts = inputFfts = NULL;
		kernelBlocks = 0;
		inputPos = 0;
		if (!kernel || length == 0)
			return;
		kernelBlocks = (length - 1) / blockSize + 1;
		size_t floats = blockSize * 2 * kernelBlocks;
		kernelFfts = (float*) pffft_aligned_malloc(sizeof(float) * floats);
		inputFfts = (float*) pffft_aligned_malloc(sizeof(float) * floats);
		std::memset(inputFfts, 0, sizeof(float) * floats);
		for (size_t b = 0; b < kernelBlocks; b++) {
			std::memset(tmpBlock, 0, sizeof(float) * blockSize * 2);
			size_t n = std::min(blockSize, length - b * blockSize);
			std::memcpy(tmpBlock, &kernel[b * blockSize], sizeof(float) * n);
			pffft_transform(pffft, tmpBlock, &kernelFfts[blockSize * 2 * b], NULL, PFFFT_FORWARD);
		}
	}
	/// `blockSize` samples in, as many out.
	void processBlock(const float* input, float* output) {
		if (kernelBlocks == 0) {
			std::memset(output, 0, sizeof(float) * blockSize);
			return;
		}
		std::memset(tmpBlock, 0, sizeof(float) * blockSize * 2);
		std::memcpy(tmpBlock, input, sizeof(float) * blockSize);
		pffft_transform(pffft, tmpBlock, &inputFfts[blockSize * 2 * inputPos], NULL, PFFFT_FORWARD);
		std::memset(tmpBlock, 0, sizeof(float) * blockSize * 2);
		for (size_t b = 0; b < kernelBlocks; b++) {
			size_t pos = (inputPos + kernelBlocks - b) % kernelBlocks;
			pffft_zconvolve_accumulate(pffft, &kernelFfts[blockSize * 2 * b], &inputFfts[blockSize * 2 * pos], tmpBlock, 1.f);
		}
		pffft_transform(pffft, tmpBlock, tmpBlock, NULL, PFFFT_BACKWARD);
		float scale = 1.f / (blockSize * 2);
		for (size_t i = 0; i < blockSize; i++) {
			output[i] = (tmpBlock[i] + outputTail[i]) * scale;
			outputTail[i] = tmpBlock[blockSize + i];
		}
		inputPos = (inputPos + 1) % kernelBlocks;
	}
};

/// A buffer of at most `S` items in an array of `N` (N > S), its items
/// moved back to the start when the array's end is reached.
template <typename T, size_t S, size_t N>
struct AppleRingBuffer {
	size_t start = 0;
	size_t end = 0;
	T data[N];

	void returnBuffer() {
		size_t s = size();
		std::memmove(data, &data[start], sizeof(T) * s);
		start = 0;
		end = s;
	}
	void push(T t) {
		if (end + 1 > N)
			returnBuffer();
		data[end++] = t;
	}
	T shift() { return data[start++]; }
	bool empty() const { return start == end; }
	bool full() const { return end - start == S; }
	size_t size() const { return end - start; }
	size_t capacity() const { return S - size(); }
	/// Room for `n` more at the end; `endIncr(n)` once written.
	T* endData(size_t n) {
		if (end + n > N)
			returnBuffer();
		return &data[end];
	}
	void endIncr(size_t n) { end += n; }
	const T* startData() const { return &data[start]; }
	void startIncr(size_t n) { start += n; }
};

// ---- minBLEP -------------------------------------------------------------------------------

/// A band-limited step's residue, `z` zero crossings each side at `o`
/// times the sample rate, as `2 * z * o` samples running from −1 to 0.
void minBlepImpulse(int z, int o, float* output);

/// Adds band-limited steps to an oscillator's output: `insertDiscontinuity`
/// at each jump (how far before now it happened, and how big), `process`
/// each sample for what to add.
template <int Z, int O, typename T = float>
struct MinBlepGenerator {
	T buf[2 * Z] = {};
	int pos = 0;
	float impulse[2 * Z * O + 1];

	MinBlepGenerator() {
		minBlepImpulse(Z, O, impulse);
		impulse[2 * Z * O] = 1.f;
	}
	/// `p` in (−1, 0]: when the jump of `x` happened, in samples before now.
	void insertDiscontinuity(float p, T x) {
		if (!(-1 < p && p <= 0))
			return;
		for (int j = 0; j < 2 * Z; j++) {
			float minBlepIndex = ((float) j - p) * O;
			int index = (pos + j) % (2 * Z);
			int i = (int) minBlepIndex;
			float f = minBlepIndex - i;
			float v = impulse[i] + (impulse[std::min(i + 1, 2 * Z * O)] - impulse[i]) * f;
			buf[index] += x * (-1.f + v);
		}
	}
	T process() {
		T v = buf[pos];
		buf[pos] = T(0);
		pos = (pos + 1) % (2 * Z);
		return v;
	}
};

} // namespace dsp
} // namespace rack
