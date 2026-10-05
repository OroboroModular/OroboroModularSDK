// SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-Oroboro-Rack-Bridge-Exception (rack/LICENSE-EXCEPTION.md)
/* blendish's interface, as Rack bundles it (Rack's menus and some plugins'
   widgets draw with it): the Oroboro Modular SDK's own code to the same
   names. It draws plainly through NanoVG: texts are measured and drawn
   as they are, boxes and buttons are filled rounded rectangles in the
   theme's colours, and icons aren't drawn (BND_ICON_NONE is the one icon
   named here). */
#ifndef BLENDISH_H
#define BLENDISH_H

#include <math.h>
#include <string.h>

#include "nanovg.h"

#ifndef BND_EXPORT
#define BND_EXPORT
#endif

typedef struct BNDwidgetTheme {
	NVGcolor outlineColor;
	NVGcolor itemColor;
	NVGcolor innerColor;
	NVGcolor innerSelectedColor;
	NVGcolor textColor;
	NVGcolor textSelectedColor;
	int shadeTop;
	int shadeDown;
} BNDwidgetTheme;

typedef struct BNDnodeTheme {
	NVGcolor nodeSelectedColor;
	NVGcolor wiresColor;
	NVGcolor textSelectedColor;
	NVGcolor activeNodeColor;
	NVGcolor wireSelectColor;
	NVGcolor nodeBackdropColor;
	int noodleCurving;
} BNDnodeTheme;

typedef struct BNDtheme {
	NVGcolor backgroundColor;
	BNDwidgetTheme regularTheme;
	BNDwidgetTheme toolTheme;
	BNDwidgetTheme radioTheme;
	BNDwidgetTheme textFieldTheme;
	BNDwidgetTheme optionTheme;
	BNDwidgetTheme choiceTheme;
	BNDwidgetTheme numberFieldTheme;
	BNDwidgetTheme sliderTheme;
	BNDwidgetTheme scrollBarTheme;
	BNDwidgetTheme tooltipTheme;
	BNDwidgetTheme menuTheme;
	BNDwidgetTheme menuItemTheme;
	BNDnodeTheme nodeTheme;
} BNDtheme;

typedef enum BNDtextAlignment { BND_LEFT = 0, BND_CENTER } BNDtextAlignment;
typedef enum BNDwidgetState { BND_DEFAULT = 0, BND_HOVER, BND_ACTIVE } BNDwidgetState;
typedef enum BNDcornerFlags {
	BND_CORNER_NONE = 0,
	BND_CORNER_TOP_LEFT = 1,
	BND_CORNER_TOP_RIGHT = 2,
	BND_CORNER_DOWN_RIGHT = 4,
	BND_CORNER_DOWN_LEFT = 8,
	BND_CORNER_ALL = 0xF,
	BND_CORNER_TOP = 3,
	BND_CORNER_DOWN = 0xC,
	BND_CORNER_LEFT = 9,
	BND_CORNER_RIGHT = 6
} BNDcornerFlags;

#define BND_ICONID(x, y) ((x) | ((y) << 8))
typedef enum BNDicon { BND_ICON_NONE = -1 } BNDicon;

