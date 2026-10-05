// SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-Oroboro-Rack-Bridge-Exception (rack/LICENSE-EXCEPTION.md)
/*
 * The drawing functions a Rack module's panel code calls (NanoVG's
 * interface). Here nothing is drawn: what a widget draws is written down
 * as a list (rack/src/rack_draw.cpp), which the plugin draws its own way,
 * where the widget is on the module's panel. There are no fonts and no
 * pictures: a text is passed on as its words, measured by a rule of thumb,
 * and what's filled with a picture isn't drawn. A null context takes every
 * call and does nothing. The Oroboro Modular SDK's own header; none of it
 * is NanoVG's code.
 */
#ifndef OROBORO_RACK_NANOVG_H
#define OROBORO_RACK_NANOVG_H

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NVG_PI 3.14159265358979323846264338327f

typedef struct NVGcontext NVGcontext;

struct NVGcolor {
	union {
		float rgba[4];
		struct {
			float r, g, b, a;
		};
	};
};
typedef struct NVGcolor NVGcolor;

struct NVGpaint {
	float xform[6];
	float extent[2];
	float radius;
	float feather;
	NVGcolor innerColor;
	NVGcolor outerColor;
	int image;
};
typedef struct NVGpaint NVGpaint;

enum NVGwinding { NVG_CCW = 1, NVG_CW = 2 };
enum NVGsolidity { NVG_SOLID = 1, NVG_HOLE = 2 };
enum NVGlineCap { NVG_BUTT, NVG_ROUND, NVG_SQUARE, NVG_BEVEL, NVG_MITER };
enum NVGalign {
	NVG_ALIGN_LEFT = 1 << 0,
	NVG_ALIGN_CENTER = 1 << 1,
	NVG_ALIGN_RIGHT = 1 << 2,
	NVG_ALIGN_TOP = 1 << 3,
	NVG_ALIGN_MIDDLE = 1 << 4,
	NVG_ALIGN_BOTTOM = 1 << 5,
	NVG_ALIGN_BASELINE = 1 << 6
};
enum NVGblendFactor {
	NVG_ZERO = 1 << 0,
	NVG_ONE = 1 << 1,
	NVG_SRC_COLOR = 1 << 2,
	NVG_ONE_MINUS_SRC_COLOR = 1 << 3,
	NVG_DST_COLOR = 1 << 4,
	NVG_ONE_MINUS_DST_COLOR = 1 << 5,
	NVG_SRC_ALPHA = 1 << 6,
	NVG_ONE_MINUS_SRC_ALPHA = 1 << 7,
	NVG_DST_ALPHA = 1 << 8,
	NVG_ONE_MINUS_DST_ALPHA = 1 << 9,
	NVG_SRC_ALPHA_SATURATE = 1 << 10
};
enum NVGcompositeOperation {
	NVG_SOURCE_OVER,
	NVG_SOURCE_IN,
	NVG_SOURCE_OUT,
	NVG_ATOP,
	NVG_DESTINATION_OVER,
	NVG_DESTINATION_IN,
	NVG_DESTINATION_OUT,
	NVG_DESTINATION_ATOP,
	NVG_LIGHTER,
	NVG_COPY,
	NVG_XOR
};
enum NVGimageFlags {
	NVG_IMAGE_GENERATE_MIPMAPS = 1 << 0,
	NVG_IMAGE_REPEATX = 1 << 1,
	NVG_IMAGE_REPEATY = 1 << 2,
	NVG_IMAGE_FLIPY = 1 << 3,
	NVG_IMAGE_PREMULTIPLIED = 1 << 4,
	NVG_IMAGE_NEAREST = 1 << 5
};

struct NVGglyphPosition {
	const char *str;
	float x;
	float minx, maxx;
};
typedef struct NVGglyphPosition NVGglyphPosition;

struct NVGtextRow {
	const char *start;
	const char *end;
	const char *next;
	float width;
	float minx, maxx;
};
typedef struct NVGtextRow NVGtextRow;

