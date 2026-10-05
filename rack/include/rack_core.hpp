// The basics a Rack 2 module's source expects of `rack.hpp`: the standard
// headers, the logging macros, `string`, `math` (with Vec and Rect),
// `random`, `system` and `asset`.
//
// The Oroboro Modular SDK's own code, written to the same interface as VCV
// Rack 2's plugin API so that a Rack module's source compiles against it
// unchanged (docs/rack-modules.md). None of it is Rack's code.
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <climits>
#include <cmath>
#include <complex>
#include <condition_variable>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <functional>
#include <initializer_list>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962
#endif
#ifndef M_1_PI
#define M_1_PI 0.31830988618379067154
#endif
#ifndef M_2_PI
#define M_2_PI 0.63661977236758134308
#endif
#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390
#endif
#ifndef M_LOG2E
#define M_LOG2E 1.44269504088896340736
#endif
#ifndef M_LOG10E
#define M_LOG10E 0.43429448190325182765
#endif
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif
#ifndef M_LN2
#define M_LN2 0.69314718055994530942
#endif
#ifndef M_LN10
#define M_LN10 2.30258509299404568402
#endif

// Defined while a module's source is built for Oroboro Modular: for the
// few lines a module wants otherwise here than in Rack
// (`#ifdef OROBORO_MODULAR`).
#define OROBORO_MODULAR 1

// (jansson.h says what it is)
struct json_t;

#define DEPRECATED __attribute__((deprecated))
#define PRIVATE
#define CONCAT_LITERAL(x, y) x##y
#define CONCAT(x, y) CONCAT_LITERAL(x, y)
#define TOSTRING_LITERAL(x) #x
#define TOSTRING(x) TOSTRING_LITERAL(x)
#define LENGTHOF(arr) (sizeof(arr) / sizeof((arr)[0]))
// A run of `count` enum values: `ENUMS(STEP_PARAM, 8)` is STEP_PARAM … STEP_PARAM + 7.
#define ENUMS(name, count) name, name##_LAST = name + (count) - 1
#define DEFER(code) auto CONCAT(_defer_, __COUNTER__) = rack::deferWrapper([&]() code)
// A Vec's or a Rect's numbers, as arguments.
#define VEC_ARGS(v) (v).x, (v).y
#define RECT_ARGS(r) (r).pos.x, (r).pos.y, (r).size.x, (r).size.y
// A file embedded with `xxd -i` (the symbols are the plugin's own).
#define BINARY(sym) extern "C" {extern const unsigned char sym[]; extern const unsigned int sym##_len;}
#define BINARY_START(sym) (sym)
#define BINARY_END(sym) (sym + sym##_len)
#define BINARY_SIZE(sym) (sym##_len)

// Numbers of an exact type: 42_i8, 0x4a2b_i32, 4.2e-4_f64.
inline int8_t operator"" _i8(unsigned long long x) { return x; }
inline int16_t operator"" _i16(unsigned long long x) { return x; }
inline int32_t operator"" _i32(unsigned long long x) { return x; }
inline int64_t operator"" _i64(unsigned long long x) { return x; }
inline uint8_t operator"" _u8(unsigned long long x) { return x; }
inline uint16_t operator"" _u16(unsigned long long x) { return x; }
inline uint32_t operator"" _u32(unsigned long long x) { return x; }
inline uint64_t operator"" _u64(unsigned long long x) { return x; }
inline float operator"" _f32(long double x) { return x; }
inline float operator"" _f32(unsigned long long x) { return x; }
inline double operator"" _f64(long double x) { return x; }
inline double operator"" _f64(unsigned long long x) { return x; }

