// SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-Oroboro-Rack-Bridge-Exception (rack/LICENSE-EXCEPTION.md)
// `rack::simd`: four floats at once, as Rack modules that play sixteen
// channels use them. Plain C++ here (the compiler vectorises what it can):
// the same values, without tying a module's source to one processor. The
// SDK's own code, to the same interface as VCV Rack 2's.
#pragma once

#include <complex>

#include "rack_core.hpp"

namespace rack {
namespace simd {

template <typename T, int N>
struct Vector;
template <>
struct Vector<int32_t, 4>;

/// Four floats.
template <>
struct Vector<float, 4> {
	typedef float type;
	static constexpr int size = 4;
	float s[4];

	Vector() = default;
	Vector(float x) { s[0] = s[1] = s[2] = s[3] = x; }
	Vector(float x1, float x2, float x3, float x4) {
		s[0] = x1;
		s[1] = x2;
		s[2] = x3;
		s[3] = x4;
	}
	/// Four whole numbers' values (made below).
	Vector(Vector<int32_t, 4> a);
	static Vector zero() { return Vector(0.f); }
	/// Every bit set: what a comparison gives for "true".
	static Vector mask() {
		Vector v;
		uint32_t all = 0xffffffffu;
		for (int i = 0; i < 4; i++)
			std::memcpy(&v.s[i], &all, 4);
		return v;
	}
	static Vector load(const float* x) {
		Vector v;
		std::memcpy(v.s, x, sizeof(v.s));
		return v;
	}
	void store(float* x) const { std::memcpy(x, s, sizeof(s)); }
	float& operator[](int i) { return s[i]; }
	const float& operator[](int i) const { return s[i]; }
};
typedef Vector<float, 4> float_4;

/// Four 32-bit integers.
template <>
struct Vector<int32_t, 4> {
	typedef int32_t type;
	static constexpr int size = 4;
	int32_t s[4];