#define BND_DISABLED_ALPHA 0.5
#define BND_WIDGET_HEIGHT 21
#define BND_TOOL_WIDTH 20
#define BND_NODE_PORT_RADIUS 5
#define BND_NODE_MARGIN_TOP 25
#define BND_NODE_MARGIN_DOWN 5
#define BND_NODE_MARGIN_SIDE 10
#define BND_NODE_TITLE_HEIGHT 20
#define BND_NODE_ARROW_AREA_WIDTH 20
#define BND_SPLITTER_AREA_SIZE 12
#define BND_SCROLLBAR_WIDTH 13
#define BND_SCROLLBAR_HEIGHT 14
#define BND_VSPACING 1
#define BND_VSPACING_GROUP 8
#define BND_HSPACING 8
#define BND_LABEL_FONT_SIZE 13
#define BND_PAD_LEFT 8
#define BND_PAD_RIGHT 8
#define BND_LABEL_SEPARATOR ": "
#define BND_TRANSPARENT_ALPHA 0.9
#define BND_BEVEL_SHADE 30
#define BND_INSET_BEVEL_SHADE 30
#define BND_HOVER_SHADE 15
#define BND_SPLITTER_SHADE 100
#define BND_ICON_SHEET_WIDTH 602
#define BND_ICON_SHEET_HEIGHT 640
#define BND_ICON_SHEET_GRID 21
#define BND_ICON_SHEET_OFFSET_X 5
#define BND_ICON_SHEET_OFFSET_Y 10
#define BND_ICON_SHEET_RES 16
#define BND_NUMBER_ARROW_SIZE 4
#define BND_TOOL_RADIUS 4
#define BND_OPTION_RADIUS 4
#define BND_OPTION_WIDTH 14
#define BND_OPTION_HEIGHT 15
#define BND_TEXT_RADIUS 4
#define BND_NUMBER_RADIUS 10
#define BND_MENU_RADIUS 3
#define BND_SHADOW_FEATHER 12
#define BND_SHADOW_ALPHA 0.5
#define BND_SCROLLBAR_RADIUS 7
#define BND_SCROLLBAR_ACTIVE_SHADE 15
#define BND_MAX_GLYPHS 1024
#define BND_MAX_ROWS 32
#define BND_TEXT_PAD_DOWN 7
#define BND_NODE_WIRE_OUTLINE_WIDTH 4
#define BND_NODE_WIRE_WIDTH 2
#define BND_NODE_RADIUS 8
#define BND_NODE_TITLE_FEATHER 1
#define BND_NODE_ARROW_SIZE 9

static inline void bndCheck(NVGcontext* ctx, float ox, float oy, NVGcolor color);

/* the theme and the font, one of each */
static inline BNDwidgetTheme bnd_widget_theme_(NVGcolor inner, NVGcolor text) {
	BNDwidgetTheme t;
	t.outlineColor = nvgRGB(0x18, 0x18, 0x18);
	t.itemColor = nvgRGB(0xe0, 0xe0, 0xe0);
	t.innerColor = inner;
	t.innerSelectedColor = nvgRGB(0x56, 0x80, 0xc2);
	t.textColor = text;
	t.textSelectedColor = nvgRGB(0xff, 0xff, 0xff);
	t.shadeTop = 0;
	t.shadeDown = 0;
	return t;
}
static inline BNDtheme* bnd_theme_(void) {
	static BNDtheme theme;
	static int ready = 0;
	if (!ready) {
		NVGcolor dark = nvgRGB(0x2a, 0x2a, 0x2a), light = nvgRGB(0xe6, 0xe6, 0xe6);
		BNDwidgetTheme w = bnd_widget_theme_(dark, light);
		theme.backgroundColor = nvgRGB(0x20, 0x20, 0x20);
		theme.regularTheme = theme.toolTheme = theme.radioTheme = theme.textFieldTheme = w;
		theme.optionTheme = theme.choiceTheme = theme.numberFieldTheme = theme.sliderTheme = w;
		theme.scrollBarTheme = theme.tooltipTheme = theme.menuTheme = theme.menuItemTheme = w;
		theme.nodeTheme.nodeSelectedColor = nvgRGB(0xf0, 0x8c, 0x22);
		theme.nodeTheme.wiresColor = nvgRGB(0x80, 0x80, 0x80);
		theme.nodeTheme.textSelectedColor = nvgRGB(0xff, 0xff, 0xff);
		theme.nodeTheme.activeNodeColor = nvgRGB(0xf0, 0x8c, 0x22);
		theme.nodeTheme.wireSelectColor = nvgRGB(0xff, 0xff, 0xff);
		theme.nodeTheme.nodeBackdropColor = nvgRGBA(0x60, 0x60, 0x60, 0xa0);
		theme.nodeTheme.noodleCurving = 5;
		ready = 1;
	}
	return &theme;
}
static inline int* bnd_font_(void) {
	static int font = -1;
	return &font;
}

static inline void bndSetTheme(BNDtheme theme) { *bnd_theme_() = theme; }
static inline const BNDtheme* bndGetTheme(void) { return bnd_theme_(); }
static inline void bndSetIconImage(int image) { (void) image; }
static inline void bndSetFont(int font) { *bnd_font_() = font; }