namespace rack {

/// The Rack a plugin is told it runs in (a plugin compares it with
/// `string::Version`): Rack 2's interface, as the SDK has it.
static const std::string APP_VERSION = "2.6.0";
static const std::string APP_VERSION_MAJOR = "2";
static const std::string APP_NAME = "VCV Rack";
static const std::string APP_EDITION = "free";
static const std::string APP_EDITION_NAME = "Free";
#if defined(_WIN32)
static const std::string APP_OS = "win";
static const std::string APP_OS_NAME = "Windows";
#elif defined(__APPLE__)
static const std::string APP_OS = "mac";
static const std::string APP_OS_NAME = "Mac";
#else
static const std::string APP_OS = "lin";
static const std::string APP_OS_NAME = "Linux";
#endif
#if defined(__aarch64__) || defined(__arm64__)
static const std::string APP_CPU = "arm64";
static const std::string APP_CPU_NAME = "ARM64";
#else
static const std::string APP_CPU = "x64";
static const std::string APP_CPU_NAME = "x64";
#endif
static const std::string API_URL = "";

/// A primitive's bits, as another of the same size.
template <typename To, typename From>
To bitCast(From from) {
	static_assert(sizeof(From) == sizeof(To), "Types must be the same size");
	To to;
	std::memcpy(&to, &from, sizeof(From));
	return to;
}

/// A new `T`, with members set: `construct<Foo>(&Foo::legs, 2, &Foo::name, "Rex")`.
template <typename T>
T* construct() {
	return new T;
}
template <typename T, typename F, typename V, typename... Args>
T* construct(F f, V v, Args... args) {
	T* o = construct<T>(args...);
	o->*f = v;
	return o;
}

/// A map's value for `key`, or `def` (not added to the map).
template <typename C>
const typename C::mapped_type& get(const C& c, const typename C::key_type& key,
	const typename C::mapped_type& def = typename C::mapped_type()) {
	typename C::const_iterator it = c.find(key);
	if (it == c.end())
		return def;
	return it->second;
}
/// A vector's element `i`, or `def`.
template <typename C>
const typename C::value_type& get(const C& c, typename C::size_type i, const typename C::value_type& def = typename C::value_type()) {
	if (i >= c.size())
		return def;
	return c[i];
}

/// Runs `f` when it goes out of scope (`DEFER`).
template <typename F>
struct DeferWrapper {
	F f;
	bool armed = true;
	DeferWrapper(F f) : f(f) {}
	DeferWrapper(DeferWrapper&& other) : f(other.f) { other.armed = false; }
	DeferWrapper(const DeferWrapper&) = delete;
	~DeferWrapper() {
		if (armed)
			f();
	}
};
template <typename F>
DeferWrapper<F> deferWrapper(F f) {
	return DeferWrapper<F>(f);
}

/// What Rack code throws.
struct Exception : std::exception {
	std::string msg;
	Exception(const char* format, ...) __attribute__((format(printf, 2, 3)));
	Exception(const std::string& msg) : msg(msg) {}
	const char* what() const noexcept override { return msg.c_str(); }
};

// ---- Logging -------------------------------------------------------------------------------
// Silent, unless OROBORO_RACK_LOG is set where the module runs (then: stderr).
namespace logger {
enum Level { DEBUG_LEVEL, INFO_LEVEL, WARN_LEVEL, FATAL_LEVEL };
void log(Level level, const char* filename, int line, const char* func, const char* format, ...)
	__attribute__((format(printf, 5, 6)));
/// Where Rack writes its log; here it goes to stderr, when at all.
static std::string logPath;
inline bool init() { return true; }
inline void destroy() {}
inline bool wasTruncated() { return false; }
} // namespace logger

#define DEBUG(format, ...) rack::logger::log(rack::logger::DEBUG_LEVEL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define INFO(format, ...) rack::logger::log(rack::logger::INFO_LEVEL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define WARN(format, ...) rack::logger::log(rack::logger::WARN_LEVEL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define FATAL(format, ...) rack::logger::log(rack::logger::FATAL_LEVEL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)

// ---- string --------------------------------------------------------------------------------
namespace string {
/// printf into a string.
std::string f(const char* format, ...) __attribute__((format(printf, 1, 2)));
std::string fV(const char* format, va_list args);
/// `f`'s arguments as printf takes them: a std::string as its characters
/// (Rack's `f` takes them either way).
template <typename T>
T convertFArg(const T& t) {
	return t;
}
inline const char* convertFArg(const std::string& s) { return s.c_str(); }
template <typename... Args>
std::string f(Args... args) {
	typedef std::string (*FType)(const char* format, ...);
	return FType(f)(convertFArg(args)...);
}

inline std::string lowercase(const std::string& s) {
	std::string r = s;
	std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return std::tolower(c); });
	return r;
}
inline std::string uppercase(const std::string& s) {
	std::string r = s;
	std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return std::toupper(c); });
	return r;
}
inline std::string trim(const std::string& s) {
	const char* space = " \n\r\t";
	size_t first = s.find_first_not_of(space);
	if (first == std::string::npos)
		return "";
	size_t last = s.find_last_not_of(space);
	return s.substr(first, last - first + 1);
}
inline std::string ellipsize(const std::string& s, size_t len) {
	if (s.size() <= len)
		return s;
	return s.substr(0, len > 3 ? len - 3 : 0) + "...";
}
inline std::string ellipsizePrefix(const std::string& s, size_t len) {
	if (s.size() <= len)
		return s;
	return "..." + s.substr(s.size() - (len > 3 ? len - 3 : 0));
}
inline bool startsWith(const std::string& str, const std::string& prefix) {
	return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}
inline bool endsWith(const std::string& str, const std::string& suffix) {
	return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}
inline std::vector<std::string> split(const std::string& s, const std::string& separator, size_t maxTokens = 0) {
	std::vector<std::string> tokens;
	if (separator.empty()) {
		tokens.push_back(s);
		return tokens;
	}
	size_t offset = 0;
	while (true) {
		size_t end = s.find(separator, offset);
		if (end == std::string::npos || (maxTokens > 0 && tokens.size() + 1 >= maxTokens)) {
			tokens.push_back(s.substr(offset));
			break;
		}
		tokens.push_back(s.substr(offset, end - offset));
		offset = end + separator.size();
	}
	return tokens;
}
template <typename TContainer>
std::string join(const TContainer& container, std::string separator = "") {
	std::string s;
	bool first = true;
	for (const auto& c : container) {
		if (!first)
			s += separator;
		first = false;
		s += c;
	}
	return s;
}
struct CaseInsensitiveCompare {
	bool operator()(const std::string& a, const std::string& b) const { return lowercase(a) < lowercase(b); }
};
/// A version as Rack writes them, "2.5.0": compared part by part, numbers
/// as numbers.
struct Version {
	std::vector<std::string> parts;

	Version() {}
	Version(const std::string& s) : parts(split(s, ".")) {}
	Version(const char* s) : Version(std::string(s)) {}
	operator std::string() const { return join(parts, "."); }
	std::string str() const { return join(parts, "."); }
	bool operator==(const Version& other) const { return compare(other) == 0; }
	bool operator!=(const Version& other) const { return compare(other) != 0; }
	bool operator<(const Version& other) const { return compare(other) < 0; }
	std::string getMajor() const { return get(parts, 0, ""); }
	std::string getMinor() const { return get(parts, 1, ""); }
	std::string getRevision() const { return get(parts, 2, ""); }