	Vector() = default;
	Vector(int32_t x) { s[0] = s[1] = s[2] = s[3] = x; }
	Vector(int32_t x1, int32_t x2, int32_t x3, int32_t x4) {
		s[0] = x1;
		s[1] = x2;
		s[2] = x3;
		s[3] = x4;
	}
	static Vector zero() { return Vector(0); }
	static Vector mask() { return Vector(-1); }
	static Vector load(const int32_t* x) {
		Vector v;
		std::memcpy(v.s, x, sizeof(v.s));
		return v;
	}
	void store(int32_t* x) const { std::memcpy(x, s, sizeof(s)); }
	int32_t& operator[](int i) { return s[i]; }
	const int32_t& operator[](int i) const { return s[i]; }
	Vector(float_4 a) {
		for (int i = 0; i < 4; i++)
			s[i] = (int32_t) a.s[i];
	}
	static Vector cast(float_4 a) {
		Vector v;
		std::memcpy(v.s, a.s, sizeof(v.s));
		return v;
	}
};
typedef Vector<int32_t, 4> int32_4;

inline Vector<float, 4>::Vector(int32_4 a) {
	for (int i = 0; i < 4; i++)
		s[i] = (float) a.s[i];
}

namespace detail {
inline uint32_t bits(float x) {
	uint32_t u;
	std::memcpy(&u, &x, 4);
	return u;
}
inline float unbits(uint32_t u) {
	float x;
	std::memcpy(&x, &u, 4);
	return x;
}
inline float truth(bool b) { return unbits(b ? 0xffffffffu : 0u); }
} // namespace detail

// (as Rack's: a vector with a vector only, so a number beside one is made
// a vector of it, and an int32_4 beside a whole number stays an int32_4)
#define ORO_SIMD_ARITH(op) \
	inline float_4 operator op(const float_4& a, const float_4& b) { \
		float_4 r; \
		for (int i = 0; i < 4; i++) \
			r.s[i] = a.s[i] op b.s[i]; \
		return r; \
	} \
	inline float_4& operator op##=(float_4& a, const float_4& b) { return a = a op b; }
ORO_SIMD_ARITH(+)
ORO_SIMD_ARITH(-)
ORO_SIMD_ARITH(*)
ORO_SIMD_ARITH(/)
#undef ORO_SIMD_ARITH

#define ORO_SIMD_COMPARE(op) \
	inline float_4 operator op(const float_4& a, const float_4& b) { \
		float_4 r; \
		for (int i = 0; i < 4; i++) \
			r.s[i] = detail::truth(a.s[i] op b.s[i]); \
		return r; \
	}
ORO_SIMD_COMPARE(==)
ORO_SIMD_COMPARE(!=)
ORO_SIMD_COMPARE(<)
ORO_SIMD_COMPARE(<=)
ORO_SIMD_COMPARE(>)
ORO_SIMD_COMPARE(>=)
#undef ORO_SIMD_COMPARE

#define ORO_SIMD_BITS(op) \
	inline float_4 operator op(const float_4& a, const float_4& b) { \
		float_4 r; \
		for (int i = 0; i < 4; i++) \
			r.s[i] = detail::unbits(detail::bits(a.s[i]) op detail::bits(b.s[i])); \
		return r; \
	} \
	inline float_4& operator op##=(float_4& a, const float_4& b) { return a = a op b; }
ORO_SIMD_BITS(&)
ORO_SIMD_BITS(|)
ORO_SIMD_BITS(^)
#undef ORO_SIMD_BITS

inline float_4 operator+(const float_4& a) { return a; }
inline float_4 operator-(const float_4& a) { return float_4(0.f) - a; }
inline float_4 operator~(const float_4& a) {
	float_4 r;
	for (int i = 0; i < 4; i++)
		r.s[i] = detail::unbits(~detail::bits(a.s[i]));
	return r;
}
inline float_4& operator++(float_4& a) { return a += float_4(1.f); }
inline float_4& operator--(float_4& a) { return a -= float_4(1.f); }
inline float_4 operator++(float_4& a, int) {
	float_4 b = a;
	++a;
	return b;
}
inline float_4 operator--(float_4& a, int) {
	float_4 b = a;
	--a;
	return b;
}

#define ORO_SIMD_INT(op) \
	inline int32_4 operator op(const int32_4& a, const int32_4& b) { \
		int32_4 r; \
		for (int i = 0; i < 4; i++) \
			r.s[i] = a.s[i] op b.s[i]; \
		return r; \
	} \
	inline int32_4& operator op##=(int32_4& a, const int32_4& b) { return a = a op b; }
ORO_SIMD_INT(+)
ORO_SIMD_INT(-)
ORO_SIMD_INT(*)
ORO_SIMD_INT(&)
ORO_SIMD_INT(|)
ORO_SIMD_INT(^)
#undef ORO_SIMD_INT
#define ORO_SIMD_INT_COMPARE(op) \
	inline int32_4 operator op(const int32_4& a, const int32_4& b) { \
		int32_4 r; \
		for (int i = 0; i < 4; i++) \
			r.s[i] = (a.s[i] op b.s[i]) ? -1 : 0; \
		return r; \
	}
ORO_SIMD_INT_COMPARE(==)
ORO_SIMD_INT_COMPARE(!=)
ORO_SIMD_INT_COMPARE(<)
ORO_SIMD_INT_COMPARE(<=)
ORO_SIMD_INT_COMPARE(>)
ORO_SIMD_INT_COMPARE(>=)
#undef ORO_SIMD_INT_COMPARE
inline int32_4 operator-(const int32_4& a) { return int32_4(0) - a; }
inline int32_4 operator~(const int32_4& a) { return a ^ int32_4(-1); }
inline int32_4 operator<<(const int32_4& a, int b) {
	int32_4 r;
	for (int i = 0; i < 4; i++)
		r.s[i] = (int32_t)((uint32_t) a.s[i] << b);
	return r;
}
inline int32_4 operator>>(const int32_4& a, int b) {
	int32_4 r;
	for (int i = 0; i < 4; i++)
		r.s[i] = a.s[i] >> b;
	return r;
}

// One float's function, for each of four.
#define ORO_SIMD_MAP1(name, expr) \
	inline float_4 name(float_4 x) { \
		float_4 r; \
		for (int i = 0; i < 4; i++) { \
			float a = x.s[i]; \
			r.s[i] = (expr); \
		} \
		return r; \
	}
#define ORO_SIMD_MAP2(name, expr) \
	inline float_4 name(float_4 x, float_4 y) { \
		float_4 r; \
		for (int i = 0; i < 4; i++) { \
			float a = x.s[i], b = y.s[i]; \
			r.s[i] = (expr); \
		} \
		return r; \
	}
ORO_SIMD_MAP1(fabs, std::fabs(a))
ORO_SIMD_MAP1(abs, std::fabs(a))
ORO_SIMD_MAP1(sqrt, std::sqrt(a))
ORO_SIMD_MAP1(rsqrt, 1.f / std::sqrt(a))
ORO_SIMD_MAP1(rcp, 1.f / a)
ORO_SIMD_MAP1(floor, std::floor(a))
ORO_SIMD_MAP1(ceil, std::ceil(a))
ORO_SIMD_MAP1(round, std::nearbyint(a))
ORO_SIMD_MAP1(trunc, std::trunc(a))
ORO_SIMD_MAP1(sin, std::sin(a))
ORO_SIMD_MAP1(cos, std::cos(a))
ORO_SIMD_MAP1(tan, std::tan(a))
ORO_SIMD_MAP1(atan, std::atan(a))
ORO_SIMD_MAP1(exp, std::exp(a))
ORO_SIMD_MAP1(log, std::log(a))
ORO_SIMD_MAP1(log10, std::log10(a))
ORO_SIMD_MAP1(log2, std::log2(a))
ORO_SIMD_MAP1(tanh, std::tanh(a))
ORO_SIMD_MAP1(sgn, (float) ((a > 0.f) - (a < 0.f)))
ORO_SIMD_MAP2(fmin, std::fmin(a, b))
ORO_SIMD_MAP2(fmax, std::fmax(a, b))
ORO_SIMD_MAP2(min, std::fmin(a, b))
ORO_SIMD_MAP2(max, std::fmax(a, b))
ORO_SIMD_MAP2(pow, std::pow(a, b))
ORO_SIMD_MAP2(atan2, std::atan2(a, b))
ORO_SIMD_MAP2(fmod, std::fmod(a, b))
ORO_SIMD_MAP2(hypot, std::hypot(a, b))
#undef ORO_SIMD_MAP1
#undef ORO_SIMD_MAP2

inline float_4 pow(float a, float_4 b) { return pow(float_4(a), b); }
inline float_4 pow(float_4 a, float b) { return pow(a, float_4(b)); }
inline float_4 clamp(float_4 x, float_4 a = 0.f, float_4 b = 1.f) { return fmin(fmax(x, a), b); }
inline float_4 rescale(float_4 x, float_4 xMin, float_4 xMax, float_4 yMin, float_4 yMax) {
	return yMin + (x - xMin) / (xMax - xMin) * (yMax - yMin);
}
inline float_4 crossfade(float_4 a, float_4 b, float_4 p) { return a + (b - a) * p; }
/// `a` where the mask is false, `b` where it's true.
inline float_4 ifelse(float_4 mask, float_4 a, float_4 b) { return (a & mask) | (b & ~mask); }
inline float_4 andnot(float_4 a, float_4 b) { return ~a & b; }
/// One bit per float: its sign bit.
inline int movemask(float_4 a) {
	int m = 0;
	for (int i = 0; i < 4; i++)
		m |= (int) (detail::bits(a.s[i]) >> 31) << i;
	return m;
}
inline int movemask(int32_4 a) {
	int m = 0;
	for (int i = 0; i < 4; i++)
		m |= (int) ((uint32_t) a.s[i] >> 31) << i;
	return m;
}
/// The other way: a vector whose element is all ones where the bit is set.
template <typename T>
T movemaskInverse(int a);
template <>
inline int32_4 movemaskInverse<int32_4>(int a) {
	int32_4 r;
	for (int i = 0; i < 4; i++)
		r.s[i] = (a >> i) & 1 ? -1 : 0;
	return r;
}
template <>
inline float_4 movemaskInverse<float_4>(int a) {
	float_4 r;
	for (int i = 0; i < 4; i++)
		r.s[i] = detail::truth((a >> i) & 1);
	return r;
}

// four complex numbers
inline float_4 abs(std::complex<float_4> a) { return hypot(a.real(), a.imag()); }
inline float_4 arg(std::complex<float_4> a) { return atan2(a.imag(), a.real()); }

/// A power by a whole number, by squaring.
template <typename T>
T pow(T a, int b) {
	if (b < 0)
		return 1 / pow(a, -b);
	T r = 1;
	while (b) {
		if (b & 1)
			r *= a;
		a *= a;
		b >>= 1;
	}
	return r;
}

// The same names for one float, so that code written for either compiles.
inline float ifelse(bool cond, float a, float b) { return cond ? a : b; }
using std::abs;
using std::arg;
using std::hypot;
using math::sgn;
using std::atan;
using std::atan2;
using std::ceil;
using std::cos;
using std::exp;
using std::fabs;
using std::floor;
using std::fmax;
using std::fmin;
using std::fmod;
using std::log;
using std::log10;
using std::log2;
using std::pow;
using std::round;
using std::sin;
using std::sqrt;
using std::tan;
using std::tanh;
using std::trunc;
using math::clamp;
using math::crossfade;
using math::rescale;

} // namespace simd
} // namespace rack