/* colours: these do compute, since modules keep and compare them */
static inline NVGcolor nvgRGBAf(float r, float g, float b, float a) {
	NVGcolor color;
	color.r = r;
	color.g = g;
	color.b = b;
	color.a = a;
	return color;
}
static inline NVGcolor nvgRGBf(float r, float g, float b) { return nvgRGBAf(r, g, b, 1.0f); }
static inline NVGcolor nvgRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	return nvgRGBAf(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}
static inline NVGcolor nvgRGB(unsigned char r, unsigned char g, unsigned char b) { return nvgRGBA(r, g, b, 255); }
static inline NVGcolor nvgTransRGBA(NVGcolor c0, unsigned char a) {
	c0.a = a / 255.0f;
	return c0;
}
static inline NVGcolor nvgTransRGBAf(NVGcolor c0, float a) {
	c0.a = a;
	return c0;
}
static inline NVGcolor nvgLerpRGBA(NVGcolor c0, NVGcolor c1, float u) {
	float oneminu = 1.0f - u;
	NVGcolor cint;
	int i;
	for (i = 0; i < 4; i++)
		cint.rgba[i] = c0.rgba[i] * oneminu + c1.rgba[i] * u;
	return cint;
}
NVGcolor nvgHSLA(float h, float s, float l, unsigned char a);
static inline NVGcolor nvgHSL(float h, float s, float l) { return nvgHSLA(h, s, l, 255); }

/* transforms as six numbers: these compute */
static inline void nvgTransformIdentity(float *dst) {
	dst[0] = 1; dst[1] = 0; dst[2] = 0; dst[3] = 1; dst[4] = 0; dst[5] = 0;
}
static inline void nvgTransformPoint(float *dstx, float *dsty, const float *xform, float srcx, float srcy) {
	*dstx = srcx * xform[0] + srcy * xform[2] + xform[4];
	*dsty = srcx * xform[1] + srcy * xform[3] + xform[5];
}
static inline void nvgTransformTranslate(float *dst, float tx, float ty) {
	dst[0] = 1; dst[1] = 0; dst[2] = 0; dst[3] = 1; dst[4] = tx; dst[5] = ty;
}
static inline void nvgTransformScale(float *dst, float sx, float sy) {
	dst[0] = sx; dst[1] = 0; dst[2] = 0; dst[3] = sy; dst[4] = 0; dst[5] = 0;
}
static inline void nvgTransformRotate(float *dst, float a) {
	float c = cosf(a), s = sinf(a);
	dst[0] = c; dst[1] = s; dst[2] = -s; dst[3] = c; dst[4] = 0; dst[5] = 0;
}
static inline void nvgTransformSkewX(float *dst, float a) {
	dst[0] = 1; dst[1] = 0; dst[2] = tanf(a); dst[3] = 1; dst[4] = 0; dst[5] = 0;
}
static inline void nvgTransformSkewY(float *dst, float a) {
	dst[0] = 1; dst[1] = tanf(a); dst[2] = 0; dst[3] = 1; dst[4] = 0; dst[5] = 0;
}
/* dst = dst then src (as NanoVG's: src applied after dst) */
static inline void nvgTransformMultiply(float *dst, const float *src) {
	float t0 = dst[0] * src[0] + dst[1] * src[2];
	float t2 = dst[2] * src[0] + dst[3] * src[2];
	float t4 = dst[4] * src[0] + dst[5] * src[2] + src[4];
	dst[1] = dst[0] * src[1] + dst[1] * src[3];
	dst[3] = dst[2] * src[1] + dst[3] * src[3];
	dst[5] = dst[4] * src[1] + dst[5] * src[3] + src[5];
	dst[0] = t0;
	dst[2] = t2;
	dst[4] = t4;
}
static inline void nvgTransformPremultiply(float *dst, const float *src) {
	float s2[6];
	int i;
	for (i = 0; i < 6; i++)
		s2[i] = src[i];
	nvgTransformMultiply(s2, dst);
	for (i = 0; i < 6; i++)
		dst[i] = s2[i];
}
/* 0 when src can't be inverted (dst then the identity), else 1 */
static inline int nvgTransformInverse(float *dst, const float *src) {
	double det = (double) src[0] * src[3] - (double) src[2] * src[1];
	if (det > -1e-6 && det < 1e-6) {
		nvgTransformIdentity(dst);
		return 0;
	}
	double inv = 1.0 / det;
	dst[0] = (float) (src[3] * inv);
	dst[2] = (float) (-src[2] * inv);
	dst[4] = (float) (((double) src[2] * src[5] - (double) src[3] * src[4]) * inv);
	dst[1] = (float) (-src[1] * inv);
	dst[3] = (float) (src[0] * inv);
	dst[5] = (float) (((double) src[1] * src[4] - (double) src[0] * src[5]) * inv);
	return 1;
}
static inline float nvgDegToRad(float deg) { return deg / 180.0f * 3.14159265358979323846f; }
static inline float nvgRadToDeg(float rad) { return rad / 3.14159265358979323846f * 180.0f; }