	int compare(const Version& other) const {
		size_t n = std::max(parts.size(), other.parts.size());
		for (size_t i = 0; i < n; i++) {
			std::string a = i < parts.size() ? parts[i] : "0";
			std::string b = i < other.parts.size() ? other.parts[i] : "0";
			bool numbers = !a.empty() && !b.empty() && a.find_first_not_of("0123456789") == std::string::npos
				&& b.find_first_not_of("0123456789") == std::string::npos;
			if (numbers) {
				long long x = std::stoll(a), y = std::stoll(b);
				if (x != y)
					return x < y ? -1 : 1;
			} else if (a != b) {
				return a < b ? -1 : 1;
			}
		}
		return 0;
	}
};
/// How well `query` matches `s`, 0 when it doesn't: its letters in order, case aside.
inline float fuzzyScore(const std::string& s, const std::string& query) {
	std::string a = lowercase(s), b = lowercase(query);
	size_t at = 0;
	for (char c : b) {
		at = a.find(c, at);
		if (at == std::string::npos)
			return 0.f;
		at++;
	}
	return b.empty() ? 0.f : (float) b.size() / (float) std::max<size_t>(a.size(), 1);
}

// UTF-8 and the other encodings, by codepoint.
/// Where the codepoint after the one at `pos` starts (the string's length at its end).
inline size_t UTF8NextCodepoint(const std::string& s8, size_t pos) {
	if (pos >= s8.size())
		return s8.size();
	pos++;
	while (pos < s8.size() && (((unsigned char) s8[pos]) & 0xc0) == 0x80)
		pos++;
	return pos;
}
/// Where the codepoint before `pos` starts (0 at the start).
inline size_t UTF8PrevCodepoint(const std::string& s8, size_t pos) {
	if (pos == 0)
		return 0;
	pos = std::min(pos, s8.size());
	pos--;
	while (pos > 0 && (((unsigned char) s8[pos]) & 0xc0) == 0x80)
		pos--;
	return pos;
}
inline size_t UTF8Length(const std::string& s8) {
	size_t n = 0;
	for (unsigned char c : s8)
		n += (c & 0xc0) != 0x80;
	return n;
}
/// The number of codepoints before byte `pos`.
inline size_t UTF8CodepointIndex(const std::string& s8, size_t pos) {
	return UTF8Length(s8.substr(0, std::min(pos, s8.size())));
}
/// Where codepoint `index` starts.
inline size_t UTF8CodepointPos(const std::string& s8, size_t index) {
	size_t pos = 0;
	while (index-- > 0 && pos < s8.size())
		pos = UTF8NextCodepoint(s8, pos);
	return pos;
}
inline std::u32string UTF8toUTF32(const std::string& s8) {
	std::u32string out;
	for (size_t i = 0; i < s8.size();) {
		unsigned char c = s8[i];
		char32_t cp;
		size_t n;
		if (c < 0x80) { cp = c; n = 1; }
		else if ((c & 0xe0) == 0xc0) { cp = c & 0x1f; n = 2; }
		else if ((c & 0xf0) == 0xe0) { cp = c & 0x0f; n = 3; }
		else { cp = c & 0x07; n = 4; }
		for (size_t k = 1; k < n && i + k < s8.size(); k++)
			cp = (cp << 6) | (((unsigned char) s8[i + k]) & 0x3f);
		out.push_back(cp);
		i += n;
	}
	return out;
}
inline std::string UTF32toUTF8(const std::u32string& s32) {
	std::string out;
	for (char32_t cp : s32) {
		if (cp < 0x80) {
			out += (char) cp;
		} else if (cp < 0x800) {
			out += (char) (0xc0 | (cp >> 6));
			out += (char) (0x80 | (cp & 0x3f));
		} else if (cp < 0x10000) {
			out += (char) (0xe0 | (cp >> 12));
			out += (char) (0x80 | ((cp >> 6) & 0x3f));
			out += (char) (0x80 | (cp & 0x3f));
		} else {
			out += (char) (0xf0 | (cp >> 18));
			out += (char) (0x80 | ((cp >> 12) & 0x3f));
			out += (char) (0x80 | ((cp >> 6) & 0x3f));
			out += (char) (0x80 | (cp & 0x3f));
		}
	}
	return out;
}
inline std::wstring UTF8toUTF16(const std::string& s) {
	std::wstring out;
	for (char32_t cp : UTF8toUTF32(s)) {
		if (sizeof(wchar_t) == 2 && cp >= 0x10000) {
			cp -= 0x10000;
			out += (wchar_t) (0xd800 + (cp >> 10));
			out += (wchar_t) (0xdc00 + (cp & 0x3ff));
		} else {
			out += (wchar_t) cp;
		}
	}
	return out;
}
inline std::string UTF16toUTF8(const std::wstring& w) {
	std::u32string s32;
	for (size_t i = 0; i < w.size(); i++) {
		char32_t c = (char32_t) w[i];
		if (sizeof(wchar_t) == 2 && c >= 0xd800 && c < 0xdc00 && i + 1 < w.size()) {
			c = 0x10000 + ((c - 0xd800) << 10) + ((char32_t) w[i + 1] - 0xdc00);
			i++;
		}
		s32 += c;
	}
	return UTF32toUTF8(s32);
}
/// At most `maxCodepoints` of `s`'s codepoints, from its start (an ellipsis
/// where it's cut).
inline std::string truncate(const std::string& s, size_t maxCodepoints) {
	if (UTF8Length(s) <= maxCodepoints)
		return s;
	if (maxCodepoints == 0)
		return "";
	return s.substr(0, UTF8CodepointPos(s, maxCodepoints - 1)) + "\u2026";
}
/// The same, keeping its end.
inline std::string truncatePrefix(const std::string& s, size_t maxCodepoints) {
	size_t length = UTF8Length(s);
	if (length <= maxCodepoints)
		return s;
	if (maxCodepoints == 0)
		return "";
	return "\u2026" + s.substr(UTF8CodepointPos(s, length - (maxCodepoints - 1)));
}
inline std::string toBase64(const uint8_t* data, size_t dataLen) {
	static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	for (size_t i = 0; i < dataLen; i += 3) {
		uint32_t n = (uint32_t) data[i] << 16;
		if (i + 1 < dataLen)
			n |= (uint32_t) data[i + 1] << 8;
		if (i + 2 < dataLen)
			n |= data[i + 2];
		out += table[(n >> 18) & 63];
		out += table[(n >> 12) & 63];
		out += i + 1 < dataLen ? table[(n >> 6) & 63] : '=';
		out += i + 2 < dataLen ? table[n & 63] : '=';
	}
	return out;
}
inline std::string toBase64(const std::vector<uint8_t>& data) { return toBase64(data.data(), data.size()); }
inline std::vector<uint8_t> fromBase64(const std::string& str) {
	auto value = [](char c) -> int {
		if (c >= 'A' && c <= 'Z') return c - 'A';
		if (c >= 'a' && c <= 'z') return c - 'a' + 26;
		if (c >= '0' && c <= '9') return c - '0' + 52;
		if (c == '+' || c == '-') return 62;
		if (c == '/' || c == '_') return 63;
		return -1;
	};
	std::vector<uint8_t> out;
	uint32_t bits = 0;
	int count = 0;
	for (char c : str) {
		int v = value(c);
		if (v < 0)
			continue;
		bits = (bits << 6) | (uint32_t) v;
		count += 6;
		if (count >= 8) {
			count -= 8;
			out.push_back((uint8_t) (bits >> count));
		}
	}
	return out;
}
/// A Unix time (seconds) as `strftime`'s `format` writes it, in local time.
inline std::string formatTime(const char* format, double timestamp) {
	time_t t = (time_t) timestamp;
	struct tm local;
#if defined(_WIN32)
	localtime_s(&local, &t);
#else
	localtime_r(&t, &local);
#endif
	char out[256];
	size_t n = strftime(out, sizeof(out), format, &local);
	return std::string(out, n);
}
inline std::string formatTimeISO(double timestamp) { return formatTime("%Y-%m-%d %H:%M:%S", timestamp); }

