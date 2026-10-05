// What a Rack module's panel code draws (NanoVG's interface, nanovg.h),
// written down instead of drawn: each fill, stroke, text and picture (an SVG
// of the module's, `window::svgDraw`) becomes an entry
// of a list of numbers (a drawing: native/include/oroboro_module.h says how
// it reads), in the panel's own pixels, which the plugin draws in its own
// way. Paths are kept as straight pieces; curves are cut fine enough for
// twice the plugin's zoom. There are no fonts here: a text is passed on as
// its words, its size and where it stands, and measured by a rule of thumb.
// The Oroboro Modular SDK's own code; none of it is NanoVG's.

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include <nanovg.h>

namespace {

const float PI = 3.14159265358979323846f;
/// How far a curve's straight pieces may be from it (panel pixels).
const float FLATNESS = 0.06f;
/// A letter's width for a size of 1, about (there are no fonts to ask).
const float ADVANCE = 0.55f;

struct State {
	float xform[6] = {1, 0, 0, 1, 0, 0};
	NVGpaint fill = {};
	NVGpaint stroke = {};
	float strokeWidth = 1.f;
	float miterLimit = 10.f;
	int lineCap = NVG_BUTT;
	int lineJoin = NVG_MITER;
	float alpha = 1.f;
	/// The clip, in panel pixels (none while left > right).
	float clip[4] = {1.f, 0.f, 0.f, 0.f};
	float fontSize = 16.f;
	float letterSpacing = 0.f;
	float lineHeight = 1.f;
	int textAlign = NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE;
	int fontId = -1;
};

struct Sub {
	/// x, y, x, y, … in panel pixels.
	std::vector<float> points;
	bool closed = false;
	bool hole = false;
};

/// The fonts asked for so far, by the ids given for them: their names
/// (file names), which say at least whether a font is a typewriter's.
std::vector<std::string>& fonts() {
	static std::vector<std::string>* all = new std::vector<std::string>;
	return *all;
}

void setColor(NVGpaint& p, NVGcolor color) {
	p = NVGpaint();
	p.xform[0] = 1.f;
	p.xform[3] = 1.f;
	p.feather = 1.f;
	p.innerColor = color;
	p.outerColor = color;
}

/// `t` then `by`: a point goes through `t` first.
void multiply(float* t, const float* by) {
	float t0 = t[0] * by[0] + t[1] * by[2];
	float t2 = t[2] * by[0] + t[3] * by[2];
	float t4 = t[4] * by[0] + t[5] * by[2] + by[4];
	t[1] = t[0] * by[1] + t[1] * by[3];
	t[3] = t[2] * by[1] + t[3] * by[3];
	t[5] = t[4] * by[1] + t[5] * by[3] + by[5];
	t[0] = t0;
	t[2] = t2;
	t[4] = t4;
}

bool inverse(float* inv, const float* t) {
	double det = (double) t[0] * t[3] - (double) t[2] * t[1];
	if (det > -1e-9 && det < 1e-9) {
		inv[0] = 1; inv[1] = 0; inv[2] = 0; inv[3] = 1; inv[4] = 0; inv[5] = 0;
		return false;
	}
	double invdet = 1.0 / det;
	inv[0] = (float) (t[3] * invdet);
	inv[2] = (float) (-t[2] * invdet);
	inv[4] = (float) (((double) t[2] * t[5] - (double) t[3] * t[4]) * invdet);
	inv[1] = (float) (-t[1] * invdet);
	inv[3] = (float) (t[0] * invdet);
	inv[5] = (float) (((double) t[1] * t[4] - (double) t[0] * t[5]) * invdet);
	return true;
}

} // namespace

struct NVGcontext {
	std::vector<State> states;
	std::vector<Sub> path;
	/// Where the pen is, in the caller's own coordinates.
	float penX = 0.f, penY = 0.f;
	std::vector<float> out;

	NVGcontext() { states.push_back(State()); }
	State& state() { return states.back(); }