/* the SDK's own: a context that writes down what's drawn, and the list it wrote */
NVGcontext *oroboro_nvg_new(void);
void oroboro_nvg_free(NVGcontext *ctx);
void oroboro_nvg_begin(NVGcontext *ctx);
const float *oroboro_nvg_list(NVGcontext *ctx, unsigned int *count);
/* the id of the font of this file (the same for the same file) */
int oroboro_nvg_font(const char *filename);
/* one of the module's pictures (its number in its list), `width` by `height`
 * of its own pixels, drawn through the transform as it is now */
void oroboro_nvg_picture(NVGcontext *ctx, int index, float width, float height);

void nvgBeginFrame(NVGcontext *ctx, float windowWidth, float windowHeight, float devicePixelRatio);
void nvgCancelFrame(NVGcontext *ctx);
void nvgEndFrame(NVGcontext *ctx);
void nvgGlobalCompositeOperation(NVGcontext *ctx, int op);
void nvgGlobalCompositeBlendFunc(NVGcontext *ctx, int sfactor, int dfactor);
void nvgGlobalCompositeBlendFuncSeparate(NVGcontext *ctx, int srcRGB, int dstRGB, int srcAlpha, int dstAlpha);
void nvgSave(NVGcontext *ctx);
void nvgRestore(NVGcontext *ctx);
void nvgReset(NVGcontext *ctx);
void nvgShapeAntiAlias(NVGcontext *ctx, int enabled);
void nvgStrokeColor(NVGcontext *ctx, NVGcolor color);
void nvgStrokePaint(NVGcontext *ctx, NVGpaint paint);
void nvgFillColor(NVGcontext *ctx, NVGcolor color);
void nvgFillPaint(NVGcontext *ctx, NVGpaint paint);
void nvgMiterLimit(NVGcontext *ctx, float limit);
void nvgStrokeWidth(NVGcontext *ctx, float size);
void nvgLineCap(NVGcontext *ctx, int cap);
void nvgLineJoin(NVGcontext *ctx, int join);
void nvgGlobalAlpha(NVGcontext *ctx, float alpha);
void nvgGlobalTint(NVGcontext *ctx, NVGcolor tint);
NVGcolor nvgGetGlobalTint(NVGcontext *ctx);
void nvgAlpha(NVGcontext *ctx, float alpha);
void nvgTint(NVGcontext *ctx, NVGcolor tint);
void nvgResetTransform(NVGcontext *ctx);
void nvgTransform(NVGcontext *ctx, float a, float b, float c, float d, float e, float f);
void nvgTranslate(NVGcontext *ctx, float x, float y);
void nvgRotate(NVGcontext *ctx, float angle);
void nvgSkewX(NVGcontext *ctx, float angle);
void nvgSkewY(NVGcontext *ctx, float angle);
void nvgScale(NVGcontext *ctx, float x, float y);
void nvgCurrentTransform(NVGcontext *ctx, float *xform);
int nvgCreateImage(NVGcontext *ctx, const char *filename, int imageFlags);
int nvgCreateImageMem(NVGcontext *ctx, int imageFlags, unsigned char *data, int ndata);
int nvgCreateImageRGBA(NVGcontext *ctx, int w, int h, int imageFlags, const unsigned char *data);
void nvgUpdateImage(NVGcontext *ctx, int image, const unsigned char *data);
void nvgImageSize(NVGcontext *ctx, int image, int *w, int *h);
void nvgDeleteImage(NVGcontext *ctx, int image);
NVGpaint nvgLinearGradient(NVGcontext *ctx, float sx, float sy, float ex, float ey, NVGcolor icol, NVGcolor ocol);
NVGpaint nvgBoxGradient(NVGcontext *ctx, float x, float y, float w, float h, float r, float f, NVGcolor icol, NVGcolor ocol);
NVGpaint nvgRadialGradient(NVGcontext *ctx, float cx, float cy, float inr, float outr, NVGcolor icol, NVGcolor ocol);
NVGpaint nvgImagePattern(NVGcontext *ctx, float ox, float oy, float ex, float ey, float angle, int image, float alpha);
void nvgScissor(NVGcontext *ctx, float x, float y, float w, float h);
void nvgIntersectScissor(NVGcontext *ctx, float x, float y, float w, float h);
void nvgResetScissor(NVGcontext *ctx);
void nvgBeginPath(NVGcontext *ctx);
void nvgMoveTo(NVGcontext *ctx, float x, float y);
void nvgLineTo(NVGcontext *ctx, float x, float y);
void nvgBezierTo(NVGcontext *ctx, float c1x, float c1y, float c2x, float c2y, float x, float y);
void nvgQuadTo(NVGcontext *ctx, float cx, float cy, float x, float y);
void nvgArcTo(NVGcontext *ctx, float x1, float y1, float x2, float y2, float radius);
void nvgClosePath(NVGcontext *ctx);
void nvgPathWinding(NVGcontext *ctx, int dir);
void nvgArc(NVGcontext *ctx, float cx, float cy, float r, float a0, float a1, int dir);
void nvgRect(NVGcontext *ctx, float x, float y, float w, float h);
void nvgRoundedRect(NVGcontext *ctx, float x, float y, float w, float h, float r);
void nvgRoundedRectVarying(NVGcontext *ctx, float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight,
	float radBottomLeft);