// Rack's translations: the SDK has English only.
inline std::string translate(const std::string& id) { return id; }
inline std::string translate(const std::string& id, const std::string& language) { return id; }
inline std::vector<std::string> getLanguages() { return {"en"}; }
inline void init() {}
} // namespace string

// ---- math ----------------------------------------------------------------------------------
namespace math {

inline int clamp(int x, int a, int b) { return std::max(std::min(x, b), a); }
inline int clampSafe(int x, int a, int b) { return (a <= b) ? clamp(x, a, b) : clamp(x, b, a); }
inline int eucMod(int a, int b) {
	int mod = a % b;
	return (mod >= 0) ? mod : mod + b;
}
inline int eucDiv(int a, int b) {
	int div = a / b;
	int mod = a % b;
	return (mod < 0) ? div - 1 : div;
}
inline void eucDivMod(int a, int b, int* div, int* mod) {
	*div = a / b;
	*mod = a % b;
	if (*mod < 0) {
		*div -= 1;
		*mod += b;
	}
}
inline int log2(int n) {
	int i = 0;
	while (n >>= 1)
		i++;
	return i;
}
template <typename T>
bool isPow2(T n) {
	return n > 0 && (n & (n - 1)) == 0;
}
template <typename T>
T sgn(T x) {
	return x > 0 ? 1 : (x < 0 ? -1 : 0);
}
inline bool isEven(int x) { return x % 2 == 0; }
inline bool isOdd(int x) { return x % 2 != 0; }
inline int binomial(int n, int k) {
	int r = 1;
	for (int i = 1; i <= k; i++) {
		r *= n - k + i;
		r /= i;
	}
	return r;
}

inline float clamp(float x, float a = 0.f, float b = 1.f) { return std::fmax(std::fmin(x, b), a); }
inline float clampSafe(float x, float a = 0.f, float b = 1.f) { return (a <= b) ? clamp(x, a, b) : clamp(x, b, a); }
inline float normalizeZero(float x) { return x + 0.f; }
inline float eucMod(float a, float b) {
	float mod = std::fmod(a, b);
	return (mod >= 0.f) ? mod : mod + b;
}
inline bool isNear(float a, float b, float epsilon = 1e-6f) { return std::fabs(a - b) <= epsilon; }
inline float chop(float x, float epsilon = 1e-6f) { return std::fabs(x) <= epsilon ? 0.f : x; }
/// `x` from the range `xMin`..`xMax` to the range `yMin`..`yMax`.
inline float rescale(float x, float xMin, float xMax, float yMin, float yMax) {
	return yMin + (x - xMin) / (xMax - xMin) * (yMax - yMin);
}
inline float crossfade(float a, float b, float p) { return a + (b - a) * p; }
/// Between `p[i]` and `p[i + 1]`, for `x` between `i` and `i + 1`.
inline float interpolateLinear(const float* p, float x) {
	int xi = (int) x;
	float xf = x - xi;
	return crossfade(p[xi], p[xi + 1], xf);
}
inline void cartesianToPolar(float x, float y, float* r, float* theta) {
	*r = std::hypot(x, y);
	*theta = std::atan2(y, x);
}
inline void polarToCartesian(float r, float theta, float* x, float* y) {
	*x = r * std::cos(theta);
	*y = r * std::sin(theta);
}

struct Rect;

struct Vec {
	float x = 0.f;
	float y = 0.f;

	Vec() {}
	Vec(float xy) : x(xy), y(xy) {}
	Vec(float x, float y) : x(x), y(y) {}

