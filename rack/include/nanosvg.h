// SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-Oroboro-Rack-Bridge-Exception (rack/LICENSE-EXCEPTION.md)
// nanosvg's interface, as Rack plugins reach it (Rack draws SVGs with
// nanosvg, and a plugin may read a picture's shapes through
// `window::Svg::handle`, or parse an SVG itself). The SDK's own
// declarations, not nanosvg: the plugin draws a module's SVGs (`oromod rack
// build` packs them), so what's parsed here is a picture's size. An image
// has no shapes: code that walks them finds none.
#ifndef NANOSVG_H
#define NANOSVG_H

#ifdef __cplusplus
extern "C" {
#endif

enum NSVGpaintType {
	NSVG_PAINT_UNDEF = -1,
	NSVG_PAINT_NONE = 0,
	NSVG_PAINT_COLOR = 1,
	NSVG_PAINT_LINEAR_GRADIENT = 2,
	NSVG_PAINT_RADIAL_GRADIENT = 3,
};
enum NSVGspreadType {
	NSVG_SPREAD_PAD = 0,
	NSVG_SPREAD_REFLECT = 1,
	NSVG_SPREAD_REPEAT = 2,
};
enum NSVGlineJoin {
	NSVG_JOIN_MITER = 0,
	NSVG_JOIN_ROUND = 1,
	NSVG_JOIN_BEVEL = 2,
};
enum NSVGlineCap {
	NSVG_CAP_BUTT = 0,
	NSVG_CAP_ROUND = 1,
	NSVG_CAP_SQUARE = 2,
};
enum NSVGfillRule {
	NSVG_FILLRULE_NONZERO = 0,
	NSVG_FILLRULE_EVENODD = 1,
};
enum NSVGflags {
	NSVG_FLAGS_VISIBLE = 0x01,
};

typedef struct NSVGgradientStop {
	unsigned int color; // ABGR
	float offset;
} NSVGgradientStop;

typedef struct NSVGgradient {
	float xform[6];
	char spread;
	float fx, fy;
	int nstops;
	NSVGgradientStop stops[1];
} NSVGgradient;

typedef struct NSVGpaint {
	signed char type;
	union {
		unsigned int color; // ABGR
		NSVGgradient* gradient;
	};
} NSVGpaint;

/// A path: cubic Béziers, a start point then three points a curve.
typedef struct NSVGpath {
	float* pts;
	int npts;
	char closed;
	float bounds[4];
	struct NSVGpath* next;
} NSVGpath;

typedef struct NSVGshape {
	char id[64];
	NSVGpaint fill;
	NSVGpaint stroke;
	float opacity;
	float strokeWidth;
	float strokeDashOffset;
	float strokeDashArray[8];
	char strokeDashCount;
	char strokeLineJoin;
	char strokeLineCap;
	float miterLimit;
	char fillRule;
	unsigned char flags;
	float bounds[4];
	char fillGradient[64];
	char strokeGradient[64];
	float xform[6];
	NSVGpath* paths;
	struct NSVGshape* next;
} NSVGshape;

/// A picture: its size (in `units` at `dpi`, as it was parsed), and its
/// shapes (none here).
typedef struct NSVGimage {
#ifdef __cplusplus
	float width = 0.f;
	float height = 0.f;
	NSVGshape* shapes = nullptr;
#else
	float width;
	float height;
	NSVGshape* shapes;
#endif
} NSVGimage;

/// An SVG file's picture (null if it can't be read). `units`: "px" (at
/// `dpi`), "pt", "pc", "mm", "cm" or "in".
NSVGimage* nsvgParseFromFile(const char* filename, const char* units, float dpi);
/// An SVG's text's picture (the text may be changed, as nanosvg's).
NSVGimage* nsvgParse(char* input, const char* units, float dpi);
NSVGpath* nsvgDuplicatePath(NSVGpath* p);
void nsvgDelete(NSVGimage* image);

#ifdef __cplusplus
}
#endif

#endif