	void map(float x, float y, float& ox, float& oy) {
		const float* t = state().xform;
		ox = x * t[0] + y * t[2] + t[4];
		oy = x * t[1] + y * t[3] + t[5];
	}
	/// How much the transform stretches lengths, about.
	float stretch() {
		const float* t = state().xform;
		return std::sqrt(std::fabs(t[0] * t[3] - t[2] * t[1]));
	}
	void point(float x, float y) {
		if (path.empty())
			path.push_back(Sub());
		float ox, oy;
		map(x, y, ox, oy);
		path.back().points.push_back(ox);
		path.back().points.push_back(oy);
		penX = x;
		penY = y;
	}
	void moveTo(float x, float y) {
		path.push_back(Sub());
		point(x, y);
	}
	void lineTo(float x, float y) {
		if (path.empty() || path.back().points.empty())
			moveTo(penX, penY);
		point(x, y);
	}
	void bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y) {
		if (path.empty() || path.back().points.empty())
			moveTo(penX, penY);
		float x0 = penX, y0 = penY;
		// as many pieces as keep them within the flatness, in panel pixels
		float s = stretch();
		float bend1 = std::hypot(x0 - 2 * c1x + c2x, y0 - 2 * c1y + c2y) * s;
		float bend2 = std::hypot(c1x - 2 * c2x + x, c1y - 2 * c2y + y) * s;
		int n = (int) std::ceil(std::sqrt(0.75f * std::fmax(bend1, bend2) / FLATNESS));
		n = n < 1 ? 1 : (n > 200 ? 200 : n);
		for (int i = 1; i <= n; i++) {
			float t = (float) i / n, u = 1.f - t;
			float a = u * u * u, b = 3 * u * u * t, c = 3 * u * t * t, d = t * t * t;
			point(a * x0 + b * c1x + c * c2x + d * x, a * y0 + b * c1y + c * c2y + d * y);
		}
	}
	/// Along a circle around (cx, cy) from angle a0, `sweep` further.
	void arc(float cx, float cy, float rx, float ry, float a0, float sweep, bool move) {
		float r = std::fmax(rx, ry) * stretch();
		float step = r > FLATNESS ? 2.f * std::acos(1.f - FLATNESS / r) : PI / 2;
		int n = (int) std::ceil(std::fabs(sweep) / std::fmax(step, 0.02f));
		n = n < 1 ? 1 : (n > 400 ? 400 : n);
		for (int i = 0; i <= n; i++) {
			float a = a0 + sweep * i / n;
			float x = cx + std::cos(a) * rx, y = cy + std::sin(a) * ry;
			if (i == 0 && move)
				moveTo(x, y);
			else
				lineTo(x, y);
		}
	}

	void put(float x) { out.push_back(x); }
	void putClip() {
		for (float c : state().clip)
			put(c);
	}
	void putPaint(const NVGpaint& p) {
		const NVGcolor& in = p.innerColor;
		const NVGcolor& ou = p.outerColor;
		bool plain = in.r == ou.r && in.g == ou.g && in.b == ou.b && in.a == ou.a;
		float a = state().alpha;
		put(plain ? 0.f : 1.f);
		put(in.r); put(in.g); put(in.b); put(in.a * a);
		put(ou.r); put(ou.g); put(ou.b); put(ou.a * a);
		// from panel pixels back to where the paint is laid out
		float inv[6];
		inverse(inv, p.xform);
		for (float m : inv)
			put(m);
		put(p.extent[0]); put(p.extent[1]); put(p.radius); put(p.feather);
	}
	/// An entry: its kind, how many numbers follow (filled in by `close`).
	size_t open(float kind) {
		put(kind);
		put(0.f);
		return out.size();
	}
	void close(size_t from) { out[from - 1] = (float) (out.size() - from); }
};