	float& operator[](int i) { return (i == 0) ? x : y; }
	const float& operator[](int i) const { return (i == 0) ? x : y; }
	Vec neg() const { return Vec(-x, -y); }
	Vec plus(Vec b) const { return Vec(x + b.x, y + b.y); }
	Vec minus(Vec b) const { return Vec(x - b.x, y - b.y); }
	Vec mult(float s) const { return Vec(x * s, y * s); }
	Vec mult(Vec b) const { return Vec(x * b.x, y * b.y); }
	Vec div(float s) const { return Vec(x / s, y / s); }
	Vec div(Vec b) const { return Vec(x / b.x, y / b.y); }
	float dot(Vec b) const { return x * b.x + y * b.y; }
	float arg() const { return std::atan2(y, x); }
	float norm() const { return std::hypot(x, y); }
	Vec normalize() const { return div(norm()); }
	float square() const { return x * x + y * y; }
	float area() const { return x * y; }
	Vec rotate(float angle) const {
		float sin = std::sin(angle), cos = std::cos(angle);
		return Vec(x * cos - y * sin, x * sin + y * cos);
	}
	Vec flip() const { return Vec(y, x); }
	Vec min(Vec b) const { return Vec(std::fmin(x, b.x), std::fmin(y, b.y)); }
	Vec max(Vec b) const { return Vec(std::fmax(x, b.x), std::fmax(y, b.y)); }
	Vec abs() const { return Vec(std::fabs(x), std::fabs(y)); }
	Vec round() const { return Vec(std::round(x), std::round(y)); }
	Vec floor() const { return Vec(std::floor(x), std::floor(y)); }
	Vec ceil() const { return Vec(std::ceil(x), std::ceil(y)); }
	bool equals(Vec b) const { return x == b.x && y == b.y; }
	bool isEqual(Vec b) const { return equals(b); }
	bool isZero() const { return x == 0.f && y == 0.f; }
	bool isFinite() const { return std::isfinite(x) && std::isfinite(y); }
	Vec clamp(Rect bound) const;
	Vec clampSafe(Rect bound) const;
	Vec crossfade(Vec b, float p) const { return this->plus(b.minus(*this).mult(p)); }
	/// Fits it into `aspect`'s shape, keeping its area's proportions.
	Vec fit(Vec aspect) const {
		Vec ratio = div(aspect);
		float r = std::fmin(ratio.x, ratio.y);
		return aspect.mult(r);
	}
};

struct Rect {
	Vec pos;
	Vec size;

	Rect() {}
	Rect(Vec pos, Vec size) : pos(pos), size(size) {}
	Rect(float posX, float posY, float sizeX, float sizeY) : pos(Vec(posX, posY)), size(Vec(sizeX, sizeY)) {}
	static Rect fromMinMax(Vec a, Vec b) { return Rect(a, b.minus(a)); }
	static Rect fromCorners(Vec a, Vec b) { return fromMinMax(a.min(b), a.max(b)); }
	static Rect inf() { return Rect(Vec(-INFINITY, -INFINITY), Vec(INFINITY, INFINITY)); }