void nvgEllipse(NVGcontext *ctx, float cx, float cy, float rx, float ry);
void nvgCircle(NVGcontext *ctx, float cx, float cy, float r);
void nvgFill(NVGcontext *ctx);
void nvgStroke(NVGcontext *ctx);
int nvgCreateFont(NVGcontext *ctx, const char *name, const char *filename);
int nvgCreateFontMem(NVGcontext *ctx, const char *name, unsigned char *data, int ndata, int freeData);
int nvgFindFont(NVGcontext *ctx, const char *name);
int nvgAddFallbackFontId(NVGcontext *ctx, int baseFont, int fallbackFont);
void nvgFontSize(NVGcontext *ctx, float size);
void nvgFontBlur(NVGcontext *ctx, float blur);
void nvgTextLetterSpacing(NVGcontext *ctx, float spacing);
void nvgTextLineHeight(NVGcontext *ctx, float lineHeight);
void nvgTextAlign(NVGcontext *ctx, int align);
void nvgFontFaceId(NVGcontext *ctx, int font);
void nvgFontFace(NVGcontext *ctx, const char *font);
float nvgText(NVGcontext *ctx, float x, float y, const char *string, const char *end);
void nvgTextBox(NVGcontext *ctx, float x, float y, float breakRowWidth, const char *string, const char *end);
float nvgTextBounds(NVGcontext *ctx, float x, float y, const char *string, const char *end, float *bounds);
void nvgTextBoxBounds(NVGcontext *ctx, float x, float y, float breakRowWidth, const char *string, const char *end, float *bounds);
int nvgTextGlyphPositions(NVGcontext *ctx, float x, float y, const char *string, const char *end, NVGglyphPosition *positions, int maxPositions);
void nvgTextMetrics(NVGcontext *ctx, float *ascender, float *descender, float *lineh);
int nvgTextBreakLines(NVGcontext *ctx, const char *string, const char *end, float breakRowWidth, NVGtextRow *rows, int maxRows);

#ifdef __cplusplus
}
#endif

#endif