extern "C" {

NVGcontext* oroboro_nvg_new(void) { return new NVGcontext; }
void oroboro_nvg_free(NVGcontext* ctx) { delete ctx; }
void oroboro_nvg_begin(NVGcontext* ctx) {
	if (!ctx)
		return;
	ctx->states.clear();
	ctx->states.push_back(State());
	setColor(ctx->state().fill, nvgRGBAf(1, 1, 1, 1));
	setColor(ctx->state().stroke, nvgRGBAf(0, 0, 0, 1));
	ctx->path.clear();
	ctx->out.clear();
}
const float* oroboro_nvg_list(NVGcontext* ctx, unsigned int* count) {
	if (count)
		*count = ctx ? (unsigned int) ctx->out.size() : 0;
	return ctx && !ctx->out.empty() ? ctx->out.data() : nullptr;
}
void oroboro_nvg_picture(NVGcontext* ctx, int index, float width, float height) {
	if (!ctx || index < 0 || !(width > 0.f) || !(height > 0.f))
		return;
	size_t from = ctx->open(4.f);
	ctx->putClip();
	ctx->put((float) index);
	ctx->put(width);
	ctx->put(height);
	for (float m : ctx->state().xform)
		ctx->put(m);
	ctx->close(from);
}
int oroboro_nvg_font(const char* filename) {
	std::string name = filename ? filename : "";
	std::vector<std::string>& all = fonts();
	for (size_t i = 0; i < all.size(); i++)
		if (all[i] == name)
			return (int) i;
	all.push_back(name);
	return (int) all.size() - 1;
}

void nvgBeginFrame(NVGcontext* ctx, float, float, float) {}
void nvgCancelFrame(NVGcontext* ctx) {}
void nvgEndFrame(NVGcontext* ctx) {}
void nvgGlobalCompositeOperation(NVGcontext* ctx, int op) {}
void nvgGlobalCompositeBlendFunc(NVGcontext* ctx, int sfactor, int dfactor) {}
void nvgGlobalCompositeBlendFuncSeparate(NVGcontext* ctx, int srcRGB, int dstRGB, int srcAlpha, int dstAlpha) {}

void nvgSave(NVGcontext* ctx) {
	if (ctx && ctx->states.size() < 64)
		ctx->states.push_back(ctx->states.back());
}
void nvgRestore(NVGcontext* ctx) {
	if (ctx && ctx->states.size() > 1)
		ctx->states.pop_back();
}
void nvgReset(NVGcontext* ctx) {
	if (!ctx)
		return;
	ctx->state() = State();
	setColor(ctx->state().fill, nvgRGBAf(1, 1, 1, 1));
	setColor(ctx->state().stroke, nvgRGBAf(0, 0, 0, 1));
}
void nvgShapeAntiAlias(NVGcontext* ctx, int enabled) {}
void nvgStrokeColor(NVGcontext* ctx, NVGcolor color) {
	if (ctx)
		setColor(ctx->state().stroke, color);
}
void nvgStrokePaint(NVGcontext* ctx, NVGpaint paint) {
	if (!ctx)
		return;
	ctx->state().stroke = paint;
	multiply(ctx->state().stroke.xform, ctx->state().xform);
}
void nvgFillColor(NVGcontext* ctx, NVGcolor color) {
	if (ctx)
		setColor(ctx->state().fill, color);
}
void nvgFillPaint(NVGcontext* ctx, NVGpaint paint) {
	if (!ctx)
		return;
	ctx->state().fill = paint;
	multiply(ctx->state().fill.xform, ctx->state().xform);
}
void nvgMiterLimit(NVGcontext* ctx, float limit) {
	if (ctx)
		ctx->state().miterLimit = limit;
}
void nvgStrokeWidth(NVGcontext* ctx, float size) {
	if (ctx)
		ctx->state().strokeWidth = size;
}
void nvgLineCap(NVGcontext* ctx, int cap) {
	if (ctx)
		ctx->state().lineCap = cap;
}
void nvgLineJoin(NVGcontext* ctx, int join) {
	if (ctx)
		ctx->state().lineJoin = join;
}
void nvgGlobalAlpha(NVGcontext* ctx, float alpha) {
	if (ctx)
		ctx->state().alpha = alpha;
}
void nvgGlobalTint(NVGcontext* ctx, NVGcolor tint) {}
NVGcolor nvgGetGlobalTint(NVGcontext* ctx) { return nvgRGBAf(1, 1, 1, 1); }
void nvgAlpha(NVGcontext* ctx, float alpha) {
	if (ctx)
		ctx->state().alpha *= alpha;
}
void nvgTint(NVGcontext* ctx, NVGcolor tint) {}

void nvgResetTransform(NVGcontext* ctx) {
	if (!ctx)
		return;
	float same[6] = {1, 0, 0, 1, 0, 0};
	std::memcpy(ctx->state().xform, same, sizeof(same));
}
void nvgTransform(NVGcontext* ctx, float a, float b, float c, float d, float e, float f) {
	if (!ctx)
		return;
	float t[6] = {a, b, c, d, e, f};
	multiply(t, ctx->state().xform);
	std::memcpy(ctx->state().xform, t, sizeof(t));
}
void nvgTranslate(NVGcontext* ctx, float x, float y) { nvgTransform(ctx, 1, 0, 0, 1, x, y); }
void nvgRotate(NVGcontext* ctx, float angle) {
	float cs = std::cos(angle), sn = std::sin(angle);
	nvgTransform(ctx, cs, sn, -sn, cs, 0, 0);
}
void nvgSkewX(NVGcontext* ctx, float angle) { nvgTransform(ctx, 1, 0, std::tan(angle), 1, 0, 0); }
void nvgSkewY(NVGcontext* ctx, float angle) { nvgTransform(ctx, 1, std::tan(angle), 0, 1, 0, 0); }
void nvgScale(NVGcontext* ctx, float x, float y) { nvgTransform(ctx, x, 0, 0, y, 0, 0); }
void nvgCurrentTransform(NVGcontext* ctx, float* xform) {
	if (!xform)
		return;
	float same[6] = {1, 0, 0, 1, 0, 0};
	std::memcpy(xform, ctx ? ctx->state().xform : same, sizeof(same));
}

int nvgCreateImage(NVGcontext* ctx, const char* filename, int imageFlags) { return 0; }
int nvgCreateImageMem(NVGcontext* ctx, int imageFlags, unsigned char* data, int ndata) { return 0; }
int nvgCreateImageRGBA(NVGcontext* ctx, int w, int h, int imageFlags, const unsigned char* data) { return 0; }
void nvgUpdateImage(NVGcontext* ctx, int image, const unsigned char* data) {}
void nvgImageSize(NVGcontext* ctx, int image, int* w, int* h) {
	if (w)
		*w = 0;
	if (h)
		*h = 0;
}
void nvgDeleteImage(NVGcontext* ctx, int image) {}

// The three gradients are one kind of paint: a rounded box (its half
// sizes, its corners' radius) laid out by a transform, the inner colour
// inside it, the outer one outside, and `feather` wide the way from one to
// the other across its edge.
NVGpaint nvgLinearGradient(NVGcontext* ctx, float sx, float sy, float ex, float ey, NVGcolor icol, NVGcolor ocol) {
	NVGpaint p = {};
	const float large = 1e5f;
	float dx = ex - sx, dy = ey - sy;
	float d = std::sqrt(dx * dx + dy * dy);
	if (d > 0.0001f) {
		dx /= d;
		dy /= d;
	}
	else {
		dx = 0;
		dy = 1;
	}
	p.xform[0] = dy; p.xform[1] = -dx;
	p.xform[2] = dx; p.xform[3] = dy;
	p.xform[4] = sx - dx * large; p.xform[5] = sy - dy * large;
	p.extent[0] = large;
	p.extent[1] = large + d * 0.5f;
	p.radius = 0.f;
	p.feather = std::fmax(1.f, d);
	p.innerColor = icol;
	p.outerColor = ocol;
	return p;
}
NVGpaint nvgBoxGradient(NVGcontext* ctx, float x, float y, float w, float h, float r, float f, NVGcolor icol, NVGcolor ocol) {
	NVGpaint p = {};
	p.xform[0] = 1; p.xform[3] = 1;
	p.xform[4] = x + w * 0.5f; p.xform[5] = y + h * 0.5f;
	p.extent[0] = w * 0.5f;
	p.extent[1] = h * 0.5f;
	p.radius = r;
	p.feather = std::fmax(1.f, f);
	p.innerColor = icol;
	p.outerColor = ocol;
	return p;
}
NVGpaint nvgRadialGradient(NVGcontext* ctx, float cx, float cy, float inr, float outr, NVGcolor icol, NVGcolor ocol) {
	NVGpaint p = {};
	float r = (inr + outr) * 0.5f;
	p.xform[0] = 1; p.xform[3] = 1;
	p.xform[4] = cx; p.xform[5] = cy;
	p.extent[0] = r;
	p.extent[1] = r;
	p.radius = r;
	p.feather = std::fmax(1.f, outr - inr);
	p.innerColor = icol;
	p.outerColor = ocol;
	return p;
}
NVGpaint nvgImagePattern(NVGcontext* ctx, float ox, float oy, float ex, float ey, float angle, int image, float alpha) {
	// (there are no pictures to paint with: what's filled with one isn't drawn)
	NVGpaint p = {};
	p.xform[0] = 1; p.xform[3] = 1;
	p.image = image ? image : -1;
	return p;
}

void nvgScissor(NVGcontext* ctx, float x, float y, float w, float h) {
	if (!ctx)
		return;
	// the box around the rectangle as the transform leaves it
	float xs[4], ys[4];
	ctx->map(x, y, xs[0], ys[0]);
	ctx->map(x + w, y, xs[1], ys[1]);
	ctx->map(x + w, y + h, xs[2], ys[2]);
	ctx->map(x, y + h, xs[3], ys[3]);
	float* c = ctx->state().clip;
	c[0] = c[2] = xs[0];
	c[1] = c[3] = ys[0];
	for (int i = 1; i < 4; i++) {
		c[0] = std::fmin(c[0], xs[i]); c[2] = std::fmax(c[2], xs[i]);
		c[1] = std::fmin(c[1], ys[i]); c[3] = std::fmax(c[3], ys[i]);
	}
}
void nvgIntersectScissor(NVGcontext* ctx, float x, float y, float w, float h) {
	if (!ctx)
		return;
	float was[4];
	std::memcpy(was, ctx->state().clip, sizeof(was));
	nvgScissor(ctx, x, y, w, h);
	if (was[0] > was[2])
		return;
	float* c = ctx->state().clip;
	c[0] = std::fmax(c[0], was[0]); c[1] = std::fmax(c[1], was[1]);
	c[2] = std::fmin(c[2], was[2]); c[3] = std::fmin(c[3], was[3]);
	if (c[0] > c[2] || c[1] > c[3]) {
		// nothing is left: a clip no point is in
		c[2] = c[0];
		c[3] = c[1];
	}
}
void nvgResetScissor(NVGcontext* ctx) {
	if (!ctx)
		return;
	float none[4] = {1.f, 0.f, 0.f, 0.f};
	std::memcpy(ctx->state().clip, none, sizeof(none));
}

void nvgBeginPath(NVGcontext* ctx) {
	if (ctx)
		ctx->path.clear();
}
void nvgMoveTo(NVGcontext* ctx, float x, float y) {
	if (ctx)
		ctx->moveTo(x, y);
}
void nvgLineTo(NVGcontext* ctx, float x, float y) {
	if (ctx)
		ctx->lineTo(x, y);
}
void nvgBezierTo(NVGcontext* ctx, float c1x, float c1y, float c2x, float c2y, float x, float y) {
	if (ctx)
		ctx->bezierTo(c1x, c1y, c2x, c2y, x, y);
}
void nvgQuadTo(NVGcontext* ctx, float cx, float cy, float x, float y) {
	if (!ctx)
		return;
	float x0 = ctx->penX, y0 = ctx->penY;
	ctx->bezierTo(x0 + 2.f / 3.f * (cx - x0), y0 + 2.f / 3.f * (cy - y0), x + 2.f / 3.f * (cx - x), y + 2.f / 3.f * (cy - y), x, y);
}
void nvgClosePath(NVGcontext* ctx) {
	if (ctx && !ctx->path.empty())
		ctx->path.back().closed = true;
}
void nvgPathWinding(NVGcontext* ctx, int dir) {
	if (ctx && !ctx->path.empty())
		ctx->path.back().hole = dir == NVG_HOLE;
}
void nvgArc(NVGcontext* ctx, float cx, float cy, float r, float a0, float a1, int dir) {
	if (!ctx)
		return;
	float da = a1 - a0;
	if (dir == NVG_CW) {
		if (std::fabs(da) >= PI * 2)
			da = PI * 2;
		else
			while (da < 0.f)
				da += PI * 2;
	}
	else {
		if (std::fabs(da) >= PI * 2)
			da = -PI * 2;
		else
			while (da > 0.f)
				da -= PI * 2;
	}
	bool move = ctx->path.empty() || ctx->path.back().points.empty();
	ctx->arc(cx, cy, r, r, a0, da, move);
}
void nvgArcTo(NVGcontext* ctx, float x1, float y1, float x2, float y2, float radius) {
	if (!ctx)
		return;
	float x0 = ctx->penX, y0 = ctx->penY;
	// the corner at (x1, y1) between the lines to it and from it, rounded
	float dx0 = x0 - x1, dy0 = y0 - y1, dx1 = x2 - x1, dy1 = y2 - y1;
	float l0 = std::hypot(dx0, dy0), l1 = std::hypot(dx1, dy1);
	if (l0 < 1e-6f || l1 < 1e-6f || radius < 1e-6f) {
		ctx->lineTo(x1, y1);
		return;
	}
	dx0 /= l0; dy0 /= l0; dx1 /= l1; dy1 /= l1;
	float a = std::acos(std::fmax(-1.f, std::fmin(1.f, dx0 * dx1 + dy0 * dy1)));
	float d = radius / std::tan(a / 2.f);
	if (!std::isfinite(d) || d > 10000.f) {
		ctx->lineTo(x1, y1);
		return;
	}
	float cross = dx1 * dy0 - dx0 * dy1;
	float cx, cy, a0, a1;
	int dir;
	if (cross > 0.f) {
		cx = x1 + dx0 * d + dy0 * radius;
		cy = y1 + dy0 * d - dx0 * radius;
		a0 = std::atan2(dx0, -dy0);
		a1 = std::atan2(-dx1, dy1);
		dir = NVG_CW;
	}
	else {
		cx = x1 + dx0 * d - dy0 * radius;
		cy = y1 + dy0 * d + dx0 * radius;
		a0 = std::atan2(-dx0, dy0);
		a1 = std::atan2(dx1, -dy1);
		dir = NVG_CCW;
	}
	nvgArc(ctx, cx, cy, radius, a0, a1, dir);
}
void nvgRect(NVGcontext* ctx, float x, float y, float w, float h) {
	if (!ctx)
		return;
	ctx->moveTo(x, y);
	ctx->lineTo(x, y + h);
	ctx->lineTo(x + w, y + h);
	ctx->lineTo(x + w, y);
	ctx->path.back().closed = true;
}
void nvgRoundedRectVarying(NVGcontext* ctx, float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight,
	float radBottomLeft) {
	if (!ctx)
		return;
	if (radTopLeft < 0.1f && radTopRight < 0.1f && radBottomRight < 0.1f && radBottomLeft < 0.1f) {
		nvgRect(ctx, x, y, w, h);
		return;
	}
	float most = std::fmin(std::fabs(w), std::fabs(h)) * 0.5f;
	float tl = std::fmin(radTopLeft, most), tr = std::fmin(radTopRight, most);
	float br = std::fmin(radBottomRight, most), bl = std::fmin(radBottomLeft, most);
	// each corner a quarter of a circle, going round
	ctx->arc(x + tl, y + tl, tl, tl, PI, PI / 2, true);
	ctx->arc(x + w - tr, y + tr, tr, tr, -PI / 2, PI / 2, false);
	ctx->arc(x + w - br, y + h - br, br, br, 0.f, PI / 2, false);
	ctx->arc(x + bl, y + h - bl, bl, bl, PI / 2, PI / 2, false);
	ctx->path.back().closed = true;
}
void nvgRoundedRect(NVGcontext* ctx, float x, float y, float w, float h, float r) { nvgRoundedRectVarying(ctx, x, y, w, h, r, r, r, r); }
void nvgEllipse(NVGcontext* ctx, float cx, float cy, float rx, float ry) {
	if (!ctx)
		return;
	ctx->arc(cx, cy, rx, ry, 0.f, PI * 2, true);
	// (its last point is its first again)
	std::vector<float>& points = ctx->path.back().points;
	if (points.size() >= 4)
		points.resize(points.size() - 2);
	ctx->path.back().closed = true;
}
void nvgCircle(NVGcontext* ctx, float cx, float cy, float r) { nvgEllipse(ctx, cx, cy, r, r); }

void nvgFill(NVGcontext* ctx) {
	if (!ctx || ctx->state().fill.image != 0)
		return;
	bool any = false;
	for (const Sub& sub : ctx->path)
		any = any || sub.points.size() >= 6;
	if (!any)
		return;
	size_t from = ctx->open(1.f);
	ctx->putClip();
	ctx->putPaint(ctx->state().fill);
	for (const Sub& sub : ctx->path) {
		if (sub.points.size() < 6)
			continue;
		ctx->put((float) (sub.points.size() / 2));
		ctx->put(sub.hole ? 1.f : 0.f);
		ctx->out.insert(ctx->out.end(), sub.points.begin(), sub.points.end());
	}
	ctx->close(from);
}
void nvgStroke(NVGcontext* ctx) {
	if (!ctx || ctx->state().stroke.image != 0)
		return;
	bool any = false;
	for (const Sub& sub : ctx->path)
		any = any || sub.points.size() >= 4;
	if (!any)
		return;
	const State& s = ctx->state();
	size_t from = ctx->open(2.f);
	ctx->putClip();
	ctx->putPaint(s.stroke);
	ctx->put(s.strokeWidth * ctx->stretch());
	ctx->put((float) s.lineCap);
	ctx->put((float) s.lineJoin);
	ctx->put(s.miterLimit);
	for (const Sub& sub : ctx->path) {
		if (sub.points.size() < 4)
			continue;
		ctx->put((float) (sub.points.size() / 2));
		ctx->put(sub.closed ? 1.f : 0.f);
		ctx->out.insert(ctx->out.end(), sub.points.begin(), sub.points.end());
	}
	ctx->close(from);
}

int nvgCreateFont(NVGcontext* ctx, const char* name, const char* filename) { return oroboro_nvg_font(filename ? filename : name); }
int nvgCreateFontMem(NVGcontext* ctx, const char* name, unsigned char* data, int ndata, int freeData) { return oroboro_nvg_font(name); }
int nvgFindFont(NVGcontext* ctx, const char* name) { return oroboro_nvg_font(name); }
int nvgAddFallbackFontId(NVGcontext* ctx, int baseFont, int fallbackFont) { return 1; }
void nvgFontSize(NVGcontext* ctx, float size) {
	if (ctx)
		ctx->state().fontSize = size;
}
void nvgFontBlur(NVGcontext* ctx, float blur) {}
void nvgTextLetterSpacing(NVGcontext* ctx, float spacing) {
	if (ctx)
		ctx->state().letterSpacing = spacing;
}
void nvgTextLineHeight(NVGcontext* ctx, float lineHeight) {
	if (ctx)
		ctx->state().lineHeight = lineHeight;
}
void nvgTextAlign(NVGcontext* ctx, int align) {
	if (ctx)
		ctx->state().textAlign = align;
}
void nvgFontFaceId(NVGcontext* ctx, int font) {
	if (ctx)
		ctx->state().fontId = font;
}
void nvgFontFace(NVGcontext* ctx, const char* font) {
	if (ctx)
		ctx->state().fontId = oroboro_nvg_font(font);
}

/// How many letters a text has (UTF-8: its bytes that start one).
static int letters(const char* string, const char* end) {
	int n = 0;
	for (const char* c = string; c != end && *c; c++)
		if ((*c & 0xc0) != 0x80)
			n++;
	return n;
}
static float widthOf(NVGcontext* ctx, const char* string, const char* end) {
	const State& s = ctx->state();
	int n = letters(string, end);
	return n * (s.fontSize * ADVANCE + s.letterSpacing);
}

float nvgText(NVGcontext* ctx, float x, float y, const char* string, const char* end) {
	if (!ctx || !string)
		return x;
	if (!end)
		end = string + std::strlen(string);
	float width = widthOf(ctx, string, end);
	if (end == string)
		return x;
	const State& s = ctx->state();
	const NVGcolor& c = s.fill.innerColor;
	int font = s.fontId;
	const std::vector<std::string>& all = fonts();
	std::string name = font >= 0 && (size_t) font < all.size() ? all[font] : "";
	for (char& ch : name)
		ch = (char) std::tolower((unsigned char) ch);
	bool mono = name.find("mono") != std::string::npos || name.find("code") != std::string::npos || name.find("courier") != std::string::npos;
	bool bold = name.find("bold") != std::string::npos;
	size_t from = ctx->open(3.f);
	ctx->putClip();
	ctx->put(c.r); ctx->put(c.g); ctx->put(c.b); ctx->put(c.a * s.alpha);
	ctx->put(s.fontSize * ctx->stretch());
	ctx->put((float) s.textAlign);
	float ox, oy;
	ctx->map(x, y, ox, oy);
	ctx->put(ox);
	ctx->put(oy);
	ctx->put(std::atan2(s.xform[1], s.xform[0]));
	ctx->put((mono ? 1.f : 0.f) + (bold ? 2.f : 0.f));
	for (const char* ch = string; ch != end; ch++)
		ctx->put((float) (unsigned char) *ch);
	ctx->close(from);
	return x + width;
}
float nvgTextBounds(NVGcontext* ctx, float x, float y, const char* string, const char* end, float* bounds) {
	if (!ctx || !string) {
		if (bounds) {
			bounds[0] = x; bounds[1] = y; bounds[2] = x; bounds[3] = y;
		}
		return 0.f;
	}
	if (!end)
		end = string + std::strlen(string);
	const State& s = ctx->state();
	float width = widthOf(ctx, string, end);
	if (bounds) {
		// by how the text is set against its point
		float left = x;
		if (s.textAlign & NVG_ALIGN_CENTER)
			left = x - width / 2;
		else if (s.textAlign & NVG_ALIGN_RIGHT)
			left = x - width;
		float top = y - s.fontSize * 0.8f;
		if (s.textAlign & NVG_ALIGN_TOP)
			top = y;
		else if (s.textAlign & NVG_ALIGN_MIDDLE)
			top = y - s.fontSize * 0.5f;
		else if (s.textAlign & NVG_ALIGN_BOTTOM)
			top = y - s.fontSize;
		bounds[0] = left; bounds[1] = top; bounds[2] = left + width; bounds[3] = top + s.fontSize;
	}
	return width;
}
void nvgTextMetrics(NVGcontext* ctx, float* ascender, float* descender, float* lineh) {
	float size = ctx ? ctx->state().fontSize : 0.f;
	float line = ctx ? ctx->state().lineHeight : 1.f;
	if (ascender)
		*ascender = size * 0.8f;
	if (descender)
		*descender = -size * 0.2f;
	if (lineh)
		*lineh = size * 1.2f * line;
}
int nvgTextGlyphPositions(NVGcontext* ctx, float x, float y, const char* string, const char* end, NVGglyphPosition* positions, int maxPositions) {
	if (!ctx || !string || !positions)
		return 0;
	if (!end)
		end = string + std::strlen(string);
	const State& s = ctx->state();
	float advance = s.fontSize * ADVANCE + s.letterSpacing;
	int n = 0;
	for (const char* c = string; c != end && *c && n < maxPositions; c++) {
		if ((*c & 0xc0) == 0x80)
			continue;
		positions[n].str = c;
		positions[n].x = x + n * advance;
		positions[n].minx = positions[n].x;
		positions[n].maxx = positions[n].x + advance;
		n++;
	}
	return n;
}
int nvgTextBreakLines(NVGcontext* ctx, const char* string, const char* end, float breakRowWidth, NVGtextRow* rows, int maxRows) {
	if (!ctx || !string || !rows || maxRows <= 0)
		return 0;
	if (!end)
		end = string + std::strlen(string);
	const State& s = ctx->state();
	float advance = s.fontSize * ADVANCE + s.letterSpacing;
	int perRow = advance > 0.f ? (int) std::fmax(1.f, std::floor(breakRowWidth / advance)) : 1000000;
	int n = 0;
	const char* start = string;
	while (start != end && *start && n < maxRows) {
		// as many letters as fit, back to the last space; a new line ends a row
		const char* stop = start;
		const char* space = nullptr;
		int count = 0;
		while (stop != end && *stop && *stop != '\n' && count < perRow) {
			if (*stop == ' ')
				space = stop;
			if ((*stop & 0xc0) != 0x80)
				count++;
			stop++;
		}
		const char* next = stop;
		if (stop != end && *stop == '\n')
			next = stop + 1;
		else if (stop != end && *stop && space && space > start) {
			stop = space;
			next = space + 1;
		}
		rows[n].start = start;
		rows[n].end = stop;
		rows[n].next = next;
		rows[n].width = letters(start, stop) * advance;
		rows[n].minx = 0.f;
		rows[n].maxx = rows[n].width;
		n++;
		if (next == start)
			break;
		start = next;
	}
	return n;
}
void nvgTextBox(NVGcontext* ctx, float x, float y, float breakRowWidth, const char* string, const char* end) {
	if (!ctx || !string)
		return;
	NVGtextRow rows[32];
	int n = nvgTextBreakLines(ctx, string, end, breakRowWidth, rows, 32);
	float size = ctx->state().fontSize, line = ctx->state().lineHeight;
	int align = ctx->state().textAlign;
	for (int i = 0; i < n; i++) {
		// each row set in the box as the whole is
		float at = x;
		if (align & NVG_ALIGN_CENTER)
			at = x + breakRowWidth / 2;
		else if (align & NVG_ALIGN_RIGHT)
			at = x + breakRowWidth;
		nvgText(ctx, at, y + i * size * 1.2f * line, rows[i].start, rows[i].end);
	}
}
void nvgTextBoxBounds(NVGcontext* ctx, float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds) {
	if (!bounds)
		return;
	NVGtextRow rows[32];
	int n = ctx && string ? nvgTextBreakLines(ctx, string, end, breakRowWidth, rows, 32) : 0;
	float size = ctx ? ctx->state().fontSize * 1.2f * ctx->state().lineHeight : 0.f;
	bounds[0] = x; bounds[1] = y - (ctx ? ctx->state().fontSize * 0.8f : 0.f);
	bounds[2] = x + breakRowWidth; bounds[3] = bounds[1] + n * size;
}

} // extern "C"