	bool contains(Vec v) const { return pos.x <= v.x && v.x < pos.x + size.x && pos.y <= v.y && v.y < pos.y + size.y; }
	bool contains(Rect r) const {
		return pos.x <= r.pos.x && r.pos.x + r.size.x <= pos.x + size.x && pos.y <= r.pos.y && r.pos.y + r.size.y <= pos.y + size.y;
	}
	bool intersects(Rect r) const {
		return pos.x < r.pos.x + r.size.x && r.pos.x < pos.x + size.x && pos.y < r.pos.y + r.size.y && r.pos.y < pos.y + size.y;
	}
	bool equals(Rect r) const { return pos.equals(r.pos) && size.equals(r.size); }
	bool isEqual(Rect r) const { return equals(r); }
	bool isContaining(Vec v) const { return contains(v); }
	bool isContaining(Rect r) const { return contains(r); }
	bool isIntersecting(Rect r) const { return intersects(r); }
	float getLeft() const { return pos.x; }
	float getRight() const { return pos.x + size.x; }
	float getTop() const { return pos.y; }
	float getBottom() const { return pos.y + size.y; }
	float getWidth() const { return size.x; }
	float getHeight() const { return size.y; }
	Vec getCenter() const { return pos.plus(size.mult(0.5f)); }
	Vec getTopLeft() const { return pos; }
	Vec getTopCenter() const { return Vec(pos.x + size.x * 0.5f, pos.y); }
	Vec getTopRight() const { return Vec(pos.x + size.x, pos.y); }
	Vec getLeftCenter() const { return Vec(pos.x, pos.y + size.y * 0.5f); }
	Vec getRightCenter() const { return Vec(pos.x + size.x, pos.y + size.y * 0.5f); }
	Vec getBottomLeft() const { return Vec(pos.x, pos.y + size.y); }
	Vec getBottomCenter() const { return Vec(pos.x + size.x * 0.5f, pos.y + size.y); }
	Vec getBottomRight() const { return pos.plus(size); }
	Vec getSize() const { return size; }
	Rect clamp(Rect bound) const {
		Rect r;
		r.pos.x = math::clampSafe(pos.x, bound.pos.x, bound.pos.x + bound.size.x);
		r.pos.y = math::clampSafe(pos.y, bound.pos.y, bound.pos.y + bound.size.y);
		r.size.x = math::clamp(pos.x + size.x, bound.pos.x, bound.pos.x + bound.size.x) - r.pos.x;
		r.size.y = math::clamp(pos.y + size.y, bound.pos.y, bound.pos.y + bound.size.y) - r.pos.y;
		return r;
	}
	Rect nudge(Rect bound) const {
		Rect r;
		r.size = size;
		r.pos.x = math::clampSafe(pos.x, bound.pos.x, bound.pos.x + bound.size.x - size.x);
		r.pos.y = math::clampSafe(pos.y, bound.pos.y, bound.pos.y + bound.size.y - size.y);
		return r;
	}
	Rect expand(Rect b) const {
		Rect r;
		r.pos.x = std::fmin(pos.x, b.pos.x);
		r.pos.y = std::fmin(pos.y, b.pos.y);
		r.size.x = std::fmax(pos.x + size.x, b.pos.x + b.size.x) - r.pos.x;
		r.size.y = std::fmax(pos.y + size.y, b.pos.y + b.size.y) - r.pos.y;
		return r;
	}
	Rect intersect(Rect b) const {
		Rect r;
		r.pos.x = std::fmax(pos.x, b.pos.x);
		r.pos.y = std::fmax(pos.y, b.pos.y);
		r.size.x = std::fmin(pos.x + size.x, b.pos.x + b.size.x) - r.pos.x;
		r.size.y = std::fmin(pos.y + size.y, b.pos.y + b.size.y) - r.pos.y;
		return r;
	}
	Rect zeroPos() const { return Rect(Vec(), size); }
	Rect grow(Vec delta) const { return Rect(pos.minus(delta), size.plus(delta.mult(2.f))); }
	Rect shrink(Vec delta) const { return Rect(pos.plus(delta), size.minus(delta.mult(2.f))); }
	Vec interpolate(Vec p) const { return pos.plus(size.mult(p)); }
};

inline Vec Vec::clamp(Rect bound) const {
	return Vec(math::clamp(x, bound.pos.x, bound.pos.x + bound.size.x), math::clamp(y, bound.pos.y, bound.pos.y + bound.size.y));
}
inline Vec Vec::clampSafe(Rect bound) const {
	return Vec(math::clampSafe(x, bound.pos.x, bound.pos.x + bound.size.x), math::clampSafe(y, bound.pos.y, bound.pos.y + bound.size.y));
}

inline Vec operator+(const Vec& a) { return a; }
inline Vec operator-(const Vec& a) { return a.neg(); }
inline Vec operator+(const Vec& a, const Vec& b) { return a.plus(b); }
inline Vec operator-(const Vec& a, const Vec& b) { return a.minus(b); }
inline Vec operator*(const Vec& a, const Vec& b) { return a.mult(b); }
inline Vec operator*(const Vec& a, const float& b) { return a.mult(b); }
inline Vec operator*(const float& a, const Vec& b) { return b.mult(a); }
inline Vec operator/(const Vec& a, const Vec& b) { return a.div(b); }
inline Vec operator/(const Vec& a, const float& b) { return a.div(b); }
inline Vec operator+=(Vec& a, const Vec& b) { return a = a.plus(b); }
inline Vec operator-=(Vec& a, const Vec& b) { return a = a.minus(b); }
inline Vec operator*=(Vec& a, const Vec& b) { return a = a.mult(b); }
inline Vec operator*=(Vec& a, const float& b) { return a = a.mult(b); }
inline Vec operator/=(Vec& a, const Vec& b) { return a = a.div(b); }
inline Vec operator/=(Vec& a, const float& b) { return a = a.div(b); }
inline bool operator==(const Vec& a, const Vec& b) { return a.equals(b); }
inline bool operator!=(const Vec& a, const Vec& b) { return !a.equals(b); }
inline bool operator==(const Rect& a, const Rect& b) { return a.equals(b); }
inline bool operator!=(const Rect& a, const Rect& b) { return !a.equals(b); }

/// (ar + ai i)(br + bi i), into cr and ci.
inline void complexMult(float ar, float ai, float br, float bi, float* cr, float* ci) {
	*cr = ar * br - ai * bi;
	*ci = ar * bi + ai * br;
}
} // namespace math

// ---- random --------------------------------------------------------------------------------
namespace random {

/// The xoroshiro128+ generator.
struct Xoroshiro128Plus {
	uint64_t state[2] = {};

	void seed(uint64_t s0, uint64_t s1) {
		state[0] = s0;
		state[1] = s1;
		// (never all zero, and stirred)
		if (state[0] == 0 && state[1] == 0)
			state[0] = 0x9e3779b97f4a7c15ull;
		(*this)();
	}
	bool isSeeded() { return state[0] || state[1]; }
	static uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
	uint64_t operator()() {
		uint64_t s0 = state[0];
		uint64_t s1 = state[1];
		uint64_t result = s0 + s1;
		s1 ^= s0;
		state[0] = rotl(s0, 55) ^ s1 ^ (s1 << 14);
		state[1] = rotl(s1, 36);
		return result;
	}
	constexpr uint64_t min() const { return 0; }
	constexpr uint64_t max() const { return UINT64_MAX; }
};

/// This thread's generator, seeded when first asked.
Xoroshiro128Plus& local();
inline void init() { local(); }

template <typename T>
T get() {
	return (T) local()();
}
inline uint64_t u64() { return local()(); }
inline uint32_t u32() { return (uint32_t)(local()() >> 32); }
/// In [0, 1).
inline float uniform() { return (float)(u32() >> 8) * (1.f / 16777216.f); }
/// A normal distribution's, mean 0 and deviation 1.
inline float normal() {
	const float radius = std::sqrt(-2.f * std::log(1.f - uniform()));
	const float theta = 2.f * (float) M_PI * uniform();
	return radius * std::sin(theta);
}
inline void buffer(uint8_t* out, size_t len) {
	for (size_t i = 0; i < len; i++)
		out[i] = (uint8_t)(u64() >> 56);
}
/// `len` random bytes.
inline std::vector<uint8_t> vector(size_t len) {
	std::vector<uint8_t> v(len);
	buffer(v.data(), len);
	return v;
}
} // namespace random