/* colours */
static inline NVGcolor bndTransparent(NVGcolor color) {
	color.a *= (float) BND_TRANSPARENT_ALPHA;
	return color;
}
static inline float bnd_clamp_(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
static inline NVGcolor bndOffsetColor(NVGcolor color, int delta) {
	float d = delta / 255.f;
	return delta ? nvgRGBAf(bnd_clamp_(color.r + d, 0.f, 1.f), bnd_clamp_(color.g + d, 0.f, 1.f), bnd_clamp_(color.b + d, 0.f, 1.f), color.a)
		: color;
}
static inline void bndSelectCorners(float* radiuses, float r, int flags) {
	radiuses[0] = (flags & BND_CORNER_TOP_LEFT) ? 0 : r;
	radiuses[1] = (flags & BND_CORNER_TOP_RIGHT) ? 0 : r;
	radiuses[2] = (flags & BND_CORNER_DOWN_RIGHT) ? 0 : r;
	radiuses[3] = (flags & BND_CORNER_DOWN_LEFT) ? 0 : r;
}
static inline void bndInnerColors(NVGcolor* shade_top, NVGcolor* shade_down, const BNDwidgetTheme* theme, BNDwidgetState state,
	int flipActive) {
	NVGcolor inner = state == BND_ACTIVE ? theme->innerSelectedColor : theme->innerColor;
	if (state == BND_HOVER)
		inner = bndOffsetColor(inner, BND_HOVER_SHADE);
	int top = (state == BND_ACTIVE && flipActive) ? theme->shadeDown : theme->shadeTop;
	int down = (state == BND_ACTIVE && flipActive) ? theme->shadeTop : theme->shadeDown;
	*shade_top = bndOffsetColor(inner, top);
	*shade_down = bndOffsetColor(inner, down);
}
static inline NVGcolor bndTextColor(const BNDwidgetTheme* theme, BNDwidgetState state) {
	return state == BND_ACTIVE ? theme->textSelectedColor : theme->textColor;
}
static inline NVGcolor bndNodeWireColor(const BNDnodeTheme* theme, BNDwidgetState state) {
	return state == BND_ACTIVE ? theme->activeNodeColor : (state == BND_HOVER ? theme->wireSelectColor : theme->wiresColor);
}
static inline void bndScrollHandleRect(float* x, float* y, float* w, float* h, float offset, float size) {
	size = bnd_clamp_(size, 0.f, 1.f);
	offset = bnd_clamp_(offset, 0.f, 1.f);
	if (*h > *w) {
		float hs = (*h > *h * size) ? *h * size : *h;
		if (hs < *w)
			hs = *w;
		*y = *y + (*h - hs) * offset;
		*h = hs;
	} else {
		float ws = *w * size;
		if (ws < *h)
			ws = *h;
		*x = *x + (*w - ws) * offset;
		*w = ws;
	}
}

/* shapes */
static inline void bndRoundedBox(NVGcontext* ctx, float x, float y, float w, float h, float cr0, float cr1, float cr2, float cr3) {
	nvgRoundedRectVarying(ctx, x, y, w, h, cr0, cr1, cr2, cr3);
}
static inline void bndBackground(NVGcontext* ctx, float x, float y, float w, float h) {
	nvgBeginPath(ctx);
	nvgRect(ctx, x, y, w, h);
	nvgFillColor(ctx, bnd_theme_()->backgroundColor);
	nvgFill(ctx);
}
static inline void bndBevel(NVGcontext* ctx, float x, float y, float w, float h) {
	nvgStrokeWidth(ctx, 1);
	nvgBeginPath(ctx);
	nvgMoveTo(ctx, x + 0.5f, y + h - 0.5f);
	nvgLineTo(ctx, x + w - 0.5f, y + h - 0.5f);
	nvgLineTo(ctx, x + w - 0.5f, y + 0.5f);
	nvgStrokeColor(ctx, bndTransparent(bndOffsetColor(bnd_theme_()->backgroundColor, -BND_BEVEL_SHADE)));
	nvgStroke(ctx);
}
static inline void bndBevelInset(NVGcontext* ctx, float x, float y, float w, float h, float cr2, float cr3) {
	(void) cr2;
	(void) cr3;
	bndBevel(ctx, x, y, w, h);
}
static inline void bndIcon(NVGcontext* ctx, float x, float y, int iconid) {
	(void) ctx;
	(void) x;
	(void) y;
	(void) iconid;
}
static inline void bndDropShadow(NVGcontext* ctx, float x, float y, float w, float h, float r, float feather, float alpha) {
	nvgBeginPath(ctx);
	nvgRoundedRect(ctx, x, y + feather * 0.5f, w, h, r);
	nvgFillColor(ctx, nvgRGBAf(0, 0, 0, alpha * 0.5f));
	nvgFill(ctx);
}
static inline void bndInnerBox(NVGcontext* ctx, float x, float y, float w, float h, float cr0, float cr1, float cr2, float cr3,
	NVGcolor shade_top, NVGcolor shade_down) {
	nvgBeginPath(ctx);
	bndRoundedBox(ctx, x + 1, y + 1, w - 2, h - 3, cr0 > 1 ? cr0 - 1 : 0, cr1 > 1 ? cr1 - 1 : 0, cr2 > 1 ? cr2 - 1 : 0,
		cr3 > 1 ? cr3 - 1 : 0);
	nvgFillPaint(ctx, nvgLinearGradient(ctx, x, y, x, y + h, shade_top, shade_down));
	nvgFill(ctx);
}
static inline void bndOutlineBox(NVGcontext* ctx, float x, float y, float w, float h, float cr0, float cr1, float cr2, float cr3,
	NVGcolor color) {
	nvgBeginPath(ctx);
	bndRoundedBox(ctx, x + 0.5f, y + 0.5f, w - 1, h - 2, cr0, cr1, cr2, cr3);
	nvgStrokeColor(ctx, color);
	nvgStrokeWidth(ctx, 1);
	nvgStroke(ctx);
}

/* texts */
static inline void bnd_text_style_(NVGcontext* ctx, float fontsize) {
	if (*bnd_font_() >= 0)
		nvgFontFaceId(ctx, *bnd_font_());
	nvgFontSize(ctx, fontsize);
}
static inline void bndIconLabelValue(NVGcontext* ctx, float x, float y, float w, float h, int iconid, NVGcolor color, int align,
	float fontsize, const char* label, const char* value) {
	(void) iconid;
	if (!label)
		return;
	float pleft = BND_PAD_LEFT;
	bnd_text_style_(ctx, fontsize);
	nvgFillColor(ctx, color);
	if (value) {
		float label_width = nvgTextBounds(ctx, 1, 1, label, NULL, NULL);
		float sep_width = nvgTextBounds(ctx, 1, 1, BND_LABEL_SEPARATOR, NULL, NULL);
		nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		x += pleft;
		if (align == BND_CENTER) {
			float width = label_width + sep_width + nvgTextBounds(ctx, 1, 1, value, NULL, NULL);
			x += ((w - BND_PAD_RIGHT - pleft) - width) * 0.5f;
		}
		y += h * 0.5f;
		nvgText(ctx, x, y, label, NULL);
		x += label_width;
		nvgText(ctx, x, y, BND_LABEL_SEPARATOR, NULL);
		x += sep_width;
		nvgText(ctx, x, y, value, NULL);
	} else {
		nvgTextAlign(ctx, (align == BND_LEFT ? NVG_ALIGN_LEFT : NVG_ALIGN_CENTER) | NVG_ALIGN_MIDDLE);
		float tx = align == BND_LEFT ? x + pleft : x + w * 0.5f;
		nvgText(ctx, tx, y + h * 0.5f, label, NULL);
	}
}
static inline void bndNodeIconLabel(NVGcontext* ctx, float x, float y, float w, float h, int iconid, NVGcolor color,
	NVGcolor shadowColor, int align, float fontsize, const char* label) {
	(void) shadowColor;
	bndIconLabelValue(ctx, x, y, w, h, iconid, color, align, fontsize, label, NULL);
}
/* the character under (px, py): from the glyphs' positions */
static inline int bndIconLabelTextPosition(NVGcontext* ctx, float x, float y, float w, float h, int iconid, float fontsize,
	const char* label, int px, int py) {
	(void) y;
	(void) w;
	(void) h;
	(void) iconid;
	(void) py;
	if (!label)
		return -1;
	NVGglyphPosition glyphs[BND_MAX_GLYPHS];
	bnd_text_style_(ctx, fontsize);
	nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	int n = nvgTextGlyphPositions(ctx, x + BND_PAD_LEFT, 0, label, NULL, glyphs, BND_MAX_GLYPHS);
	int length = (int) strlen(label);
	for (int i = 0; i < n; i++) {
		if (px < (glyphs[i].minx + glyphs[i].maxx) * 0.5f)
			return (int) (glyphs[i].str - label);
	}
	return length;
}
static inline void bndIconLabelCaret(NVGcontext* ctx, float x, float y, float w, float h, int iconid, NVGcolor color, float fontsize,
	const char* label, NVGcolor caretcolor, int cbegin, int cend) {
	(void) caretcolor;
	(void) cbegin;
	(void) cend;
	bndIconLabelValue(ctx, x, y, w, h, iconid, color, BND_LEFT, fontsize, label, NULL);
}
static inline float bndLabelWidth(NVGcontext* ctx, int iconid, const char* label) {
	float w = BND_PAD_LEFT + BND_PAD_RIGHT;
	if (iconid >= 0)
		w += BND_ICON_SHEET_RES;
	if (label) {
		bnd_text_style_(ctx, BND_LABEL_FONT_SIZE);
		w += nvgTextBounds(ctx, 1, 1, label, NULL, NULL);
	}
	return w;
}
static inline float bndLabelHeight(NVGcontext* ctx, int iconid, const char* label, float width) {
	float h = BND_WIDGET_HEIGHT;
	width -= BND_TEXT_RADIUS * 2;
	if (iconid >= 0)
		width -= BND_ICON_SHEET_RES;
	if (label && width > 0) {
		float bounds[4];
		bnd_text_style_(ctx, BND_LABEL_FONT_SIZE);
		nvgTextBoxBounds(ctx, 1, 1, width, label, NULL, bounds);
		float bh = bounds[3] - bounds[1] + BND_TEXT_PAD_DOWN;
		if (bh > h)
			h = bh;
	}
	return h;
}

/* widgets: a box in the theme's colours, its text over it */
static inline void bnd_widget_(NVGcontext* ctx, float x, float y, float w, float h, int flags, BNDwidgetState state,
	const BNDwidgetTheme* theme, float radius, int iconid, const char* label, const char* value, int align) {
	float cr[4];
	NVGcolor top, down;
	bndSelectCorners(cr, radius, flags);
	bndInnerColors(&top, &down, theme, state, 1);
	bndInnerBox(ctx, x, y, w, h, cr[0], cr[1], cr[2], cr[3], top, down);
	bndOutlineBox(ctx, x, y, w, h, cr[0], cr[1], cr[2], cr[3], bndTransparent(theme->outlineColor));
	bndIconLabelValue(ctx, x, y, w, h, iconid, bndTextColor(theme, state), align, BND_LABEL_FONT_SIZE, label, value);
}
static inline void bndLabel(NVGcontext* ctx, float x, float y, float w, float h, int iconid, const char* label) {
	bndIconLabelValue(ctx, x, y, w, h, iconid, bnd_theme_()->regularTheme.textColor, BND_LEFT, BND_LABEL_FONT_SIZE, label, NULL);
}
static inline void bndToolButton(NVGcontext* ctx, float x, float y, float w, float h, int flags, BNDwidgetState state, int iconid,
	const char* label) {
	bnd_widget_(ctx, x, y, w, h, flags, state, &bnd_theme_()->toolTheme, BND_TOOL_RADIUS, iconid, label, NULL, BND_CENTER);
}
static inline void bndRadioButton(NVGcontext* ctx, float x, float y, float w, float h, int flags, BNDwidgetState state, int iconid,
	const char* label) {
	bnd_widget_(ctx, x, y, w, h, flags, state, &bnd_theme_()->radioTheme, BND_OPTION_RADIUS, iconid, label, NULL, BND_CENTER);
}
static inline int bndTextFieldTextPosition(NVGcontext* ctx, float x, float y, float w, float h, int iconid, const char* text, int px,
	int py) {
	return bndIconLabelTextPosition(ctx, x, y, w, h, iconid, BND_LABEL_FONT_SIZE, text, px, py);
}
static inline void bndTextField(NVGcontext* ctx, float x, float y, float w, float h, int flags, BNDwidgetState state, int iconid,
	const char* text, int cbegin, int cend) {
	(void) cbegin;
	(void) cend;
	bnd_widget_(ctx, x, y, w, h, flags, state, &bnd_theme_()->textFieldTheme, BND_TEXT_RADIUS, iconid, text, NULL, BND_LEFT);
}
static inline void bndOptionButton(NVGcontext* ctx, float x, float y, float w, float h, BNDwidgetState state, const char* label) {
	const BNDwidgetTheme* theme = &bnd_theme_()->optionTheme;
	float ox = x, oy = y + h - BND_OPTION_HEIGHT - 3;
	NVGcolor top, down;
	bndInnerColors(&top, &down, theme, state, 1);
	bndInnerBox(ctx, ox, oy, BND_OPTION_WIDTH, BND_OPTION_HEIGHT, BND_OPTION_RADIUS, BND_OPTION_RADIUS, BND_OPTION_RADIUS,
		BND_OPTION_RADIUS, top, down);
	if (state == BND_ACTIVE)
		bndCheck(ctx, ox, oy, bndTransparent(theme->itemColor));
	bndIconLabelValue(ctx, x + 12, y, w - 12, h, -1, bndTextColor(theme, state), BND_LEFT, BND_LABEL_FONT_SIZE, label, NULL);
}
static inline void bndChoiceButton(NVGcontext* ctx, float x, float y, float w, float h, int flags, BNDwidgetState state, int iconid,
	const char* label) {
	bnd_widget_(ctx, x, y, w, h, flags, state, &bnd_theme_()->choiceTheme, BND_OPTION_RADIUS, iconid, label, NULL, BND_LEFT);
}
static inline void bndColorButton(NVGcontext* ctx, float x, float y, float w, float h, int flags, NVGcolor color) {
	float cr[4];
	bndSelectCorners(cr, BND_TOOL_RADIUS, flags);
	bndInnerBox(ctx, x, y, w, h, cr[0], cr[1], cr[2], cr[3], color, color);
	bndOutlineBox(ctx, x, y, w, h, cr[0], cr[1], cr[2], cr[3], bndTransparent(bnd_theme_()->toolTheme.outlineColor));
}
static inline void bndNumberField(NVGcontext* ctx, float x, float y, float w, float h, int flags, BNDwidgetState state,
	const char* label, const char* value) {
	bnd_widget_(ctx, x, y, w, h, flags, state, &bnd_theme_()->numberFieldTheme, BND_NUMBER_RADIUS, -1, label, value, BND_CENTER);
}
static inline void bndSlider(NVGcontext* ctx, float x, float y, float w, float h, int flags, BNDwidgetState state, float progress,
	const char* label, const char* value) {
	const BNDwidgetTheme* theme = &bnd_theme_()->sliderTheme;
	float cr[4];
	NVGcolor top, down;
	bndSelectCorners(cr, BND_NUMBER_RADIUS, flags);
	bndInnerColors(&top, &down, theme, state, 0);
	bndInnerBox(ctx, x, y, w, h, cr[0], cr[1], cr[2], cr[3], top, down);
	nvgSave(ctx);
	nvgScissor(ctx, x, y, 8 + (w - 8) * bnd_clamp_(progress, 0.f, 1.f), h);
	bndInnerBox(ctx, x, y, w, h, cr[0], cr[1], cr[2], cr[3], theme->itemColor, theme->itemColor);
	nvgRestore(ctx);
	bndOutlineBox(ctx, x, y, w, h, cr[0], cr[1], cr[2], cr[3], bndTransparent(theme->outlineColor));
	bndIconLabelValue(ctx, x, y, w, h, -1, bndTextColor(theme, state), BND_CENTER, BND_LABEL_FONT_SIZE, label, value);
}
static inline void bndScrollBar(NVGcontext* ctx, float x, float y, float w, float h, BNDwidgetState state, float offset, float size) {
	const BNDwidgetTheme* theme = &bnd_theme_()->scrollBarTheme;
	bndInnerBox(ctx, x, y, w, h, BND_SCROLLBAR_RADIUS, BND_SCROLLBAR_RADIUS, BND_SCROLLBAR_RADIUS, BND_SCROLLBAR_RADIUS,
		bndOffsetColor(theme->innerColor, 3 * theme->shadeDown), bndOffsetColor(theme->innerColor, 3 * theme->shadeTop));
	bndScrollHandleRect(&x, &y, &w, &h, offset, size);
	NVGcolor item = state == BND_ACTIVE ? bndOffsetColor(theme->itemColor, BND_SCROLLBAR_ACTIVE_SHADE) : theme->itemColor;
	bndInnerBox(ctx, x, y, w, h, BND_SCROLLBAR_RADIUS, BND_SCROLLBAR_RADIUS, BND_SCROLLBAR_RADIUS, BND_SCROLLBAR_RADIUS, item, item);
}
static inline void bndMenuBackground(NVGcontext* ctx, float x, float y, float w, float h, int flags) {
	const BNDwidgetTheme* theme = &bnd_theme_()->menuTheme;
	float cr[4];
	NVGcolor top, down;
	bndSelectCorners(cr, BND_MENU_RADIUS, flags);
	bndInnerColors(&top, &down, theme, BND_DEFAULT, 0);
	bndInnerBox(ctx, x, y, w, h + 1, cr[0], cr[1], cr[2], cr[3], top, down);
	bndOutlineBox(ctx, x, y, w, h + 1, cr[0], cr[1], cr[2], cr[3], bndTransparent(theme->outlineColor));
}
static inline void bndMenuLabel(NVGcontext* ctx, float x, float y, float w, float h, int iconid, const char* label) {
	bndIconLabelValue(ctx, x, y, w, h, iconid, bnd_theme_()->menuTheme.textColor, BND_LEFT, BND_LABEL_FONT_SIZE, label, NULL);
}
static inline void bndMenuItem(NVGcontext* ctx, float x, float y, float w, float h, BNDwidgetState state, int iconid,
	const char* label) {
	const BNDwidgetTheme* theme = &bnd_theme_()->menuItemTheme;
	if (state != BND_DEFAULT) {
		NVGcolor c = theme->innerSelectedColor;
		bndInnerBox(ctx, x, y, w, h, 0, 0, 0, 0, c, c);
		state = BND_ACTIVE;
	}
	bndIconLabelValue(ctx, x, y, w, h, iconid, bndTextColor(theme, state), BND_LEFT, BND_LABEL_FONT_SIZE, label, NULL);
}
static inline void bndTooltipBackground(NVGcontext* ctx, float x, float y, float w, float h) {
	const BNDwidgetTheme* theme = &bnd_theme_()->tooltipTheme;
	NVGcolor top, down;
	bndInnerColors(&top, &down, theme, BND_DEFAULT, 0);
	bndInnerBox(ctx, x, y, w, h + 1, BND_MENU_RADIUS, BND_MENU_RADIUS, BND_MENU_RADIUS, BND_MENU_RADIUS, top, down);
	bndOutlineBox(ctx, x, y, w, h + 1, BND_MENU_RADIUS, BND_MENU_RADIUS, BND_MENU_RADIUS, BND_MENU_RADIUS,
		bndTransparent(theme->outlineColor));
}

/* nodes and wires */
static inline void bndNodePort(NVGcontext* ctx, float x, float y, BNDwidgetState state, NVGcolor color) {
	nvgBeginPath(ctx);
	nvgCircle(ctx, x, y, BND_NODE_PORT_RADIUS);
	nvgStrokeColor(ctx, bnd_theme_()->nodeTheme.wiresColor);
	nvgStrokeWidth(ctx, 1.0f);
	nvgStroke(ctx);
	nvgFillColor(ctx, state != BND_DEFAULT ? bndOffsetColor(color, BND_HOVER_SHADE) : color);
	nvgFill(ctx);
}
static inline void bndColoredNodeWire(NVGcontext* ctx, float x0, float y0, float x1, float y1, NVGcolor color0, NVGcolor color1) {
	float length = fabsf(x1 - x0) > 0 ? fabsf(x1 - x0) : 0;
	float delta = length * (float) bnd_theme_()->nodeTheme.noodleCurving / 10.0f;
	nvgBeginPath(ctx);
	nvgMoveTo(ctx, x0, y0);
	nvgBezierTo(ctx, x0 + delta, y0, x1 - delta, y1, x1, y1);
	nvgStrokeColor(ctx, nvgLerpRGBA(color0, color1, 0.5f));
	nvgStrokeWidth(ctx, BND_NODE_WIRE_WIDTH);
	nvgStroke(ctx);
}
static inline void bndNodeWire(NVGcontext* ctx, float x0, float y0, float x1, float y1, BNDwidgetState state0, BNDwidgetState state1) {
	bndColoredNodeWire(ctx, x0, y0, x1, y1, bndNodeWireColor(&bnd_theme_()->nodeTheme, state0),
		bndNodeWireColor(&bnd_theme_()->nodeTheme, state1));
}
static inline void bndNodeBackground(NVGcontext* ctx, float x, float y, float w, float h, BNDwidgetState state, int iconid,
	const char* label, NVGcolor titleColor) {
	bndInnerBox(ctx, x, y, w, BND_NODE_TITLE_HEIGHT + 2, BND_NODE_RADIUS, BND_NODE_RADIUS, 0, 0, bndTransparent(titleColor),
		bndTransparent(titleColor));
	bndInnerBox(ctx, x, y + BND_NODE_TITLE_HEIGHT - 1, w, h + 2 - BND_NODE_TITLE_HEIGHT, 0, 0, BND_NODE_RADIUS, BND_NODE_RADIUS,
		bndTransparent(bnd_theme_()->nodeTheme.nodeBackdropColor), bndTransparent(bnd_theme_()->nodeTheme.nodeBackdropColor));
	bndNodeIconLabel(ctx, x + BND_NODE_ARROW_AREA_WIDTH, y, w - BND_NODE_ARROW_AREA_WIDTH - BND_NODE_MARGIN_SIDE,
		BND_NODE_TITLE_HEIGHT, iconid, bnd_theme_()->regularTheme.textColor, bndOffsetColor(titleColor, BND_BEVEL_SHADE), BND_LEFT,
		BND_LABEL_FONT_SIZE, label);
	(void) state;
}
static inline void bndSplitterWidgets(NVGcontext* ctx, float x, float y, float w, float h) {
	(void) ctx;
	(void) x;
	(void) y;
	(void) w;
	(void) h;
}
static inline void bndJoinAreaOverlay(NVGcontext* ctx, float x, float y, float w, float h, int vertical, int mirror) {
	(void) vertical;
	(void) mirror;
	nvgBeginPath(ctx);
	nvgRect(ctx, x, y, w, h);
	nvgFillColor(ctx, nvgRGBAf(0, 0, 0, 0.3f));
	nvgFill(ctx);
}

/* little marks */
static inline void bndCheck(NVGcontext* ctx, float ox, float oy, NVGcolor color) {
	nvgBeginPath(ctx);
	nvgStrokeWidth(ctx, 2);
	nvgStrokeColor(ctx, color);
	nvgMoveTo(ctx, ox + 4, oy + 5);
	nvgLineTo(ctx, ox + 7, oy + 8);
	nvgLineTo(ctx, ox + 14, oy + 1);
	nvgStroke(ctx);
}
static inline void bndArrow(NVGcontext* ctx, float x, float y, float s, NVGcolor color) {
	nvgBeginPath(ctx);
	nvgMoveTo(ctx, x, y);
	nvgLineTo(ctx, x - s, y + s);
	nvgLineTo(ctx, x - s, y - s);
	nvgClosePath(ctx);
	nvgFillColor(ctx, color);
	nvgFill(ctx);
}
static inline void bndUpDownArrow(NVGcontext* ctx, float x, float y, float s, NVGcolor color) {
	float w = 1.1f * s;
	nvgBeginPath(ctx);
	nvgMoveTo(ctx, x, y - 1);
	nvgLineTo(ctx, x + 0.5f * w, y - s - 1);
	nvgLineTo(ctx, x + w, y - 1);
	nvgClosePath(ctx);
	nvgMoveTo(ctx, x, y + 1);
	nvgLineTo(ctx, x + 0.5f * w, y + s + 1);
	nvgLineTo(ctx, x + w, y + 1);
	nvgClosePath(ctx);
	nvgFillColor(ctx, color);
	nvgFill(ctx);
}
static inline void bndNodeArrowDown(NVGcontext* ctx, float x, float y, float s, NVGcolor color) {
	float w = 1.0f * s;
	nvgBeginPath(ctx);
	nvgMoveTo(ctx, x, y);
	nvgLineTo(ctx, x + 0.5f * w, y - s);
	nvgLineTo(ctx, x - 0.5f * w, y - s);
	nvgClosePath(ctx);
	nvgFillColor(ctx, color);
	nvgFill(ctx);
}

#endif /* BLENDISH_H */