// ---- system --------------------------------------------------------------------------------
namespace system {

/// Two paths as one.
std::string joinTwo(const std::string& path1, const std::string& path2);
inline std::string join(const std::string& path) { return path; }
template <typename... Paths>
std::string join(const std::string& path1, const std::string& path2, Paths... paths) {
	return join(joinTwo(path1, path2), paths...);
}

std::vector<std::string> getEntries(const std::string& dirPath, int depth = 0);
bool exists(const std::string& path);
bool isFile(const std::string& path);
bool isDirectory(const std::string& path);
uint64_t getFileSize(const std::string& path);
bool rename(const std::string& srcPath, const std::string& destPath);
bool copy(const std::string& srcPath, const std::string& destPath);
bool createDirectory(const std::string& path);
bool createDirectories(const std::string& path);
bool createSymbolicLink(const std::string& target, const std::string& link);
bool remove(const std::string& path);
int removeRecursively(const std::string& path);
std::string getWorkingDirectory();
void setWorkingDirectory(const std::string& path);
std::string getTempDirectory();
std::string getAbsolute(const std::string& path);
std::string getCanonical(const std::string& path);
std::string getDirectory(const std::string& path);
std::string getFilename(const std::string& path);
std::string getStem(const std::string& path);
std::string getExtension(const std::string& path);
std::vector<uint8_t> readFile(const std::string& path);
uint8_t* readFile(const std::string& path, size_t* size);
bool writeFile(const std::string& path, const std::vector<uint8_t>& data);

/// Seconds since something fixed, for intervals.
double getTime();
double getUnixTime();
double getThreadTime();
void sleep(double time);
int getLogicalCoreCount();
std::string getOperatingSystemInfo();
/// The host has no browser to open or folder to show: these do nothing.
void openBrowser(const std::string& url);
void openDirectory(const std::string& path);
void runProcessDetached(const std::string& path);
void setThreadName(const std::string& name);
/// The thread's floating-point flags (flush denormals to zero, …).
inline uint32_t getFpuFlags() {
#if defined(__SSE__) || defined(__x86_64__) || defined(_M_X64)
	uint32_t csr;
	__asm__ volatile("stmxcsr %0" : "=m"(csr));
	return csr;
#else
	return 0;
#endif
}
inline void setFpuFlags(uint32_t flags) {
#if defined(__SSE__) || defined(__x86_64__) || defined(_M_X64)
	__asm__ volatile("ldmxcsr %0" : : "m"(flags));
#else
	(void) flags;
#endif
}
/// Denormals flushed to zero, as Rack's engine threads run.
inline void resetFpuFlags() {
#if defined(__SSE__) || defined(__x86_64__) || defined(_M_X64)
	setFpuFlags((getFpuFlags() & ~0x8040u) | 0x8040u);
#endif
}
inline std::string getStackTrace() { return ""; }
// Rack's patch archives (tar and zstd): not here.
inline void archiveDirectory(const std::string& archivePath, const std::string& dirPath, int compressionLevel = 1) {
	throw Exception("Archives aren't made here");
}
inline std::vector<uint8_t> archiveDirectory(const std::string& dirPath, int compressionLevel = 1) {
	throw Exception("Archives aren't made here");
}
inline void unarchiveToDirectory(const std::string& archivePath, const std::string& dirPath) {
	throw Exception("Archives aren't read here");
}
inline void unarchiveToDirectory(const std::vector<uint8_t>& archiveData, const std::string& dirPath) {
	throw Exception("Archives aren't read here");
}
} // namespace system

namespace plugin {
struct Plugin;
struct Model;
} // namespace plugin

// ---- network -------------------------------------------------------------------------------
// A module asks nothing of the internet from inside a musician's plugin:
// every request here fails, as when there's no connection.
namespace network {
enum Method { METHOD_GET, METHOD_POST, METHOD_PUT, METHOD_DELETE };
typedef std::map<std::string, std::string> CookieMap;
inline ::json_t* requestJson(Method method, const std::string& url, ::json_t* dataJ, const CookieMap& cookies = {}) { return nullptr; }
inline bool requestDownload(const std::string& url, const std::string& filename, float* progress, const CookieMap& cookies = {}) {
	return false;
}
inline std::string encodeUrl(const std::string& s) { return s; }
inline std::string urlPath(const std::string& url) {
	size_t scheme = url.find("://");
	size_t slash = url.find('/', scheme == std::string::npos ? 0 : scheme + 3);
	return slash == std::string::npos ? "" : url.substr(slash);
}
} // namespace network

// ---- asset ---------------------------------------------------------------------------------
namespace asset {
/// A file of Rack's own (its fonts, its panel art): there is none here, so
/// a path that doesn't exist.
std::string system(std::string filename = "");
/// A file of the musician's: under "Oroboro Modular/Rack" in their documents.
std::string user(std::string filename = "");
/// A file of the plugin's own folder (its res/, its presets): the folder
/// its source was built from.
std::string plugin(plugin::Plugin* plugin, std::string filename = "");
extern std::string systemDir;
extern std::string userDir;
/// Rack's own folders before its user folder moved, and its app bundle:
/// none here.
static std::string oldUserDir;
static std::string bundlePath;
} // namespace asset

// ---- tags ----------------------------------------------------------------------------------
/// What a module does, as plugin.json says it (`tags`): Rack's list.
namespace tag {
static const std::vector<std::vector<std::string>> tagAliases = {
	{"Arpeggiator"}, {"Attenuator"}, {"Blank"}, {"Chorus"}, {"Clock generator", "Clock"}, {"Clock modulator"},
	{"Compressor"}, {"Controller"}, {"Delay"}, {"Digital"}, {"Distortion"}, {"Drum", "Drums", "Percussion"},
	{"Dual", "2"}, {"Dynamics"}, {"Effect"}, {"Envelope follower"}, {"Envelope generator"}, {"Equalizer", "EQ"},
	{"Expander"}, {"External"}, {"Filter", "VCF", "Voltage controlled filter"}, {"Flanger"},
	{"Function generator"}, {"Granular"}, {"Hardware clone", "Hardware"}, {"Limiter"}, {"Logic"},
	{"Low-frequency oscillator", "LFO", "Low frequency oscillator"}, {"Low-pass gate", "Low pass gate", "Lowpass gate"},
	{"MIDI"}, {"Mixer"}, {"Multiple"}, {"Noise"}, {"Oscillator", "VCO", "Voltage controlled oscillator"}, {"Panning", "Pan"},
	{"Phaser"}, {"Physical modeling"}, {"Polyphonic", "Poly"}, {"Quad", "4"}, {"Quantizer"}, {"Random"},
	{"Recording"}, {"Reverb"}, {"Ring modulator"}, {"Sample and hold", "S&H", "Sample & hold"}, {"Sampler"},
	{"Sequencer"}, {"Slew limiter"}, {"Speech"}, {"Switch"}, {"Synth voice", "Voice"}, {"Tuner"}, {"Utility"},
	{"Visual"}, {"Vocoder"}, {"Voltage-controlled amplifier", "Amplifier", "VCA", "Voltage controlled amplifier"},
	{"Waveshaper"},
};
/// A tag's number, by any of its names (case aside); -1 if none.
inline int findId(const std::string& tag) {
	std::string lower = string::lowercase(tag);
	for (size_t id = 0; id < tagAliases.size(); id++)
		for (const std::string& alias : tagAliases[id])
			if (string::lowercase(alias) == lower)
				return (int) id;
	return -1;
}
inline std::string getTag(int tagId) {
	return tagId >= 0 && (size_t) tagId < tagAliases.size() ? tagAliases[tagId][0] : "";
}
} // namespace tag

// ---- shared mutex --------------------------------------------------------------------------
/// A lock many may read under and one may write under (C++17's
/// std::shared_mutex, for C++11).
struct SharedMutex {
	std::mutex m;
	std::condition_variable cv;
	int readers = 0;
	bool writer = false;

	void lock() {
		std::unique_lock<std::mutex> l(m);
		cv.wait(l, [&] { return !writer && readers == 0; });
		writer = true;
	}
	bool try_lock() {
		std::lock_guard<std::mutex> l(m);
		if (writer || readers > 0)
			return false;
		writer = true;
		return true;
	}
	void unlock() {
		{
			std::lock_guard<std::mutex> l(m);
			if (writer)
				writer = false;
			else if (readers > 0)
				readers--;
		}
		cv.notify_all();
	}
	void lock_shared() {
		std::unique_lock<std::mutex> l(m);
		cv.wait(l, [&] { return !writer; });
		readers++;
	}
	bool try_lock_shared() {
		std::lock_guard<std::mutex> l(m);
		if (writer)
			return false;
		readers++;
		return true;
	}
	void unlock_shared() {
		{
			std::lock_guard<std::mutex> l(m);
			readers--;
		}
		cv.notify_all();
	}
};
template <class TMutex>
struct SharedLock {
	TMutex& m;
	SharedLock(TMutex& m) : m(m) { m.lock_shared(); }
	~SharedLock() { m.unlock_shared(); }
};

// ---- weak pointers -------------------------------------------------------------------------
struct WeakBase;

/// What `WeakPtr`s to one object share: the object while it lives (null
/// once it's gone), and how many follow it.
struct WeakHandle {
	WeakBase* object = nullptr;
	size_t count = 0;
};

/// What a `WeakPtr` can follow: widgets and modules are. A copy of one
/// isn't followed by the original's.
struct WeakBase {
	WeakHandle* weakHandle = nullptr;
	WeakBase() = default;
	WeakBase(const WeakBase&) {}
	WeakBase& operator=(const WeakBase&) { return *this; }
	~WeakBase() {
		if (weakHandle)
			weakHandle->object = nullptr;
	}
	size_t getWeakCount() { return weakHandle ? weakHandle->count : 0; }
};

/// A pointer that turns null when what it points to is deleted (a widget
/// a menu's action may outlive, as Rack's `WeakPtr`).
template <typename T>
struct WeakPtr {
	WeakHandle* handle = nullptr;
	WeakPtr() {}
	WeakPtr(T* ptr) { set(ptr); }
	WeakPtr(const WeakPtr& other) { set(other.get()); }
	WeakPtr& operator=(const WeakPtr& other) {
		set(other.get());
		return *this;
	}
	~WeakPtr() { set(nullptr); }
	void set(T* ptr) {
		if (handle) {
			// the last one to follow it lets the handle go
			if (--handle->count == 0) {
				if (handle->object)
					handle->object->weakHandle = nullptr;
				delete handle;
			}
			handle = nullptr;
		}
		if (ptr) {
			WeakBase* object = ptr;
			if (!object->weakHandle) {
				object->weakHandle = new WeakHandle;
				object->weakHandle->object = object;
			}
			handle = object->weakHandle;
			handle->count++;
		}
	}
	T* get() const { return handle && handle->object ? static_cast<T*>(handle->object) : nullptr; }
	T* operator->() const { return get(); }
	T& operator*() const { return *get(); }
	operator T*() const { return get(); }
};

} // namespace rack
