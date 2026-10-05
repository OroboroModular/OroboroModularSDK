// SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-Oroboro-Rack-Bridge-Exception (rack/LICENSE-EXCEPTION.md)
// Stand-ins for Rack's window: widgets, events, menus, the component
// library, the window and the app's context. A module built for Oroboro
// Modular never opens a window of its own (the plugin shows its knobs and
// jacks its own way), so nothing here draws or is clicked: it's what a
// module's panel code needs to compile and, where a module asks (a
// widget's place, a knob's quantity), to answer sensibly. The SDK's own
// code, to the same interface as VCV Rack 2's (docs/rack-modules.md).
#pragma once

#include <nanovg.h>
#include <blendish.h>
#include <nanosvg.h>

#include "rack_engine.hpp"

// What a menu item shows at its right when it's the one chosen, or opens a menu.
#define CHECKMARK_STRING "\xe2\x9c\x94"
#define CHECKMARK(_cond) ((_cond) ? CHECKMARK_STRING : "")
#define RIGHT_ARROW "\xe2\x96\xb8"

// The keys, buttons and window attributes panel code names (GLFW's
// numbers: a key event carries them, as Rack's does).
#define GLFW_TRUE 1
#define GLFW_FALSE 0
#define GLFW_RELEASE 0
#define GLFW_PRESS 1
#define GLFW_REPEAT 2
#define GLFW_MOUSE_BUTTON_1 0
#define GLFW_MOUSE_BUTTON_2 1
#define GLFW_MOUSE_BUTTON_3 2
#define GLFW_MOUSE_BUTTON_4 3
#define GLFW_MOUSE_BUTTON_5 4
#define GLFW_MOUSE_BUTTON_6 5
#define GLFW_MOUSE_BUTTON_7 6
#define GLFW_MOUSE_BUTTON_8 7
#define GLFW_MOUSE_BUTTON_LAST GLFW_MOUSE_BUTTON_8
#define GLFW_MOUSE_BUTTON_LEFT GLFW_MOUSE_BUTTON_1
#define GLFW_MOUSE_BUTTON_RIGHT GLFW_MOUSE_BUTTON_2
#define GLFW_MOUSE_BUTTON_MIDDLE GLFW_MOUSE_BUTTON_3
#define GLFW_MOD_SHIFT 0x0001
#define GLFW_MOD_CONTROL 0x0002
#define GLFW_MOD_ALT 0x0004
#define GLFW_MOD_SUPER 0x0008
#define GLFW_MOD_CAPS_LOCK 0x0010
#define GLFW_MOD_NUM_LOCK 0x0020
#define GLFW_KEY_UNKNOWN -1
#define GLFW_KEY_SPACE 32
#define GLFW_KEY_APOSTROPHE 39
#define GLFW_KEY_COMMA 44
#define GLFW_KEY_MINUS 45
#define GLFW_KEY_PERIOD 46
#define GLFW_KEY_SLASH 47
#define GLFW_KEY_0 48
#define GLFW_KEY_1 49
#define GLFW_KEY_2 50
#define GLFW_KEY_3 51
#define GLFW_KEY_4 52
#define GLFW_KEY_5 53
#define GLFW_KEY_6 54
#define GLFW_KEY_7 55
#define GLFW_KEY_8 56
#define GLFW_KEY_9 57
#define GLFW_KEY_SEMICOLON 59
#define GLFW_KEY_EQUAL 61
#define GLFW_KEY_A 65
#define GLFW_KEY_B 66
#define GLFW_KEY_C 67
#define GLFW_KEY_D 68
#define GLFW_KEY_E 69
#define GLFW_KEY_F 70
#define GLFW_KEY_G 71
#define GLFW_KEY_H 72
#define GLFW_KEY_I 73
#define GLFW_KEY_J 74
#define GLFW_KEY_K 75
#define GLFW_KEY_L 76
#define GLFW_KEY_M 77
#define GLFW_KEY_N 78
#define GLFW_KEY_O 79
#define GLFW_KEY_P 80
#define GLFW_KEY_Q 81
#define GLFW_KEY_R 82
#define GLFW_KEY_S 83
#define GLFW_KEY_T 84
#define GLFW_KEY_U 85
#define GLFW_KEY_V 86
#define GLFW_KEY_W 87
#define GLFW_KEY_X 88
#define GLFW_KEY_Y 89
#define GLFW_KEY_Z 90
#define GLFW_KEY_LEFT_BRACKET 91
#define GLFW_KEY_BACKSLASH 92
#define GLFW_KEY_RIGHT_BRACKET 93
#define GLFW_KEY_GRAVE_ACCENT 96
#define GLFW_KEY_WORLD_1 161
#define GLFW_KEY_WORLD_2 162
#define GLFW_KEY_ESCAPE 256
#define GLFW_KEY_ENTER 257
#define GLFW_KEY_TAB 258
#define GLFW_KEY_BACKSPACE 259
#define GLFW_KEY_INSERT 260
#define GLFW_KEY_DELETE 261
#define GLFW_KEY_RIGHT 262
#define GLFW_KEY_LEFT 263
#define GLFW_KEY_DOWN 264
#define GLFW_KEY_UP 265
#define GLFW_KEY_PAGE_UP 266
#define GLFW_KEY_PAGE_DOWN 267
#define GLFW_KEY_HOME 268
#define GLFW_KEY_END 269
#define GLFW_KEY_CAPS_LOCK 280
#define GLFW_KEY_SCROLL_LOCK 281
#define GLFW_KEY_NUM_LOCK 282
#define GLFW_KEY_PRINT_SCREEN 283
#define GLFW_KEY_PAUSE 284
#define GLFW_KEY_F1 290
#define GLFW_KEY_F2 291
#define GLFW_KEY_F3 292
#define GLFW_KEY_F4 293
#define GLFW_KEY_F5 294
#define GLFW_KEY_F6 295
#define GLFW_KEY_F7 296
#define GLFW_KEY_F8 297
#define GLFW_KEY_F9 298
#define GLFW_KEY_F10 299
#define GLFW_KEY_F11 300
#define GLFW_KEY_F12 301
#define GLFW_KEY_F13 302
#define GLFW_KEY_F14 303
#define GLFW_KEY_F15 304
#define GLFW_KEY_F16 305
#define GLFW_KEY_F17 306
#define GLFW_KEY_F18 307
#define GLFW_KEY_F19 308
#define GLFW_KEY_F20 309
#define GLFW_KEY_F21 310
#define GLFW_KEY_F22 311
#define GLFW_KEY_F23 312
#define GLFW_KEY_F24 313
#define GLFW_KEY_F25 314
#define GLFW_KEY_KP_0 320
#define GLFW_KEY_KP_1 321
#define GLFW_KEY_KP_2 322
#define GLFW_KEY_KP_3 323
#define GLFW_KEY_KP_4 324
#define GLFW_KEY_KP_5 325
#define GLFW_KEY_KP_6 326
#define GLFW_KEY_KP_7 327
#define GLFW_KEY_KP_8 328
#define GLFW_KEY_KP_9 329
#define GLFW_KEY_KP_DECIMAL 330
#define GLFW_KEY_KP_DIVIDE 331
#define GLFW_KEY_KP_MULTIPLY 332
#define GLFW_KEY_KP_SUBTRACT 333
#define GLFW_KEY_KP_ADD 334
#define GLFW_KEY_KP_ENTER 335
#define GLFW_KEY_KP_EQUAL 336
#define GLFW_KEY_LEFT_SHIFT 340
#define GLFW_KEY_LEFT_CONTROL 341
#define GLFW_KEY_LEFT_ALT 342
#define GLFW_KEY_LEFT_SUPER 343
#define GLFW_KEY_RIGHT_SHIFT 344
#define GLFW_KEY_RIGHT_CONTROL 345
#define GLFW_KEY_RIGHT_ALT 346
#define GLFW_KEY_RIGHT_SUPER 347
#define GLFW_KEY_MENU 348
#define GLFW_KEY_LAST GLFW_KEY_MENU
#define GLFW_FOCUSED 0x00020001
#define GLFW_ICONIFIED 0x00020002
#define GLFW_RESIZABLE 0x00020003
#define GLFW_VISIBLE 0x00020004
#define GLFW_DECORATED 0x00020005
#define GLFW_AUTO_ICONIFY 0x00020006
#define GLFW_FLOATING 0x00020007
#define GLFW_MAXIMIZED 0x00020008
#define GLFW_HOVERED 0x0002000B
#define GLFW_CURSOR 0x00033001
#define GLFW_STICKY_KEYS 0x00033002
#define GLFW_STICKY_MOUSE_BUTTONS 0x00033003
#define GLFW_LOCK_KEY_MODS 0x00033004
#define GLFW_RAW_MOUSE_MOTION 0x00033005
#define GLFW_CURSOR_NORMAL 0x00034001
#define GLFW_CURSOR_HIDDEN 0x00034002
#define GLFW_CURSOR_DISABLED 0x00034003
#define GLFW_ARROW_CURSOR 0x00036001
#define GLFW_IBEAM_CURSOR 0x00036002
#define GLFW_CROSSHAIR_CURSOR 0x00036003
#define GLFW_HAND_CURSOR 0x00036004
#define GLFW_HRESIZE_CURSOR 0x00036005
#define GLFW_VRESIZE_CURSOR 0x00036006
#if defined ARCH_MAC
#define RACK_MOD_CTRL GLFW_MOD_SUPER
#define RACK_MOD_CTRL_NAME "⌘"
#else
#define RACK_MOD_CTRL GLFW_MOD_CONTROL
#define RACK_MOD_CTRL_NAME "Ctrl"
#endif
#define RACK_MOD_SHIFT_NAME "Shift"
#define RACK_MOD_ALT_NAME "Alt"
#define RACK_MOD_MASK (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER)

struct GLFWwindow;
struct GLFWcursor;
struct NVGLUframebuffer;
inline int glfwGetKey(GLFWwindow*, int) { return GLFW_RELEASE; }
inline int glfwGetWindowAttrib(GLFWwindow*, int) { return 0; }
inline int glfwGetMouseButton(GLFWwindow*, int) { return GLFW_RELEASE; }
inline void glfwGetCursorPos(GLFWwindow*, double* x, double* y) {
	if (x)
		*x = 0;
	if (y)
		*y = 0;
}
inline void glfwSetCursorPos(GLFWwindow*, double, double) {}
inline void glfwSetInputMode(GLFWwindow*, int, int) {}
inline int glfwGetInputMode(GLFWwindow*, int) { return GLFW_CURSOR_NORMAL; }
inline const char* glfwGetClipboardString(GLFWwindow*) { return ""; }
inline void glfwSetClipboardString(GLFWwindow*, const char*) {}
inline double glfwGetTime() { return rack::system::getTime(); }
inline const char* glfwGetKeyName(int, int) { return nullptr; }
inline void glfwSetCursor(GLFWwindow*, GLFWcursor*) {}
// (the cursor's shape and the window are the plugin's: these keep nothing)
inline GLFWcursor* glfwCreateStandardCursor(int shape) { return nullptr; }
inline void glfwDestroyCursor(GLFWcursor*) {}
inline void glfwGetFramebufferSize(GLFWwindow*, int* width, int* height) {
	if (width)
		*width = 0;
	if (height)
		*height = 0;
}
inline void glfwGetWindowSize(GLFWwindow*, int* width, int* height) { glfwGetFramebufferSize(nullptr, width, height); }
inline void glfwIconifyWindow(GLFWwindow*) {}
inline int glfwGetKeyScancode(int key) { return -1; }
typedef void (*GLFWscrollfun)(GLFWwindow*, double, double);
typedef void (*GLFWmousebuttonfun)(GLFWwindow*, int, int, int);
typedef void (*GLFWcharfun)(GLFWwindow*, unsigned int);
inline GLFWscrollfun glfwSetScrollCallback(GLFWwindow*, GLFWscrollfun) { return nullptr; }
inline GLFWmousebuttonfun glfwSetMouseButtonCallback(GLFWwindow*, GLFWmousebuttonfun) { return nullptr; }
inline GLFWcharfun glfwSetCharCallback(GLFWwindow*, GLFWcharfun) { return nullptr; }

namespace rack {

static const float SVG_DPI = 75.f;
static const float MM_PER_IN = 25.4f;
inline float in2px(float in) { return in * SVG_DPI; }
inline math::Vec in2px(math::Vec in) { return in.mult(SVG_DPI); }
inline float mm2px(float mm) { return mm * (SVG_DPI / MM_PER_IN); }
inline math::Vec mm2px(math::Vec mm) { return mm.mult(SVG_DPI / MM_PER_IN); }
inline float px2mm(float px) { return px * (MM_PER_IN / SVG_DPI); }
inline math::Vec px2mm(math::Vec px) { return px.mult(MM_PER_IN / SVG_DPI); }
inline float px2in(float px) { return px / SVG_DPI; }
inline math::Vec px2in(math::Vec px) { return px.div(SVG_DPI); }

// ---- colours -------------------------------------------------------------------------------
namespace color {
static const NVGcolor BLACK_TRANSPARENT = nvgRGBA(0x00, 0x00, 0x00, 0x00);
static const NVGcolor WHITE_TRANSPARENT = nvgRGBA(0xff, 0xff, 0xff, 0x00);
static const NVGcolor BLACK = nvgRGB(0x00, 0x00, 0x00);
static const NVGcolor RED = nvgRGB(0xff, 0x00, 0x00);
static const NVGcolor GREEN = nvgRGB(0x00, 0xff, 0x00);
static const NVGcolor BLUE = nvgRGB(0x00, 0x00, 0xff);
static const NVGcolor CYAN = nvgRGB(0x00, 0xff, 0xff);
static const NVGcolor MAGENTA = nvgRGB(0xff, 0x00, 0xff);
static const NVGcolor YELLOW = nvgRGB(0xff, 0xff, 0x00);
static const NVGcolor WHITE = nvgRGB(0xff, 0xff, 0xff);

inline bool isEqual(NVGcolor a, NVGcolor b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }
inline NVGcolor clamp(NVGcolor a) {
	for (int i = 0; i < 4; i++)
		a.rgba[i] = math::clamp(a.rgba[i], 0.f, 1.f);
	return a;
}
inline NVGcolor minus(NVGcolor a, NVGcolor b) {
	for (int i = 0; i < 3; i++)
		a.rgba[i] -= b.rgba[i];
	return a;
}
inline NVGcolor plus(NVGcolor a, NVGcolor b) {
	for (int i = 0; i < 3; i++)
		a.rgba[i] += b.rgba[i];
	return a;
}
inline NVGcolor mult(NVGcolor a, NVGcolor b) {
	for (int i = 0; i < 4; i++)
		a.rgba[i] *= b.rgba[i];
	return a;
}
inline NVGcolor mult(NVGcolor a, float x) {
	for (int i = 0; i < 3; i++)
		a.rgba[i] *= x;
	return a;
}
inline NVGcolor screen(NVGcolor a, NVGcolor b) {
	if (a.a == 0.f)
		return b;
	if (b.a == 0.f)
		return a;
	NVGcolor c;
	for (int i = 0; i < 3; i++)
		c.rgba[i] = 1.f - (1.f - a.rgba[i] * a.a) * (1.f - b.rgba[i] * b.a);
	c.a = a.a + (1.f - a.a) * b.a;
	for (int i = 0; i < 3; i++)
		c.rgba[i] /= c.a;
	return c;
}
inline NVGcolor alpha(NVGcolor a, float alpha) {
	a.a *= alpha;
	return a;
}
inline NVGcolor lerp(NVGcolor a, NVGcolor b, float t) { return nvgLerpRGBA(a, b, t); }
inline NVGcolor fromHexString(std::string s) {
	uint8_t r = 0, g = 0, b = 0, a = 255;
	unsigned int ri = 0, gi = 0, bi = 0, ai = 255;
	if (std::sscanf(s.c_str(), "#%2x%2x%2x%2x", &ri, &gi, &bi, &ai) >= 3) {
		r = ri;
		g = gi;
		b = bi;
		a = ai;
	}
	return nvgRGBA(r, g, b, a);
}
inline std::string toHexString(NVGcolor c) {
	uint8_t r = std::round(c.r * 255), g = std::round(c.g * 255), b = std::round(c.b * 255), a = std::round(c.a * 255);
	if (a == 0xff)
		return string::f("#%02x%02x%02x", r, g, b);
	return string::f("#%02x%02x%02x%02x", r, g, b, a);
}
} // namespace color

// ---- window --------------------------------------------------------------------------------
namespace window {

// (Rack's units are window's: rack::window::mm2px too)
using rack::SVG_DPI;
using rack::MM_PER_IN;
using rack::in2px;
using rack::mm2px;
using rack::px2mm;
using rack::px2in;

struct Font;
struct Image;
std::shared_ptr<Font> loadFontFile(const std::string& filename);

struct Font {
	NVGcontext* vg = nullptr;
	int handle = -1;
	/// A font is which file it is: a text drawn with it is passed on as words.
	void loadFile(const std::string& filename, NVGcontext* vg);
	DEPRECATED static std::shared_ptr<Font> load(const std::string& filename) { return loadFontFile(filename); }
};
/// There are no pictures to draw with here: an image draws nothing.
struct Image {
	NVGcontext* vg = nullptr;
	int handle = -1;
	void loadFile(const std::string& filename, NVGcontext* vg) { this->vg = vg; }
	DEPRECATED static std::shared_ptr<Image> load(const std::string& filename) { return std::make_shared<Image>(); }
};
inline void Font::loadFile(const std::string& filename, NVGcontext* vg) {
	this->vg = vg;
	handle = oroboro_nvg_font(filename.c_str());
}
inline std::shared_ptr<Font> loadFontFile(const std::string& filename) {
	std::shared_ptr<Font> font = std::make_shared<Font>();
	font->loadFile(filename, nullptr);
	return font;
}
/// An SVG file: which one, and how big it is (Rack's pixels, 75 to the
/// inch). It isn't drawn here: `oromod rack build` draws a panel's once
/// and packs the picture, and the size is read from the file where the
/// plugin's folder is, else from the list the build packed with the module.
struct Svg {
	/// Its size as nanosvg's handle has it, in Rack's pixels (0 where it
	/// isn't known). Its shapes aren't read here: `window::svgDraw` asks the
	/// plugin to draw the file.
	NSVGimage* handle = nullptr;
	NSVGimage image;
	std::string path;
	math::Vec size;

	void loadFile(const std::string& filename);
	void loadString(const std::string& str) {}
	math::Vec getSize() { return size; }
	int getNumShapes() { return 0; }
	int getNumPaths() { return 0; }
	int getNumPoints() { return 0; }
	void draw(NVGcontext* vg) {}
	static std::shared_ptr<Svg> load(const std::string& filename);
	/// The files loaded so far (each once), for whoever packs the module.
	static std::vector<std::shared_ptr<Svg>>& loaded();
};

DEPRECATED typedef Svg SVG;

/// Draws the SVG at its size through the transform as it is (a picture in
/// what the module draws: the plugin draws the file, which the build packs
/// with the module).
void svgDraw(NVGcontext* vg, NSVGimage* svg);

struct Window {
	GLFWwindow* win = nullptr;
	NVGcontext* vg = nullptr;
	NVGcontext* fbVg = nullptr;
	float pixelRatio = 1.f;
	float windowRatio = 1.f;
	std::shared_ptr<Font> uiFont;

	/// There are no fonts or pictures to draw with. What's returned is a
	/// font or a picture all the same (panel code keeps and asks them),
	/// with a handle that draws nothing.
	std::shared_ptr<Font> loadFont(const std::string& filename) {
		// (a font is which file it is: a text drawn with it is passed on as words)
		std::shared_ptr<Font> font = std::make_shared<Font>();
		font->vg = vg;
		font->handle = oroboro_nvg_font(filename.c_str());
		return font;
	}
	std::shared_ptr<Font> loadFontWithoutFallbacks(const std::string& filename) { return loadFont(filename); }
	std::shared_ptr<Image> loadImage(const std::string& filename) { return std::make_shared<Image>(); }
	std::shared_ptr<Svg> loadSvg(const std::string& filename) { return Svg::load(filename); }
	math::Vec getSize() { return math::Vec(); }
	void setSize(math::Vec size) {}
	math::Vec getPosition() { return math::Vec(); }
	void close() {}
	void cursorLock() {}
	void cursorUnlock() {}
	bool isCursorLocked() { return false; }
	int getMods() { return 0; }
	void setFullScreen(bool fullScreen) {}
	bool isFullScreen() { return false; }
	double getMonitorRefreshRate() { return 60.0; }
	double getFrameTime() { return 0.0; }
	double getLastFrameDuration() { return 1.0 / 60.0; }
	double getFrameDurationRemaining() { return 0.0; }
	int getFrame() { return 0; }
	bool isFrameOverdue() { return false; }
};

inline void init() {}
inline void destroy() {}
} // namespace window

namespace ui {
struct Menu;
struct Tooltip;
} // namespace ui

// ---- widget --------------------------------------------------------------------------------
namespace widget {

struct Widget;

struct EventContext {
	Widget* target = nullptr;
	bool propagating = true;
	bool consumed = false;
};

struct BaseEvent {
	EventContext* context = nullptr;

	void stopPropagating() const {
		if (context)
			context->propagating = false;
	}
	bool isPropagating() const { return context ? context->propagating : true; }
	void setTarget(Widget* w) const {
		if (context)
			context->target = w;
	}
	Widget* getTarget() const { return context ? context->target : nullptr; }
	void consume(Widget* w) const {
		if (context) {
			context->propagating = false;
			context->consumed = true;
			context->target = w;
		}
	}
	void unconsume() const {
		if (context)
			context->consumed = false;
	}
	bool isConsumed() const { return context ? context->consumed : false; }
};

/// Anything in a window: a box, a parent, children. Nothing draws.
struct Widget : WeakBase {
	math::Rect box = math::Rect(math::Vec(), math::Vec(INFINITY, INFINITY));
	Widget* parent = nullptr;
	std::list<Widget*> children;
	bool visible = true;
	bool requestedDelete = false;

	virtual ~Widget() { clearChildren(); }

	math::Rect getBox() { return box; }
	void setBox(math::Rect box) { this->box = box; }
	math::Vec getPosition() { return box.pos; }
	void setPosition(math::Vec pos) { box.pos = pos; }
	math::Vec getSize() { return box.size; }
	void setSize(math::Vec size) { box.size = size; }
	Widget* getParent() { return parent; }
	bool isVisible() { return visible; }
	void setVisible(bool visible) { this->visible = visible; }
	void show() { visible = true; }
	void hide() { visible = false; }
	void requestDelete() { requestedDelete = true; }

	virtual math::Rect getChildrenBoundingBox() {
		math::Vec min(INFINITY, INFINITY), max(-INFINITY, -INFINITY);
		for (Widget* child : children) {
			min = min.min(child->box.getTopLeft());
			max = max.max(child->box.getBottomRight());
		}
		return math::Rect::fromMinMax(min, max);
	}
	virtual math::Rect getVisibleChildrenBoundingBox() { return getChildrenBoundingBox(); }
	bool isDescendantOf(Widget* ancestor) {
		for (Widget* w = parent; w; w = w->parent)
			if (w == ancestor)
				return true;
		return false;
	}
	virtual math::Vec getRelativeOffset(math::Vec v, Widget* ancestor) {
		if (this == ancestor)
			return v;
		v = v.plus(box.pos);
		if (!parent)
			return v;
		return parent->getRelativeOffset(v, ancestor);
	}
	math::Vec getAbsoluteOffset(math::Vec v) { return getRelativeOffset(v, nullptr); }
	virtual float getRelativeZoom(Widget* ancestor) {
		if (this == ancestor || !parent)
			return 1.f;
		return parent->getRelativeZoom(ancestor);
	}
	float getAbsoluteZoom() { return getRelativeZoom(nullptr); }
	virtual math::Rect getViewport(math::Rect r = math::Rect::inf()) { return r; }

	template <class T>
	T* getAncestorOfType() {
		for (Widget* w = parent; w; w = w->parent)
			if (T* t = dynamic_cast<T*>(w))
				return t;
		return nullptr;
	}
	template <class T>
	T* getFirstDescendantOfType() {
		for (Widget* child : children) {
			if (T* t = dynamic_cast<T*>(child))
				return t;
			if (T* t = child->getFirstDescendantOfType<T>())
				return t;
		}
		return nullptr;
	}

	bool hasChild(Widget* child) { return std::find(children.begin(), children.end(), child) != children.end(); }
	void addChild(Widget* child) {
		child->parent = this;
		children.push_back(child);
	}
	void addChildBottom(Widget* child) {
		child->parent = this;
		children.push_front(child);
	}
	void addChildBelow(Widget* child, Widget* sibling) {
		child->parent = this;
		children.insert(std::find(children.begin(), children.end(), sibling), child);
	}
	void addChildAbove(Widget* child, Widget* sibling) {
		child->parent = this;
		auto it = std::find(children.begin(), children.end(), sibling);
		if (it != children.end())
			++it;
		children.insert(it, child);
	}
	void removeChild(Widget* child) {
		children.remove(child);
		child->parent = nullptr;
	}
	void clearChildren() {
		for (Widget* child : children) {
			child->parent = nullptr;
			delete child;
		}
		children.clear();
	}

	virtual void step() {
		for (Widget* child : children)
			child->step();
	}

	struct DrawArgs {
		NVGcontext* vg = nullptr;
		math::Rect clipBox;
		NVGLUframebuffer* fb = nullptr;
	};
	/// Draws itself: by default its children, each where it is. (What's
	/// drawn is written down, not drawn: nanovg.h.)
	virtual void draw(const DrawArgs& args) {
		for (Widget* child : children)
			if (child->visible)
				drawChild(child, args);
	}
	DEPRECATED virtual void draw(NVGcontext* vg) {}
	/// Draws a layer over the first (1: what lights up by itself).
	virtual void drawLayer(const DrawArgs& args, int layer) {
		for (Widget* child : children)
			if (child->visible)
				drawChild(child, args, layer);
	}
	void drawChild(Widget* child, const DrawArgs& args, int layer = 0) {
		DrawArgs childArgs = args;
		childArgs.clipBox.pos = childArgs.clipBox.pos.minus(child->box.pos);
		nvgSave(args.vg);
		nvgTranslate(args.vg, child->box.pos.x, child->box.pos.y);
		if (layer == 0)
			child->draw(childArgs);
		else
			child->drawLayer(childArgs, layer);
		nvgRestore(args.vg);
	}

	using BaseEvent = widget::BaseEvent;
	struct PositionBaseEvent {
		math::Vec pos;
	};
	struct KeyBaseEvent {
		int key = 0;
		int scancode = 0;
		std::string keyName;
		int action = 0;
		int mods = 0;
		bool isKeyCommand(int key, int mods = 0) const { return false; }
	};
	struct TextBaseEvent {
		int codepoint = 0;
	};
	struct HoverEvent : BaseEvent, PositionBaseEvent {
		math::Vec mouseDelta;
	};
	struct ButtonEvent : BaseEvent, PositionBaseEvent {
		int button = 0;
		int action = 0;
		int mods = 0;
	};
	struct DoubleClickEvent : BaseEvent {};
	struct HoverKeyEvent : BaseEvent, PositionBaseEvent, KeyBaseEvent {};
	struct HoverTextEvent : BaseEvent, PositionBaseEvent, TextBaseEvent {};
	struct HoverScrollEvent : BaseEvent, PositionBaseEvent {
		math::Vec scrollDelta;
	};
	struct EnterEvent : BaseEvent {};
	struct LeaveEvent : BaseEvent {};
	struct SelectEvent : BaseEvent {};
	struct DeselectEvent : BaseEvent {};
	struct SelectKeyEvent : BaseEvent, KeyBaseEvent {};
	struct SelectTextEvent : BaseEvent, TextBaseEvent {};
	struct DragBaseEvent : BaseEvent {
		int button = 0;
	};
	struct DragStartEvent : DragBaseEvent {};
	struct DragEndEvent : DragBaseEvent {};
	struct DragMoveEvent : DragBaseEvent {
		math::Vec mouseDelta;
	};
	struct DragHoverEvent : DragBaseEvent, PositionBaseEvent {
		Widget* origin = nullptr;
		math::Vec mouseDelta;
	};
	struct DragEnterEvent : DragBaseEvent {
		Widget* origin = nullptr;
	};
	struct DragLeaveEvent : DragBaseEvent {
		Widget* origin = nullptr;
	};
	struct DragDropEvent : DragBaseEvent {
		Widget* origin = nullptr;
	};
	struct PathDropEvent : BaseEvent, PositionBaseEvent {
		PathDropEvent(const std::vector<std::string>& paths) : paths(paths) {}
		const std::vector<std::string>& paths;
	};
	struct ActionEvent : BaseEvent {};
	struct ChangeEvent : BaseEvent {};
	struct DirtyEvent : BaseEvent {};
	struct RepositionEvent : BaseEvent {};
	struct ResizeEvent : BaseEvent {};
	struct AddEvent : BaseEvent {};
	struct RemoveEvent : BaseEvent {};
	struct ShowEvent : BaseEvent {};
	struct HideEvent : BaseEvent {};
	struct ContextCreateEvent : BaseEvent {
		NVGcontext* vg = nullptr;
	};
	struct ContextDestroyEvent : BaseEvent {
		NVGcontext* vg = nullptr;
	};

	/// A positional event to the children under it, the top one first
	/// (the last added), each at its own place, until one stops it.
	template <typename TMethod, class TEvent>
	void recursePositionEvent(TMethod f, const TEvent& e) {
		for (auto it = children.rbegin(); it != children.rend(); it++) {
			Widget* child = *it;
			if (!child->visible || !child->box.contains(e.pos))
				continue;
			TEvent e2 = e;
			e2.pos = e.pos.minus(child->box.pos);
			(child->*f)(e2);
			if (!e.isPropagating())
				break;
		}
	}
	/// An event to every child, the top one first, until one stops it.
	template <typename TMethod, class TEvent>
	void recurseEvent(TMethod f, const TEvent& e) {
		for (auto it = children.rbegin(); it != children.rend(); it++) {
			(*it->*f)(e);
			if (!e.isPropagating())
				break;
		}
	}
	/// The pointer's events, as Rack gives them (`oroboro_module_pointer`
	/// in the bridge): down to the children under it by default.
	virtual void onHover(const HoverEvent& e) { recursePositionEvent(&Widget::onHover, e); }
	virtual void onButton(const ButtonEvent& e) { recursePositionEvent(&Widget::onButton, e); }
	virtual void onDoubleClick(const DoubleClickEvent& e) {}
	virtual void onHoverKey(const HoverKeyEvent& e) { recursePositionEvent(&Widget::onHoverKey, e); }
	virtual void onHoverText(const HoverTextEvent& e) { recursePositionEvent(&Widget::onHoverText, e); }
	virtual void onHoverScroll(const HoverScrollEvent& e) { recursePositionEvent(&Widget::onHoverScroll, e); }
	virtual void onEnter(const EnterEvent& e) {}
	virtual void onLeave(const LeaveEvent& e) {}
	virtual void onSelect(const SelectEvent& e) {}
	virtual void onDeselect(const DeselectEvent& e) {}
	virtual void onSelectKey(const SelectKeyEvent& e) {}
	virtual void onSelectText(const SelectTextEvent& e) {}
	virtual void onDragStart(const DragStartEvent& e) {}
	virtual void onDragEnd(const DragEndEvent& e) {}
	virtual void onDragMove(const DragMoveEvent& e) {}
	virtual void onDragHover(const DragHoverEvent& e) { recursePositionEvent(&Widget::onDragHover, e); }
	virtual void onDragEnter(const DragEnterEvent& e) {}
	virtual void onDragLeave(const DragLeaveEvent& e) {}
	virtual void onDragDrop(const DragDropEvent& e) {}
	virtual void onPathDrop(const PathDropEvent& e) { recursePositionEvent(&Widget::onPathDrop, e); }
	virtual void onAction(const ActionEvent& e) {}
	virtual void onChange(const ChangeEvent& e) {}
	virtual void onDirty(const DirtyEvent& e) {}
	virtual void onReposition(const RepositionEvent& e) {}
	virtual void onResize(const ResizeEvent& e) {}
	virtual void onAdd(const AddEvent& e) {}
	virtual void onRemove(const RemoveEvent& e) {}
	virtual void onShow(const ShowEvent& e) {}
	virtual void onHide(const HideEvent& e) {}
	virtual void onContextCreate(const ContextCreateEvent& e) {}
	virtual void onContextDestroy(const ContextDestroyEvent& e) {}
};

/// Lets the pointer through to what's under it.
struct TransparentWidget : Widget {
	void onHover(const HoverEvent& e) override {}
	void onButton(const ButtonEvent& e) override {}
	void onHoverKey(const HoverKeyEvent& e) override {}
	void onHoverText(const HoverTextEvent& e) override {}
	void onHoverScroll(const HoverScrollEvent& e) override {}
	void onDragHover(const DragHoverEvent& e) override {}
	void onPathDrop(const PathDropEvent& e) override {}
};
/// Takes the pointer where none of its children does.
struct OpaqueWidget : Widget {
	void onHover(const HoverEvent& e) override {
		Widget::onHover(e);
		e.stopPropagating();
		if (!e.isConsumed())
			e.consume(this);
	}
	void onButton(const ButtonEvent& e) override {
		Widget::onButton(e);
		e.stopPropagating();
		if (!e.isConsumed())
			e.consume(this);
	}
	void onHoverKey(const HoverKeyEvent& e) override {
		Widget::onHoverKey(e);
		e.stopPropagating();
	}
	void onHoverText(const HoverTextEvent& e) override {
		Widget::onHoverText(e);
		e.stopPropagating();
	}
	void onHoverScroll(const HoverScrollEvent& e) override {
		Widget::onHoverScroll(e);
		e.stopPropagating();
	}
	void onDragHover(const DragHoverEvent& e) override {
		Widget::onDragHover(e);
		e.stopPropagating();
		if (!e.isConsumed())
			e.consume(this);
	}
	void onPathDrop(const PathDropEvent& e) override {
		Widget::onPathDrop(e);
		e.stopPropagating();
	}
};
struct TransformWidget : Widget {
	float transform[6] = {1, 0, 0, 1, 0, 0};
	void identity() {}
	void translate(math::Vec delta) {}
	void rotate(float angle) {}
	void rotate(float angle, math::Vec origin) {}
	void scale(math::Vec s) {}
};
struct ZoomWidget : Widget {
	float zoom = 1.f;
	float getZoom() { return zoom; }
	void setZoom(float zoom) { this->zoom = zoom; }
	// (its children are at its zoom)
	math::Vec getRelativeOffset(math::Vec v, Widget* ancestor) override {
		if (this == ancestor)
			return v;
		return Widget::getRelativeOffset(v.mult(zoom), ancestor);
	}
	float getRelativeZoom(Widget* ancestor) override {
		if (this == ancestor)
			return 1.f;
		return zoom * Widget::getRelativeZoom(ancestor);
	}
	math::Rect getViewport(math::Rect r = math::Rect::inf()) override {
		r = Widget::getViewport(math::Rect(r.pos.mult(zoom), r.size.mult(zoom)));
		return math::Rect(r.pos.div(zoom), r.size.div(zoom));
	}
};
struct SvgWidget : Widget {
	std::shared_ptr<window::Svg> svg;
	SvgWidget() { box.size = math::Vec(); }
	/// As big as its picture (where the picture's size is known).
	void wrap() {
		if (svg && svg->size.x > 0.f && svg->size.y > 0.f)
			box.size = svg->size;
	}
	void setSvg(std::shared_ptr<window::Svg> svg) {
		this->svg = svg;
		wrap();
	}
	DEPRECATED void setSVG(std::shared_ptr<window::Svg> svg) { setSvg(svg); }
};
DEPRECATED typedef SvgWidget SVGWidget;
struct FramebufferWidget : Widget {
	bool dirty = true;
	bool bypassed = false;
	float oversample = 1.f;
	bool dirtyOnSubpixelChange = true;
	math::Vec viewportMargin = math::Vec(INFINITY, INFINITY);
	void setDirty(bool dirty = true) { this->dirty = dirty; }
	int getImageHandle() { return -1; }
	NVGLUframebuffer* getFramebuffer() { return nullptr; }
	math::Vec getFramebufferSize() { return math::Vec(); }
	void deleteFramebuffer() {}
	void render(math::Vec scale = math::Vec(1, 1), math::Vec offsetF = math::Vec(0, 0), math::Rect clipBox = math::Rect::inf()) {}
	virtual void drawFramebuffer() {}
};
/// Rack's OpenGL drawing: there's no OpenGL here, so it draws nothing.
struct OpenGlWidget : FramebufferWidget {
	void drawFramebuffer() override {}
};
typedef FramebufferWidget FramebufferWidgetDeprecated;

} // namespace widget

/// The old names of the events.
namespace event {
using Base = widget::BaseEvent;
using PositionBase = widget::Widget::PositionBaseEvent;
using KeyBase = widget::Widget::KeyBaseEvent;
using TextBase = widget::Widget::TextBaseEvent;
using Hover = widget::Widget::HoverEvent;
using Button = widget::Widget::ButtonEvent;
using DoubleClick = widget::Widget::DoubleClickEvent;
using HoverKey = widget::Widget::HoverKeyEvent;
using HoverText = widget::Widget::HoverTextEvent;
using HoverScroll = widget::Widget::HoverScrollEvent;
using Enter = widget::Widget::EnterEvent;
using Leave = widget::Widget::LeaveEvent;
using Select = widget::Widget::SelectEvent;
using Deselect = widget::Widget::DeselectEvent;
using SelectKey = widget::Widget::SelectKeyEvent;
using SelectText = widget::Widget::SelectTextEvent;
using DragBase = widget::Widget::DragBaseEvent;
using DragStart = widget::Widget::DragStartEvent;
using DragEnd = widget::Widget::DragEndEvent;
using DragMove = widget::Widget::DragMoveEvent;
using DragHover = widget::Widget::DragHoverEvent;
using DragEnter = widget::Widget::DragEnterEvent;
using DragLeave = widget::Widget::DragLeaveEvent;
using DragDrop = widget::Widget::DragDropEvent;
using PathDrop = widget::Widget::PathDropEvent;
using Action = widget::Widget::ActionEvent;
using Change = widget::Widget::ChangeEvent;
using Dirty = widget::Widget::DirtyEvent;
using Reposition = widget::Widget::RepositionEvent;
using Resize = widget::Widget::ResizeEvent;
using Add = widget::Widget::AddEvent;
using Remove = widget::Widget::RemoveEvent;
using Show = widget::Widget::ShowEvent;
using Hide = widget::Widget::HideEvent;

/// Which widget has the mouse, the keys, the drag: none ever does.
struct State {
	widget::Widget* rootWidget = nullptr;
	widget::Widget* hoveredWidget = nullptr;
	widget::Widget* draggedWidget = nullptr;
	int dragButton = 0;
	widget::Widget* dragHoveredWidget = nullptr;
	widget::Widget* selectedWidget = nullptr;
	double lastClickTime = 0.0;
	widget::Widget* lastClickedWidget = nullptr;
	std::set<int> heldKeys;

	widget::Widget* getRootWidget() { return rootWidget; }
	widget::Widget* getHoveredWidget() { return hoveredWidget; }
	widget::Widget* getDraggedWidget() { return draggedWidget; }
	widget::Widget* getDragHoveredWidget() { return dragHoveredWidget; }
	widget::Widget* getSelectedWidget() { return selectedWidget; }
	void setHoveredWidget(widget::Widget* w) { hoveredWidget = w; }
	void setDraggedWidget(widget::Widget* w, int button) { draggedWidget = w; }
	void setDragHoveredWidget(widget::Widget* w) { dragHoveredWidget = w; }
	void setSelectedWidget(widget::Widget* w) { selectedWidget = w; }
	DEPRECATED void setHovered(widget::Widget* w) { setHoveredWidget(w); }
	DEPRECATED void setDragged(widget::Widget* w, int button) { setDraggedWidget(w, button); }
	DEPRECATED void setDragHovered(widget::Widget* w) { setDragHoveredWidget(w); }
	DEPRECATED void setSelected(widget::Widget* w) { setSelectedWidget(w); }
	void finalizeWidget(widget::Widget* w) {}
};
} // namespace event

namespace widget {
// (Rack's name for it)
typedef event::State EventState;
/// A key's name, and a key command's ("Ctrl+Z"): none here, as no key is pressed.
inline std::string getKeyName(int key) { return ""; }
inline std::string getKeyCommandName(int key, int mods) { return ""; }
} // namespace widget

// ---- ui ------------------------------------------------------------------------------------
namespace ui {

/// A menu: its entries are its children. Nothing shows it here; the
/// plugin asks what's in it and shows that its own way.
/// Rack's menus' colours: blendish's theme here.
inline void setTheme(NVGcolor bg, NVGcolor fg) {
	BNDtheme theme = *bndGetTheme();
	theme.backgroundColor = bg;
	theme.menuTheme.innerColor = theme.menuItemTheme.innerColor = bg;
	theme.menuTheme.textColor = theme.menuItemTheme.textColor = theme.regularTheme.textColor = fg;
	bndSetTheme(theme);
}
inline void refreshTheme() {}

struct MenuEntry;
struct Menu : widget::OpaqueWidget {
	Menu* parentMenu = nullptr;
	Menu* childMenu = nullptr;
	MenuEntry* activeEntry = nullptr;
	BNDcornerFlags cornerFlags = BND_CORNER_NONE;
	void setChildMenu(Menu* menu) {}
};
struct MenuEntry : widget::OpaqueWidget {};
struct MenuSeparator : MenuEntry {};
struct MenuLabel : MenuEntry {
	std::string text;
};
/// An entry to choose: `onAction` is what choosing it does, `step` brings
/// its texts up to date (a check mark, the value it stands at), and
/// `createChildMenu` gives the menu it opens, if it opens one.
struct MenuItem : MenuEntry {
	std::string text;
	std::string rightText;
	bool disabled = false;
	virtual Menu* createChildMenu() { return nullptr; }
	void doAction(bool consume = true) {
		widget::EventContext context;
		ActionEvent e;
		e.context = &context;
		if (consume)
			e.consume(this);
		onAction(e);
	}
};
/// A menu item with a dot of `color` (a cable colour's, say).
struct ColorDotMenuItem : MenuItem {
	NVGcolor color = color::BLACK_TRANSPARENT;
};
struct MenuOverlay : widget::OpaqueWidget {
	NVGcolor bgColor = nvgRGBAf(0, 0, 0, 0);
};
struct Label : widget::Widget {
	enum Alignment { LEFT_ALIGNMENT, CENTER_ALIGNMENT, RIGHT_ALIGNMENT };
	std::string text;
	float fontSize = 13.f;
	float lineHeight = 1.2f;
	NVGcolor color = nvgRGB(0xff, 0xff, 0xff);
	Alignment alignment = LEFT_ALIGNMENT;
};
struct Tooltip : widget::Widget {
	std::string text;
};
struct TooltipOverlay : widget::TransparentWidget {};
struct TextField : widget::OpaqueWidget {
	std::string text;
	std::string placeholder;
	bool password = false;
	bool multiline = false;
	int cursor = 0;
	int selection = 0;
	widget::Widget* prevField = nullptr;
	widget::Widget* nextField = nullptr;

	std::string getText() { return text; }
	void setText(std::string text) {
		this->text = text;
		cursor = selection = (int) text.size();
	}
	void selectAll() {
		cursor = (int) text.size();
		selection = 0;
	}
	std::string getSelectedText() { return ""; }
	void insertText(std::string text) {}
	void copyClipboard() {}
	void cutClipboard() {}
	void pasteClipboard() {}
	void cursorToPrevWord() {}
	void cursorToNextWord() {}
	void createContextMenu() {}
	virtual int getTextPosition(math::Vec mousePos) { return 0; }
};
struct PasswordField : TextField {};
struct Button : widget::OpaqueWidget {
	std::string text;
	Quantity* quantity = nullptr;
};
struct ChoiceButton : Button {};
struct OptionButton : Button {};
struct RadioButton : widget::OpaqueWidget {
	Quantity* quantity = nullptr;
};
struct Slider : widget::OpaqueWidget {
	Quantity* quantity = nullptr;
};
struct ProgressBar : widget::Widget {
	Quantity* quantity = nullptr;
};
struct Scrollbar : widget::OpaqueWidget {
	bool vertical = false;
};
DEPRECATED typedef Scrollbar ScrollBar;
struct ScrollWidget : widget::OpaqueWidget {
	widget::Widget* container = nullptr;
	Scrollbar* horizontalScrollbar = nullptr;
	Scrollbar* verticalScrollbar = nullptr;
	math::Vec offset;
	math::Rect containerBox;
	bool hideScrollbars = false;

	ScrollWidget() {
		container = new widget::Widget;
		addChild(container);
	}
	math::Vec getScrollOffset() { return offset; }
	void scrollTo(math::Rect r) {}
	math::Rect getContainerOffsetBound() { return math::Rect(); }
	math::Vec getHandleOffset() { return math::Vec(); }
	math::Vec getHandleSize() { return math::Vec(); }
	bool isScrolling() { return false; }
};
/// Its children in a row (or a column), wrapped at its edge, each line
/// lined up as it says.
struct SequentialLayout : widget::Widget {
	enum Orientation { HORIZONTAL_ORIENTATION, VERTICAL_ORIENTATION };
	enum Alignment {
		LEFT_ALIGNMENT,
		CENTER_ALIGNMENT,
		RIGHT_ALIGNMENT,
		TOP_ALIGNMENT = LEFT_ALIGNMENT,
		MIDDLE_ALIGNMENT = CENTER_ALIGNMENT,
		BOTTOM_ALIGNMENT = RIGHT_ALIGNMENT,
	};
	Orientation orientation = HORIZONTAL_ORIENTATION;
	Alignment alignment = LEFT_ALIGNMENT;
	bool wrap = true;
	math::Vec margin;
	math::Vec spacing;

	void step() override {
		Widget::step();
		bool across = orientation == HORIZONTAL_ORIENTATION;
		float room = (across ? box.size.x - 2 * margin.x : box.size.y - 2 * margin.y);
		float gap = across ? spacing.x : spacing.y, lineGap = across ? spacing.y : spacing.x;
		// (where the next line goes, across the way lines run)
		float down = across ? margin.y : margin.x;
		std::vector<Widget*> line;
		float lineLength = 0.f, lineThickness = 0.f;
		auto place = [&]() {
			float at = (across ? margin.x : margin.y) + (room - lineLength) * (alignment / 2.f);
			for (Widget* w : line) {
				(across ? w->box.pos.x : w->box.pos.y) = at;
				(across ? w->box.pos.y : w->box.pos.x) = down;
				at += (across ? w->box.size.x : w->box.size.y) + gap;
			}
			down += lineThickness + lineGap;
			line.clear();
			lineLength = lineThickness = 0.f;
		};
		for (Widget* w : children) {
			if (!w->visible)
				continue;
			float length = across ? w->box.size.x : w->box.size.y;
			if (wrap && !line.empty() && lineLength + gap + length > room)
				place();
			lineLength += (line.empty() ? 0.f : gap) + length;
			lineThickness = std::max(lineThickness, across ? w->box.size.y : w->box.size.x);
			line.push_back(w);
		}
		if (!line.empty())
			place();
	}
};
struct MarginLayout : widget::Widget {
	math::Vec margin;
	math::Rect requestedBox;
};
/// Its children down a column, each as wide as it.
struct List : widget::OpaqueWidget {
	void step() override {
		Widget::step();
		float y = 0.f;
		for (Widget* w : children) {
			if (!w->visible)
				continue;
			w->box.pos = math::Vec(0, y);
			w->box.size.x = box.size.x;
			y += w->box.size.y;
		}
		box.size.y = y;
	}
};
} // namespace ui

namespace app {
struct ModuleWidget;
struct CableWidget;
}
// Rack's undo history. There is no undo here: what's pushed is dropped, and
// an action undoes and redoes nothing.
namespace history {
struct Action {
	std::string name;
	virtual ~Action() {}
	virtual void undo() {}
	virtual void redo() {}
};
/// An action the other way round.
template <class TAction>
struct InverseAction : TAction {
	void undo() override { TAction::redo(); }
	void redo() override { TAction::undo(); }
};
struct ComplexAction : Action {
	std::vector<Action*> actions;
	~ComplexAction() {
		for (Action* a : actions)
			delete a;
	}
	void undo() override {
		for (auto it = actions.rbegin(); it != actions.rend(); ++it)
			(*it)->undo();
	}
	void redo() override {
		for (Action* a : actions)
			a->redo();
	}
	void push(Action* action) { actions.push_back(action); }
	bool isEmpty() { return actions.empty(); }
};
struct ModuleAction : Action {
	int64_t moduleId = -1;
};
struct ModuleAdd : ModuleAction {
	plugin::Model* model = nullptr;
	math::Vec pos;
	json_t* moduleJ = nullptr;
	void setModule(app::ModuleWidget* mw) {}
};
struct ModuleRemove : InverseAction<ModuleAdd> {};
struct ModuleMove : ModuleAction {
	math::Vec oldPos;
	math::Vec newPos;
};
struct ModuleBypass : ModuleAction {
	bool bypassed = false;
};
struct ModuleChange : ModuleAction {
	json_t* oldModuleJ = nullptr;
	json_t* newModuleJ = nullptr;
};
struct ParamChange : ModuleAction {
	int paramId = -1;
	float oldValue = 0.f;
	float newValue = 0.f;
};
struct CableAdd : Action {
	int64_t cableId = -1;
	int64_t inputModuleId = -1;
	int inputId = -1;
	int64_t outputModuleId = -1;
	int outputId = -1;
	NVGcolor color = color::BLACK_TRANSPARENT;
	void setCable(app::CableWidget* cw) {}
	bool isCable(app::CableWidget* cw) const { return false; }
};
struct CableRemove : InverseAction<CableAdd> {};
struct CableColorChange : Action {
	int64_t cableId = -1;
	NVGcolor newColor = color::BLACK_TRANSPARENT;
	NVGcolor oldColor = color::BLACK_TRANSPARENT;
	void setCable(app::CableWidget* cw) {}
};
struct State {
	std::deque<Action*> actions;
	int actionIndex = 0;
	int savedIndex = 0;
	void push(Action* action) { delete action; }
	void clear() {}
	void undo() {}
	void redo() {}
	bool canUndo() { return false; }
	bool canRedo() { return false; }
	std::string getUndoName() { return ""; }
	std::string getRedoName() { return ""; }
	bool isSaved() { return true; }
	void setSaved() {}
};
} // namespace history

namespace patch {
/// Rack's patch, which a module may ask to be saved, loaded or cleared. The
/// patch here is the plugin's (its host keeps it), so these do nothing,
/// and there's no file it's in.
struct Manager {
	std::string path;
	std::string autosavePath;
	std::string templatePath;
	std::string factoryTemplatePath;

	void clear() {}
	void save(std::string path) {}
	void saveDialog() {}
	void saveAsDialog(bool setPath = true) {}
	void saveTemplateDialog() {}
	void saveAutosave() {}
	void clearAutosave() {}
	void cleanAutosave() {}
	void load(std::string path) {}
	void loadTemplate() {}
	void loadTemplateDialog() {}
	bool hasAutosave() { return false; }
	void loadAutosave() {}
	void loadAction(std::string path) {}
	void loadDialog() {}
	void loadPathDialog(std::string path) {}
	void revertDialog() {}
	void pushRecentPath(std::string path) {}
	void disconnectDialog() {}
	json_t* toJson() { return nullptr; }
	void fromJson(json_t* rootJ) {}
	bool checkUnavailableModulesJson(json_t* rootJ) { return true; }
};
} // namespace patch

// ---- plugin --------------------------------------------------------------------------------
namespace app {
struct ModuleWidget;
}

namespace plugin {

/// One kind of module of a plugin.
struct Model {
	Plugin* plugin = nullptr;
	std::string slug;
	std::string name;
	std::list<int> tagIds;
	std::string description;
	std::string manualUrl;
	std::string modularGridUrl;
	bool hidden = false;

	virtual ~Model() {}
	virtual engine::Module* createModule() { return nullptr; }
	virtual app::ModuleWidget* createModuleWidget(engine::Module* m) { return nullptr; }
	std::string getFullName();
	std::string getFactoryPresetDirectory();
	std::string getUserPresetDirectory();
	/// Its manual: its own, else its plugin's.
	std::string getManualUrl();
	/// Favourites are the module browser's, which is the plugin's here.
	bool isFavorite() { return false; }
	void setFavorite(bool favorite) {}
	void appendContextMenu(ui::Menu* menu, bool inBrowser = false) {}
};

struct Plugin {
	std::list<Model*> models;
	std::string path;
	void* handle = nullptr;
	std::string slug;
	std::string version;
	std::string license;
	std::string name;
	std::string brand;
	std::string description;
	std::string author;
	std::string authorEmail;
	std::string authorUrl;
	std::string pluginUrl;
	std::string manualUrl;
	std::string sourceUrl;
	std::string donateUrl;
	std::string changelogUrl;
	double modifiedTimestamp = -INFINITY;

	void addModel(Model* model) {
		if (!model)
			return;
		model->plugin = this;
		models.push_back(model);
	}
	Model* getModel(const std::string& slug) {
		for (Model* m : models)
			if (m->slug == slug)
				return m;
		return nullptr;
	}
	std::string getBrand() { return brand.empty() ? name : brand; }
};

/// Every model a library's sources made (`createModel`), by slug.
std::vector<Model*>& models();
/// The plugin a library's modules belong to.
Plugin* instance();
/// The plugins there are: a library is one module of one plugin.
extern std::vector<Plugin*> plugins;
/// A plugin by its slug: this one, or none.
Plugin* getPlugin(const std::string& pluginSlug);
inline Plugin* getPluginFallback(const std::string& pluginSlug) { return getPlugin(pluginSlug); }
/// A model of this plugin by its slug (its own sources' models).
Model* getModel(const std::string& pluginSlug, const std::string& modelSlug);
inline Model* getModelFallback(const std::string& pluginSlug, const std::string& modelSlug) {
	return getModel(pluginSlug, modelSlug);
}
/// The model a module's JSON names (`plugin`, `model`).
Model* modelFromJson(json_t* moduleJ);
/// Slugs are letters, digits, `-` and `_`.
bool isSlugValid(const std::string& slug);
std::string normalizeSlug(const std::string& slug);
} // namespace plugin

// ---- app -----------------------------------------------------------------------------------
namespace app {

static const float RACK_GRID_WIDTH = 15;
static const float RACK_GRID_HEIGHT = 380;
static const math::Vec RACK_GRID_SIZE = math::Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
static const math::Vec RACK_OFFSET = RACK_GRID_SIZE.mult(math::Vec(2000, 100));

struct CircularShadow : widget::TransparentWidget {
	float blurRadius = 0.f;
	float opacity = 0.15f;
};
struct PanelBorder : widget::TransparentWidget {};
/// A widget made from an SVG file is as big as its picture, where the
/// picture's size is known (a file of the plugin's own; Rack's own
/// components' files aren't here, and those keep the size they're given).
inline void sized(widget::Widget* widget, widget::Widget* fb, widget::Widget* picture) {
	if (picture->box.size.x > 0.f && picture->box.size.y > 0.f) {
		fb->box.size = picture->box.size;
		widget->box.size = picture->box.size;
	}
}
// The widgets drawn from SVG files are made of parts, as Rack's are (panel
// code reaches into them: `shadow->opacity = 0`, `sw->box.size`): a
// framebuffer holding a shadow and the picture. Nothing is drawn; the parts
// are there, and each keeps the picture it was given.
struct SvgPanel : widget::Widget {
	widget::FramebufferWidget* fb = nullptr;
	widget::SvgWidget* sw = nullptr;
	PanelBorder* panelBorder = nullptr;
	std::shared_ptr<window::Svg> svg;
	SvgPanel() {
		fb = new widget::FramebufferWidget;
		addChild(fb);
		sw = new widget::SvgWidget;
		fb->addChild(sw);
		panelBorder = new PanelBorder;
		fb->addChild(panelBorder);
	}
	void setBackground(std::shared_ptr<window::Svg> svg) {
		this->svg = svg;
		sw->setSvg(svg);
		fb->box.size = sw->box.size.round();
		panelBorder->box.size = fb->box.size;
		box.size = fb->box.size;
	}
};
DEPRECATED typedef SvgPanel SVGPanel;
struct ThemedSvgPanel : SvgPanel {
	std::shared_ptr<window::Svg> lightSvg;
	std::shared_ptr<window::Svg> darkSvg;
	void setBackground(std::shared_ptr<window::Svg> lightSvg, std::shared_ptr<window::Svg> darkSvg) {
		this->lightSvg = lightSvg;
		this->darkSvg = darkSvg;
		SvgPanel::setBackground(lightSvg);
	}
};
struct SvgScrew : widget::Widget {
	widget::FramebufferWidget* fb = nullptr;
	widget::SvgWidget* sw = nullptr;
	SvgScrew() {
		fb = new widget::FramebufferWidget;
		addChild(fb);
		sw = new widget::SvgWidget;
		fb->addChild(sw);
	}
	void setSvg(std::shared_ptr<window::Svg> svg) {
		sw->setSvg(svg);
		sized(this, fb, sw);
	}
};
struct ThemedSvgScrew : SvgScrew {
	std::shared_ptr<window::Svg> lightSvg;
	std::shared_ptr<window::Svg> darkSvg;
	void setSvg(std::shared_ptr<window::Svg> lightSvg, std::shared_ptr<window::Svg> darkSvg) {
		this->lightSvg = lightSvg;
		this->darkSvg = darkSvg;
		SvgScrew::setSvg(lightSvg);
	}
};

/// A knob, button or switch on a panel: which module's, which knob. The
/// plugin draws its own in its place and takes the pointer there, so the
/// pointer goes past this one (a widget of the module's own derived from
/// it still gets what it overrides). One that no knob of the module is
/// behind (a button that only acts, as a LOAD button) is the module's own:
/// the plugin has nothing in its place, so it's drawn here (its picture,
/// else a plain round button) and it takes the pointer, as in Rack.
struct ParamWidget : widget::OpaqueWidget {
	engine::Module* module = nullptr;
	int paramId = -1;
	ui::Tooltip* tooltip = nullptr;
	/// One of the module's own, held down.
	bool oroboroHeld = false;

	/// No knob of the module is behind it: it's the module's own.
	bool oroboroOwn() { return !getParamQuantity(); }
	void onHover(const HoverEvent& e) override {
		if (oroboroOwn())
			OpaqueWidget::onHover(e);
	}
	void onButton(const ButtonEvent& e) override {
		if (oroboroOwn())
			OpaqueWidget::onButton(e);
	}
	void onHoverScroll(const HoverScrollEvent& e) override {
		if (oroboroOwn())
			OpaqueWidget::onHoverScroll(e);
	}
	void onDragHover(const DragHoverEvent& e) override {
		if (oroboroOwn())
			OpaqueWidget::onDragHover(e);
	}
	void onDragStart(const DragStartEvent& e) override { oroboroHeld = oroboroOwn(); }
	void onDragEnd(const DragEndEvent& e) override { oroboroHeld = false; }
	void draw(const DrawArgs& args) override;

	virtual void initParamQuantity() {}
	engine::ParamQuantity* getParamQuantity() {
		if (!module || paramId < 0 || (size_t) paramId >= module->paramQuantities.size())
			return nullptr;
		return module->paramQuantities[paramId];
	}
	void createTooltip() {}
	void destroyTooltip() {}
	void createContextMenu() {}
	virtual void appendContextMenu(ui::Menu* menu) {}
	void resetAction() {}
};
struct Knob : ParamWidget {
	bool horizontal = false;
	bool smooth = true;
	bool snap = false;
	float speed = 1.f;
	bool forceLinear = false;
	float minAngle = -(float) M_PI;
	float maxAngle = (float) M_PI;
};
struct SvgKnob : Knob {
	widget::FramebufferWidget* fb = nullptr;
	CircularShadow* shadow = nullptr;
	widget::TransformWidget* tw = nullptr;
	widget::SvgWidget* sw = nullptr;
	SvgKnob() {
		fb = new widget::FramebufferWidget;
		addChild(fb);
		shadow = new CircularShadow;
		fb->addChild(shadow);
		tw = new widget::TransformWidget;
		fb->addChild(tw);
		sw = new widget::SvgWidget;
		tw->addChild(sw);
	}
	void setSvg(std::shared_ptr<window::Svg> svg) {
		sw->setSvg(svg);
		tw->box.size = sw->box.size;
		sized(this, fb, sw);
	}
	DEPRECATED void setSVG(std::shared_ptr<window::Svg> svg) { setSvg(svg); }
};
struct SliderKnob : Knob {};
struct SvgSlider : SliderKnob {
	widget::FramebufferWidget* fb = nullptr;
	widget::SvgWidget* background = nullptr;
	widget::SvgWidget* handle = nullptr;
	math::Vec minHandlePos, maxHandlePos;

	SvgSlider() {
		fb = new widget::FramebufferWidget;
		addChild(fb);
		background = new widget::SvgWidget;
		fb->addChild(background);
		handle = new widget::SvgWidget;
		fb->addChild(handle);
	}
	void setBackgroundSvg(std::shared_ptr<window::Svg> svg) {
		background->setSvg(svg);
		sized(this, fb, background);
	}
	void setHandleSvg(std::shared_ptr<window::Svg> svg) { handle->setSvg(svg); }
	void setHandlePos(math::Vec minHandlePos, math::Vec maxHandlePos) {
		this->minHandlePos = minHandlePos;
		this->maxHandlePos = maxHandlePos;
	}
	void setHandlePosCentered(math::Vec minHandlePosCentered, math::Vec maxHandlePosCentered) {
		setHandlePos(minHandlePosCentered, maxHandlePosCentered);
	}
	DEPRECATED void setBackgroundSVG(std::shared_ptr<window::Svg> svg) { setBackgroundSvg(svg); }
	DEPRECATED void setHandleSVG(std::shared_ptr<window::Svg> svg) { setHandleSvg(svg); }
	DEPRECATED void setSVGs(std::shared_ptr<window::Svg> backgroundSvg, std::shared_ptr<window::Svg> handleSvg) {
		if (backgroundSvg)
			setBackgroundSvg(backgroundSvg);
		setHandleSvg(handleSvg);
	}
};
struct Switch : ParamWidget {
	bool momentary = false;
};
struct SvgSwitch : Switch {
	widget::FramebufferWidget* fb = nullptr;
	CircularShadow* shadow = nullptr;
	widget::SvgWidget* sw = nullptr;
	std::vector<std::shared_ptr<window::Svg>> frames;
	bool latch = false;
	SvgSwitch() {
		fb = new widget::FramebufferWidget;
		addChild(fb);
		shadow = new CircularShadow;
		fb->addChild(shadow);
		sw = new widget::SvgWidget;
		fb->addChild(sw);
	}
	void addFrame(std::shared_ptr<window::Svg> svg) {
		frames.push_back(svg);
		// (the first frame is the one it shows, and gives it its size)
		if (frames.size() == 1) {
			sw->setSvg(svg);
			sized(this, fb, sw);
		}
	}
};
struct SvgButton : widget::OpaqueWidget {
	widget::FramebufferWidget* fb = nullptr;
	CircularShadow* shadow = nullptr;
	widget::SvgWidget* sw = nullptr;
	std::vector<std::shared_ptr<window::Svg>> frames;
	SvgButton() {
		fb = new widget::FramebufferWidget;
		addChild(fb);
		shadow = new CircularShadow;
		fb->addChild(shadow);
		sw = new widget::SvgWidget;
		fb->addChild(sw);
	}
	void addFrame(std::shared_ptr<window::Svg> svg) {
		frames.push_back(svg);
		if (frames.size() == 1) {
			sw->setSvg(svg);
			sized(this, fb, sw);
		}
	}
};

struct AudioButton : SvgButton {
	audio::Port* port = nullptr;
	void setAudioPort(audio::Port* port) { this->port = port; }
};
struct MidiButton : SvgButton {
	midi::Port* port = nullptr;
	void setMidiPort(midi::Port* port) { this->port = port; }
};
/// A port's menu of drivers and devices. Audio: there are none to choose
/// here. MIDI: the one driver and device are the plugin's, so its channel
/// is what's chosen (after the component library, below).
inline void appendAudioMenu(ui::Menu* menu, audio::Port* port) {}
void appendMidiMenu(ui::Menu* menu, midi::Port* port);

/// A jack on a panel: which module's, which jack. The plugin's own is in
/// its place: the pointer goes past this one.
struct PortWidget : widget::OpaqueWidget {
	void onHover(const HoverEvent& e) override {}
	void onButton(const ButtonEvent& e) override {}
	void onHoverScroll(const HoverScrollEvent& e) override {}
	void onDragHover(const DragHoverEvent& e) override {}
	engine::Module* module = nullptr;
	engine::Port::Type type = engine::Port::INPUT;
	int portId = -1;
	ui::Tooltip* tooltip = nullptr;

	engine::Port* getPort() {
		if (!module)
			return nullptr;
		if (type == engine::Port::INPUT)
			return (size_t) portId < module->inputs.size() ? &module->inputs[portId] : nullptr;
		return (size_t) portId < module->outputs.size() ? &module->outputs[portId] : nullptr;
	}
	engine::PortInfo* getPortInfo() {
		if (!module)
			return nullptr;
		if (type == engine::Port::INPUT)
			return (size_t) portId < module->inputInfos.size() ? module->inputInfos[portId] : nullptr;
		return (size_t) portId < module->outputInfos.size() ? module->outputInfos[portId] : nullptr;
	}
	void createTooltip() {}
	void destroyTooltip() {}
	void createContextMenu() {}
	virtual void appendContextMenu(ui::Menu* menu) {}
	void deleteTopCableAction() {}
};
struct SvgPort : PortWidget {
	widget::FramebufferWidget* fb = nullptr;
	CircularShadow* shadow = nullptr;
	widget::SvgWidget* sw = nullptr;
	SvgPort() {
		fb = new widget::FramebufferWidget;
		addChild(fb);
		shadow = new CircularShadow;
		fb->addChild(shadow);
		sw = new widget::SvgWidget;
		fb->addChild(sw);
	}
	void setSvg(std::shared_ptr<window::Svg> svg) {
		sw->setSvg(svg);
		sized(this, fb, sw);
	}
	DEPRECATED void setSVG(std::shared_ptr<window::Svg> svg) { setSvg(svg); }
};
struct ThemedSvgPort : SvgPort {
	std::shared_ptr<window::Svg> lightSvg;
	std::shared_ptr<window::Svg> darkSvg;
	void setSvg(std::shared_ptr<window::Svg> lightSvg, std::shared_ptr<window::Svg> darkSvg) {
		this->lightSvg = lightSvg;
		this->darkSvg = darkSvg;
		SvgPort::setSvg(lightSvg);
	}
};

// (Rack 1's names)
DEPRECATED typedef SvgButton SVGButton;
DEPRECATED typedef SvgKnob SVGKnob;
DEPRECATED typedef SvgPort SVGPort;
DEPRECATED typedef SvgScrew SVGScrew;
DEPRECATED typedef SvgSlider SVGSlider;
DEPRECATED typedef SvgSwitch SVGSwitch;

struct LightWidget : widget::TransparentWidget {
	NVGcolor bgColor = nvgRGBA(0, 0, 0, 0);
	NVGcolor color = nvgRGBA(0, 0, 0, 0);
	NVGcolor borderColor = nvgRGBA(0, 0, 0, 0);
	virtual void drawBackground(const DrawArgs& args) {}
	virtual void drawLight(const DrawArgs& args) {}
	virtual void drawHalo(const DrawArgs& args) {}
};
struct MultiLightWidget : LightWidget {
	std::vector<NVGcolor> baseColors;
	int getNumColors() { return (int) baseColors.size(); }
	void addBaseColor(NVGcolor baseColor) { baseColors.push_back(baseColor); }
	void setBrightnesses(const std::vector<float>& brightnesses) {}
};
struct ModuleLightWidget : MultiLightWidget {
	engine::Module* module = nullptr;
	int firstLightId = -1;
	ui::Tooltip* tooltip = nullptr;
	engine::Light* getLight(int colorId) {
		if (!module || firstLightId < 0)
			return nullptr;
		return &module->lights[firstLightId + colorId];
	}
	engine::LightInfo* getLightInfo() {
		if (!module || firstLightId < 0)
			return nullptr;
		return module->lightInfos[firstLightId];
	}
	void createTooltip() {}
	void destroyTooltip() {}
};

struct LedDisplay : widget::Widget {};
struct LedDisplaySeparator : widget::Widget {};
struct LedDisplayChoice : widget::OpaqueWidget {
	std::string text;
	std::string fontPath;
	math::Vec textOffset;
	NVGcolor color = nvgRGB(0xff, 0xd7, 0x14);
	NVGcolor bgColor = nvgRGBAf(0, 0, 0, 0);
};
struct LedDisplayTextField : ui::TextField {
	std::string fontPath;
	math::Vec textOffset;
	NVGcolor color = nvgRGB(0xff, 0xd7, 0x14);
	NVGcolor bgColor = nvgRGB(0, 0, 0);
};

// The audio and MIDI ports' displays and buttons. The plugin plays through
// its host's audio and MIDI: there are no drivers or devices to choose
// here, so the choices show what the port has, and a menu offers nothing.
struct AudioDriverChoice : LedDisplayChoice {
	audio::Port* port = nullptr;
};
struct AudioDeviceChoice : LedDisplayChoice {
	audio::Port* port = nullptr;
};
struct AudioSampleRateChoice : LedDisplayChoice {
	audio::Port* port = nullptr;
};
struct AudioBlockSizeChoice : LedDisplayChoice {
	audio::Port* port = nullptr;
};
struct AudioDeviceMenuChoice : AudioDeviceChoice {};
struct MidiDriverChoice : LedDisplayChoice {
	midi::Port* port = nullptr;
};
struct MidiDeviceChoice : LedDisplayChoice {
	midi::Port* port = nullptr;
};
struct MidiChannelChoice : LedDisplayChoice {
	midi::Port* port = nullptr;
};

/// Rows down a display, each a choice under a line, as tall as it's given.
template <class TChoice, class TPort>
TChoice* ledDisplayRow(LedDisplay* display, TPort* port, LedDisplaySeparator** separator, float& y, float height) {
	TChoice* choice = new TChoice;
	choice->port = port;
	choice->box.pos = math::Vec(0, y);
	choice->box.size = math::Vec(display->box.size.x, height);
	display->addChild(choice);
	y += height;
	if (separator) {
		*separator = new LedDisplaySeparator;
		(*separator)->box.pos = math::Vec(0, y);
		(*separator)->box.size = math::Vec(display->box.size.x, 0);
		display->addChild(*separator);
	}
	return choice;
}

struct AudioDisplay : LedDisplay {
	LedDisplayChoice* driverChoice = nullptr;
	LedDisplaySeparator* driverSeparator = nullptr;
	LedDisplayChoice* deviceChoice = nullptr;
	LedDisplaySeparator* deviceSeparator = nullptr;
	LedDisplayChoice* sampleRateChoice = nullptr;
	LedDisplaySeparator* sampleRateSeparator = nullptr;
	LedDisplayChoice* bufferSizeChoice = nullptr;

	void setAudioPort(audio::Port* port) {
		clearChildren();
		float y = 0.f, h = box.size.y > 0.f ? box.size.y / 3.f : 15.f;
		driverChoice = ledDisplayRow<AudioDriverChoice>(this, port, &driverSeparator, y, h);
		deviceChoice = ledDisplayRow<AudioDeviceChoice>(this, port, &deviceSeparator, y, h);
		sampleRateChoice = ledDisplayRow<AudioSampleRateChoice>(this, port, &sampleRateSeparator, y, h);
		bufferSizeChoice = ledDisplayRow<AudioBlockSizeChoice>(this, port, nullptr, y, h);
		// (the sample rate and the block size share the last row)
		sampleRateChoice->box.size.x = bufferSizeChoice->box.size.x = box.size.x / 2.f;
		bufferSizeChoice->box.pos = math::Vec(box.size.x / 2.f, sampleRateChoice->box.pos.y);
		driverChoice->text = "Host";
	}
};
struct MidiDisplay : LedDisplay {
	LedDisplayChoice* driverChoice = nullptr;
	LedDisplaySeparator* driverSeparator = nullptr;
	LedDisplayChoice* deviceChoice = nullptr;
	LedDisplaySeparator* deviceSeparator = nullptr;
	LedDisplayChoice* channelChoice = nullptr;

	void setMidiPort(midi::Port* port) {
		clearChildren();
		float y = 0.f, h = box.size.y > 0.f ? box.size.y / 3.f : 15.f;
		driverChoice = ledDisplayRow<MidiDriverChoice>(this, port, &driverSeparator, y, h);
		deviceChoice = ledDisplayRow<MidiDeviceChoice>(this, port, &deviceSeparator, y, h);
		channelChoice = ledDisplayRow<MidiChannelChoice>(this, port, nullptr, y, h);
		if (port) {
			midi::Driver* driver = midi::getDriver(port->getDriverId());
			driverChoice->text = driver ? driver->getName() : "No driver";
			deviceChoice->text = port->getDeviceId() >= 0 ? port->getDeviceName(port->getDeviceId()) : "No device";
			channelChoice->text = port->getChannelName(port->getChannel());
		}
	}
};

/// A cable's end. There are no cables here (a module plays alone), so
/// there are none to find, but they can be made.
struct PlugWidget : widget::Widget {
	CableWidget* cable = nullptr;
	engine::Port::Type type = engine::Port::INPUT;
	CableWidget* getCable() { return cable; }
	engine::Port::Type getType() { return type; }
};
struct CableWidget : widget::Widget {
	engine::Cable* cable = nullptr;
	NVGcolor color = nvgRGB(0, 0, 0);
	PlugWidget* inputPlug = nullptr;
	PlugWidget* outputPlug = nullptr;
	PortWidget* inputPort = nullptr;
	PortWidget* outputPort = nullptr;
	PortWidget* hoveredInputPort = nullptr;
	PortWidget* hoveredOutputPort = nullptr;

	bool isComplete() { return inputPort && outputPort; }
	void updateCable() {}
	void setCable(engine::Cable* cable) { this->cable = cable; }
	engine::Cable* getCable() { return cable; }
	PlugWidget*& getPlug(engine::Port::Type type) { return type == engine::Port::INPUT ? inputPlug : outputPlug; }
	PortWidget*& getPort(engine::Port::Type type) { return type == engine::Port::INPUT ? inputPort : outputPort; }
	PortWidget*& getHoveredPort(engine::Port::Type type) {
		return type == engine::Port::INPUT ? hoveredInputPort : hoveredOutputPort;
	}
	/// Where its ends are: the middles of its jacks, in the window.
	math::Vec getInputPos() { return inputPort ? inputPort->getAbsoluteOffset(inputPort->box.size.div(2)) : math::Vec(); }
	math::Vec getOutputPos() { return outputPort ? outputPort->getAbsoluteOffset(outputPort->box.size.div(2)) : math::Vec(); }
	json_t* toJson() { return nullptr; }
	void mergeJson(json_t* rootJ) {}
	void fromJson(json_t* rootJ) {}
	engine::Cable* releaseCable() {
		engine::Cable* c = cable;
		cable = nullptr;
		return c;
	}
};
struct RailWidget : widget::TransparentWidget {};

/// A module's panel: the widgets its constructor puts on it. Kept, so a
/// module that asks for one of them gets it; never shown.
struct ModuleWidget : widget::OpaqueWidget {
	plugin::Model* model = nullptr;
	engine::Module* module = nullptr;
	widget::Widget* panel = nullptr;
	std::vector<ParamWidget*> paramWidgets;
	std::vector<PortWidget*> inputWidgets;
	std::vector<PortWidget*> outputWidgets;

	ModuleWidget() {}
	DEPRECATED ModuleWidget(engine::Module* module) { setModule(module); }
	plugin::Model* getModel() { return model; }
	void setModel(plugin::Model* model) { this->model = model; }
	engine::Module* getModule() { return module; }
	template <class TModule>
	TModule* getModule() {
		return dynamic_cast<TModule*>(module);
	}
	void setModule(engine::Module* module) { this->module = module; }
	widget::Widget* getPanel() { return panel; }
	void setPanel(widget::Widget* panel) {
		if (this->panel) {
			removeChild(this->panel);
			delete this->panel;
		}
		this->panel = panel;
		if (panel) {
			addChildBottom(panel);
			box.size.x = std::round(panel->box.size.x / RACK_GRID_WIDTH) * RACK_GRID_WIDTH;
			box.size.y = RACK_GRID_HEIGHT;
		}
	}
	void setPanel(std::shared_ptr<window::Svg> svg) {
		SvgPanel* panel = new SvgPanel;
		panel->setBackground(svg);
		setPanel(panel);
	}
	void addParam(ParamWidget* param) {
		paramWidgets.push_back(param);
		addChild(param);
	}
	void addInput(PortWidget* input) {
		inputWidgets.push_back(input);
		addChild(input);
	}
	void addOutput(PortWidget* output) {
		outputWidgets.push_back(output);
		addChild(output);
	}
	ParamWidget* getParam(int paramId) {
		for (ParamWidget* w : paramWidgets)
			if (w->paramId == paramId)
				return w;
		return nullptr;
	}
	PortWidget* getInput(int portId) {
		for (PortWidget* w : inputWidgets)
			if (w->portId == portId)
				return w;
		return nullptr;
	}
	PortWidget* getOutput(int portId) {
		for (PortWidget* w : outputWidgets)
			if (w->portId == portId)
				return w;
		return nullptr;
	}
	std::vector<ParamWidget*> getParams() { return paramWidgets; }
	std::vector<PortWidget*> getPorts() {
		std::vector<PortWidget*> ports = inputWidgets;
		ports.insert(ports.end(), outputWidgets.begin(), outputWidgets.end());
		return ports;
	}
	std::vector<PortWidget*> getInputs() { return inputWidgets; }
	std::vector<PortWidget*> getOutputs() { return outputWidgets; }
	virtual void appendContextMenu(ui::Menu* menu) {}
	json_t* toJson() { return nullptr; }
	void fromJson(json_t* rootJ) {}
	void resetAction() {}
	void randomizeAction() {}
	void disconnect() {}
	void createContextMenu() {}
	// (presets, the clipboard and the rack's moves are the plugin's, not a
	// module's: these do nothing here)
	bool pasteJsonAction(json_t* rootJ) { return false; }
	void copyClipboard() {}
	bool pasteClipboardAction() { return false; }
	void load(std::string filename) {}
	void loadAction(std::string filename) {}
	void loadTemplate() {}
	void loadDialog() {}
	void save(std::string filename) {}
	void saveTemplate() {}
	void saveTemplateDialog() {}
	bool hasTemplate() { return false; }
	void clearTemplate() {}
	void clearTemplateDialog() {}
	void saveDialog() {}
	void appendDisconnectActions(history::ComplexAction* complexAction) {}
	void disconnectAction() {}
	void cloneAction(bool cloneCables = true) {}
	void bypassAction(bool bypassed) {}
	void removeAction() {}
	void setGridPosition(math::Vec pos) {}
	math::Vec getGridPosition() { return math::Vec(); }
	math::Vec getGridSize() { return math::Vec(); }
	math::Rect getGridBox() { return math::Rect(); }
	DEPRECATED void setPanel(std::shared_ptr<window::Svg> svg, bool) { setPanel(svg); }
};

/// The rack. A module plays alone here: there's no rack of modules, no
/// cables and no selection to act on, so it answers as an empty one.
struct RackWidget : widget::OpaqueWidget {
	ParamWidget* touchedParam = nullptr;

	widget::Widget* getModuleContainer() { return this; }
	widget::Widget* getPlugContainer() { return this; }
	widget::Widget* getCableContainer() { return this; }
	void clear() {}
	void mergeJson(json_t* rootJ) {}
	void fromJson(json_t* rootJ) {}
	void pasteJsonAction(json_t* rootJ) {}
	void pasteModuleJsonAction(json_t* moduleJ) {}
	void pasteClipboardAction() {}
	void addModuleAtMouse(ModuleWidget* mw) { addModule(mw); }
	void removeModule(ModuleWidget* mw) {
		if (mw && mw->parent == this)
			removeChild(mw);
	}
	bool hasModules() { return false; }
	void updateModuleOldPositions() {}
	history::ComplexAction* getModuleDragAction() { return nullptr; }
	void updateSelectionFromRect() {}
	void selectAll() {}
	void deselectAll() {}
	void select(ModuleWidget* mw, bool selected = true) {}
	bool hasSelection() { return false; }
	const std::set<ModuleWidget*>& getSelected() {
		static const std::set<ModuleWidget*> none;
		return none;
	}
	json_t* selectionToJson(bool cables = true) { return nullptr; }
	void loadSelection(std::string path) {}
	void loadSelectionDialog() {}
	void saveSelection(std::string path) {}
	void saveSelectionDialog() {}
	void copyClipboardSelection() {}
	void resetSelectionAction() {}
	void randomizeSelectionAction() {}
	void disconnectSelectionAction() {}
	void cloneSelectionAction(bool cloneCables = true) {}
	void bypassSelectionAction(bool bypassed) {}
	bool isSelectionBypassed() { return false; }
	void deleteSelectionAction() {}
	bool requestSelectionPos(math::Vec delta) { return false; }
	void setSelectionPosNearest(math::Vec delta) {}
	void appendSelectionContextMenu(ui::Menu* menu) {}
	void clearCables() {}
	void clearCablesAction() {}
	void clearCablesOnPort(PortWidget* port) {}
	/// A cable made by a module's code: never shown, and deleted.
	void addCable(CableWidget* cw) { delete cw; }
	void removeCable(CableWidget* cw) {}
	DEPRECATED CableWidget* getIncompleteCable() { return nullptr; }
	PlugWidget* getTopPlug(PortWidget* port) { return nullptr; }
	CableWidget* getCable(int64_t cableId) { return nullptr; }
	CableWidget* getCable(PortWidget* outputPort, PortWidget* inputPort) { return nullptr; }
	std::vector<CableWidget*> getCables() { return {}; }
	std::vector<CableWidget*> getCompleteCables() { return {}; }
	std::vector<CableWidget*> getIncompleteCables() { return {}; }
	int getNextCableColorId() { return 0; }
	void setNextCableColorId(int id) {}
	ParamWidget* getTouchedParam() { return touchedParam; }

	std::vector<CableWidget*> getCablesOnPort(PortWidget* port) { return {}; }
	std::vector<CableWidget*> getCompleteCablesOnPort(PortWidget* port) { return {}; }
	CableWidget* getTopCable(PortWidget* port) { return nullptr; }
	ModuleWidget* getModule(int64_t moduleId) { return nullptr; }
	std::vector<ModuleWidget*> getModules() { return {}; }
	bool isSelected(ModuleWidget* mw) { return false; }
	NVGcolor getNextCableColor() { return nvgRGB(0, 0, 0); }
	void setTouchedParam(ParamWidget* pw) { touchedParam = pw; }
	math::Vec getMousePos() { return math::Vec(); }
	/// Where a module goes (a resize handle moves it as it widens): here
	/// there's no rack to make room in, so it goes where it's asked.
	bool requestModulePos(ModuleWidget* mw, math::Vec pos) {
		if (mw)
			mw->box.pos = pos;
		return true;
	}
	void setModulePosNearest(ModuleWidget* mw, math::Vec pos) { requestModulePos(mw, pos); }
	void setModulePosForce(ModuleWidget* mw, math::Vec pos) { requestModulePos(mw, pos); }
	void setModulePosSqueeze(ModuleWidget* mw, math::Vec pos) { requestModulePos(mw, pos); }
	/// There's no rack to add a module's panel to here (`Engine::addModule`):
	/// it's kept, out of sight.
	void addModule(ModuleWidget* mw) {
		if (mw)
			addChild(mw);
	}
};
struct RackScrollWidget : ui::ScrollWidget {
	widget::ZoomWidget* zoomWidget = nullptr;
	RackWidget* rackWidget = nullptr;
	RackScrollWidget() { zoomWidget = new widget::ZoomWidget; }
	void reset() {}
	math::Vec getGridOffset() { return math::Vec(); }
	void setGridOffset(math::Vec gridOffset) {}
	float getZoom() { return 1.f; }
	void setZoom(float zoom) {}
	void setZoom(float zoom, math::Vec pivot) {}
	void zoomToModules() {}
	void zoomToBound(math::Rect bound) {}
};
struct Scene : widget::OpaqueWidget {
	RackScrollWidget* rackScroll = nullptr;
	RackWidget* rack = nullptr;
	widget::Widget* menuBar = nullptr;
	widget::Widget* browser = nullptr;
	math::Vec mousePos;
	math::Vec getMousePos() { return mousePos; }
};
} // namespace app

// ---- the app's context ---------------------------------------------------------------------
struct Context {
	event::State* event = nullptr;
	app::Scene* scene = nullptr;
	engine::Engine* engine = nullptr;
	window::Window* window = nullptr;
	history::State* history = nullptr;
	patch::Manager* patch = nullptr;
	midiloopback::Context* midiLoopbackContext = nullptr;
};
/// The one context a library has: an engine at the rate the plugin runs
/// its modules at, a window that loads nothing, a scene with nothing in it.
Context* contextGet();
inline void contextSet(Context* context) {}
#define APP rack::contextGet()
DEPRECATED inline Context* appGet() { return contextGet(); }

namespace settings {
extern bool preferDarkPanels;
extern float rackBrightness;
extern float haloBrightness;
extern float knobScrollSensitivity;
extern bool tooltips;
extern bool cpuMeter;
extern float sampleRate;
extern int threadCount;
extern bool paramTooltip;
extern bool lockModules;
extern float cableOpacity;
extern float cableTension;
extern bool devMode;
extern bool headless;
// (the settings a module may read; Rack's own, of its window and its
// patches, aren't here)
extern bool isPlugin;
extern std::string language;
extern float pixelRatio;
extern std::string uiTheme;
extern bool invertZoom;
extern bool mouseWheelZoom;
extern bool allowCursorLock;
enum KnobMode {
	KNOB_MODE_LINEAR,
	KNOB_MODE_SCALED_LINEAR,
	KNOB_MODE_ROTARY_ABSOLUTE,
	KNOB_MODE_ROTARY_RELATIVE,
};
extern KnobMode knobMode;
extern bool knobScroll;
extern float knobLinearSensitivity;
extern bool squeezeModules;
extern float frameRateLimit;
extern float autosaveInterval;
extern std::vector<NVGcolor> cableColors;
extern std::vector<std::string> cableLabels;
extern bool cableAutoRotate;
// (the module browser's: the plugin has its own, so they're only kept)
enum BrowserSort {
	BROWSER_SORT_UPDATED,
	BROWSER_SORT_LAST_USED,
	BROWSER_SORT_MOST_USED,
	BROWSER_SORT_BRAND,
	BROWSER_SORT_NAME,
	BROWSER_SORT_RANDOM,
};
extern BrowserSort browserSort;
extern float browserZoom;
/// Every module is in the browser here.
inline bool isModuleWhitelisted(const std::string& pluginSlug, const std::string& moduleSlug) { return true; }
} // namespace settings

// ---- the component library -----------------------------------------------------------------
namespace componentlibrary {
using namespace window;

static const NVGcolor SCHEME_BLACK_TRANSPARENT = nvgRGBA(0x00, 0x00, 0x00, 0x00);
static const NVGcolor SCHEME_BLACK = nvgRGB(0x00, 0x00, 0x00);
static const NVGcolor SCHEME_WHITE = nvgRGB(0xff, 0xff, 0xff);
static const NVGcolor SCHEME_RED = nvgRGB(0xed, 0x2c, 0x24);
static const NVGcolor SCHEME_ORANGE = nvgRGB(0xf2, 0xb1, 0x20);
static const NVGcolor SCHEME_YELLOW = nvgRGB(0xff, 0xd7, 0x14);
static const NVGcolor SCHEME_GREEN = nvgRGB(0x90, 0xc7, 0x3e);
static const NVGcolor SCHEME_CYAN = nvgRGB(0x22, 0xe6, 0xef);
static const NVGcolor SCHEME_BLUE = nvgRGB(0x29, 0xb2, 0xef);
static const NVGcolor SCHEME_PURPLE = nvgRGB(0xd5, 0x2b, 0xed);
static const NVGcolor SCHEME_LIGHT_GRAY = nvgRGB(0xe6, 0xe6, 0xe6);
static const NVGcolor SCHEME_DARK_GRAY = nvgRGB(0x17, 0x17, 0x17);

// lights
template <typename TBase = app::ModuleLightWidget>
struct TSvgLight : TBase {
	widget::FramebufferWidget* fb = nullptr;
	widget::SvgWidget* sw = nullptr;
	TSvgLight() {
		fb = new widget::FramebufferWidget;
		this->addChild(fb);
		sw = new widget::SvgWidget;
		fb->addChild(sw);
	}
	void setSvg(std::shared_ptr<window::Svg> svg) {
		sw->setSvg(svg);
		app::sized(this, fb, sw);
	}
};
typedef TSvgLight<> SvgLight;
template <typename TBase = app::ModuleLightWidget>
struct TGrayModuleLightWidget : TBase {};
typedef TGrayModuleLightWidget<> GrayModuleLightWidget;
template <typename TBase = GrayModuleLightWidget>
struct TWhiteLight : TBase {
	TWhiteLight() { this->addBaseColor(SCHEME_WHITE); }
};
typedef TWhiteLight<> WhiteLight;
template <typename TBase = GrayModuleLightWidget>
struct TRedLight : TBase {
	TRedLight() { this->addBaseColor(SCHEME_RED); }
};
typedef TRedLight<> RedLight;
template <typename TBase = GrayModuleLightWidget>
struct TGreenLight : TBase {
	TGreenLight() { this->addBaseColor(SCHEME_GREEN); }
};
typedef TGreenLight<> GreenLight;
template <typename TBase = GrayModuleLightWidget>
struct TBlueLight : TBase {
	TBlueLight() { this->addBaseColor(SCHEME_BLUE); }
};
typedef TBlueLight<> BlueLight;
template <typename TBase = GrayModuleLightWidget>
struct TYellowLight : TBase {
	TYellowLight() { this->addBaseColor(SCHEME_YELLOW); }
};
typedef TYellowLight<> YellowLight;
template <typename TBase = GrayModuleLightWidget>
struct TGreenRedLight : TBase {
	TGreenRedLight() {
		this->addBaseColor(SCHEME_GREEN);
		this->addBaseColor(SCHEME_RED);
	}
};
typedef TGreenRedLight<> GreenRedLight;
template <typename TBase = GrayModuleLightWidget>
struct TRedGreenBlueLight : TBase {
	TRedGreenBlueLight() {
		this->addBaseColor(SCHEME_RED);
		this->addBaseColor(SCHEME_GREEN);
		this->addBaseColor(SCHEME_BLUE);
	}
};
typedef TRedGreenBlueLight<> RedGreenBlueLight;

template <typename TBase>
struct LargeLight : TGrayModuleLightWidget<TBase> {
	LargeLight() { this->box.size = mm2px(math::Vec(5.179, 5.179)); }
};
template <typename TBase>
struct MediumLight : TGrayModuleLightWidget<TBase> {
	MediumLight() { this->box.size = mm2px(math::Vec(3.176, 3.176)); }
};
template <typename TBase>
struct SmallLight : TGrayModuleLightWidget<TBase> {
	SmallLight() { this->box.size = mm2px(math::Vec(2.176, 2.176)); }
};
template <typename TBase>
struct TinyLight : TGrayModuleLightWidget<TBase> {
	TinyLight() { this->box.size = mm2px(math::Vec(1.088, 1.088)); }
};
template <typename TBase>
struct LargeSimpleLight : TBase {
	LargeSimpleLight() { this->box.size = mm2px(math::Vec(5.179, 5.179)); }
};
template <typename TBase>
struct MediumSimpleLight : TBase {
	MediumSimpleLight() { this->box.size = mm2px(math::Vec(3.176, 3.176)); }
};
template <typename TBase>
struct SmallSimpleLight : TBase {
	SmallSimpleLight() { this->box.size = mm2px(math::Vec(2.0, 2.0)); }
};
template <typename TBase>
struct TinySimpleLight : TBase {
	TinySimpleLight() { this->box.size = mm2px(math::Vec(1.0, 1.0)); }
};
/// A light that's a box rather than a round one.
template <typename TBase>
struct RectangleLight : TBase {};
template <typename TBase>
struct VCVBezelLight : TBase {
	VCVBezelLight() {
		this->borderColor = color::BLACK_TRANSPARENT;
		this->bgColor = color::BLACK_TRANSPARENT;
		this->box.size = mm2px(math::Vec(6.0, 6.0));
	}
};
template <typename TBase>
using LEDBezelLight = VCVBezelLight<TBase>;
template <typename TBase>
struct PB61303Light : TBase {
	PB61303Light() { this->box.size = mm2px(math::Vec(9.0, 9.0)); }
};

// knobs: a size each. Their parts are there, as on Rack's (a knob of a
// plugin's own sets its pictures on `bg`, `fg`): a picture under the one
// that turns, and over it.
struct RoundKnob : app::SvgKnob {
	widget::SvgWidget* bg = nullptr;
	RoundKnob() {
		minAngle = -0.83f * (float) M_PI;
		maxAngle = 0.83f * (float) M_PI;
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
	}
};
#define ORO_RACK_KNOB(Name, Base, mm) \
	struct Name : Base { \
		Name() { box.size = mm2px(math::Vec(mm, mm)); } \
	}
ORO_RACK_KNOB(RoundBlackKnob, RoundKnob, 10.0);
ORO_RACK_KNOB(RoundSmallBlackKnob, RoundKnob, 8.0);
ORO_RACK_KNOB(RoundLargeBlackKnob, RoundKnob, 12.7);
ORO_RACK_KNOB(RoundBigBlackKnob, RoundKnob, 15.2);
ORO_RACK_KNOB(RoundHugeBlackKnob, RoundKnob, 19.0);
struct RoundBlackSnapKnob : RoundBlackKnob {
	RoundBlackSnapKnob() { snap = true; }
};
struct Davies1900hKnob : app::SvgKnob {
	widget::SvgWidget* bg = nullptr;
	Davies1900hKnob() {
		minAngle = -0.83f * (float) M_PI;
		maxAngle = 0.83f * (float) M_PI;
		box.size = mm2px(math::Vec(12.0, 12.0));
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
	}
};
struct Davies1900hWhiteKnob : Davies1900hKnob {};
struct Davies1900hBlackKnob : Davies1900hKnob {};
struct Davies1900hRedKnob : Davies1900hKnob {};
ORO_RACK_KNOB(Davies1900hLargeWhiteKnob, Davies1900hKnob, 18.0);
ORO_RACK_KNOB(Davies1900hLargeBlackKnob, Davies1900hKnob, 18.0);
ORO_RACK_KNOB(Davies1900hLargeRedKnob, Davies1900hKnob, 18.0);
struct Rogan : app::SvgKnob {
	widget::SvgWidget* bg = nullptr;
	widget::SvgWidget* fg = nullptr;
	Rogan() {
		minAngle = -0.83f * (float) M_PI;
		maxAngle = 0.83f * (float) M_PI;
		box.size = mm2px(math::Vec(11.6, 11.6));
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		fg = new widget::SvgWidget;
		fb->addChildAbove(fg, tw);
	}
};
struct Rogan6PSWhite : Rogan {};
struct Rogan5PSGray : Rogan {};
struct Rogan3PSBlue : Rogan {};
struct Rogan3PSRed : Rogan {};
struct Rogan3PSGreen : Rogan {};
struct Rogan3PSWhite : Rogan {};
struct Rogan3PBlue : Rogan {};
struct Rogan3PRed : Rogan {};
struct Rogan3PGreen : Rogan {};
struct Rogan3PWhite : Rogan {};
struct Rogan2SGray : Rogan {};
struct Rogan2PSBlue : Rogan {};
struct Rogan2PSRed : Rogan {};
struct Rogan2PSGreen : Rogan {};
struct Rogan2PSWhite : Rogan {};
struct Rogan2PBlue : Rogan {};
struct Rogan2PRed : Rogan {};
struct Rogan2PGreen : Rogan {};
struct Rogan2PWhite : Rogan {};
struct Rogan1PSBlue : Rogan {};
struct Rogan1PSRed : Rogan {};
struct Rogan1PSGreen : Rogan {};
struct Rogan1PSWhite : Rogan {};
struct Rogan1PBlue : Rogan {};
struct Rogan1PRed : Rogan {};
struct Rogan1PGreen : Rogan {};
struct Rogan1PWhite : Rogan {};
struct SynthTechAlco : app::SvgKnob {
	widget::SvgWidget* bg = nullptr;
	SynthTechAlco() {
		minAngle = -0.82f * (float) M_PI;
		maxAngle = 0.82f * (float) M_PI;
		box.size = mm2px(math::Vec(15.0, 15.0));
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
	}
};
struct Trimpot : app::SvgKnob {
	widget::SvgWidget* bg = nullptr;
	Trimpot() {
		minAngle = -0.75f * (float) M_PI;
		maxAngle = 0.75f * (float) M_PI;
		box.size = mm2px(math::Vec(6.3, 6.3));
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
	}
};
struct BefacoBigKnob : app::SvgKnob {
	widget::SvgWidget* bg = nullptr;
	BefacoBigKnob() {
		minAngle = -0.75f * (float) M_PI;
		maxAngle = 0.75f * (float) M_PI;
		box.size = mm2px(math::Vec(25.0, 25.0));
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
	}
};
struct BefacoTinyKnob : app::SvgKnob {
	widget::SvgWidget* bg = nullptr;
	BefacoTinyKnob() {
		minAngle = -0.8f * (float) M_PI;
		maxAngle = 0.8f * (float) M_PI;
		box.size = mm2px(math::Vec(10.0, 10.0));
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
	}
};
struct BefacoSlidePot : app::SvgSlider {
	BefacoSlidePot() { box.size = mm2px(math::Vec(4.5, 41.0)); }
};
struct VCVSlider : app::SvgSlider {
	VCVSlider() { box.size = mm2px(math::Vec(7.0, 30.0)); }
};
typedef VCVSlider LEDSlider;
struct VCVSliderHorizontal : app::SvgSlider {
	VCVSliderHorizontal() {
		horizontal = true;
		box.size = mm2px(math::Vec(30.0, 7.0));
	}
};
typedef VCVSliderHorizontal LEDSliderHorizontal;
template <typename TBase, typename TLightBase = RedLight>
struct LightSlider : TBase {
	app::ModuleLightWidget* light;
	LightSlider() {
		light = new TLightBase;
		this->addChild(light);
	}
	app::ModuleLightWidget* getLight() { return light; }
	/// The light rides on the handle, in its middle.
	void step() override {
		TBase::step();
		light->box.pos = this->handle->box.pos.plus(this->handle->box.size.div(2)).minus(light->box.size.div(2));
	}
};
template <typename TBase>
struct VCVSliderLight : RectangleLight<TSvgLight<TBase>> {};
template <typename TBase>
using LEDSliderLight = VCVSliderLight<TBase>;
template <typename TLightBase = RedLight>
struct VCVLightSlider : LightSlider<VCVSlider, VCVSliderLight<TLightBase>> {};
struct LEDSliderGreen : VCVLightSlider<GreenLight> {};
struct LEDSliderRed : VCVLightSlider<RedLight> {};
struct LEDSliderYellow : VCVLightSlider<YellowLight> {};
struct LEDSliderBlue : VCVLightSlider<BlueLight> {};
struct LEDSliderWhite : VCVLightSlider<WhiteLight> {};
template <typename TLightBase = RedLight>
using LEDLightSlider = VCVLightSlider<TLightBase>;
template <typename TLightBase = RedLight>
struct VCVLightSliderHorizontal : LightSlider<VCVSliderHorizontal, TLightBase> {};

// jacks
struct PJ301MPort : app::SvgPort {
	PJ301MPort() { box.size = mm2px(math::Vec(8.2, 8.2)); }
};
struct DarkPJ301MPort : PJ301MPort {};
struct ThemedPJ301MPort : app::ThemedSvgPort {
	ThemedPJ301MPort() { box.size = mm2px(math::Vec(8.2, 8.2)); }
};
struct PJ3410Port : app::SvgPort {
	PJ3410Port() { box.size = mm2px(math::Vec(10.8, 10.8)); }
};
struct CL1362Port : app::SvgPort {
	CL1362Port() { box.size = mm2px(math::Vec(11.0, 11.0)); }
};

// switches and buttons
struct MomentarySwitch : app::SvgSwitch {
	MomentarySwitch() { momentary = true; }
};
struct NKK : app::SvgSwitch {
	NKK() { box.size = mm2px(math::Vec(9.0, 13.0)); }
};
struct CKSS : app::SvgSwitch {
	CKSS() { box.size = mm2px(math::Vec(4.7, 7.0)); }
};
struct CKSSThree : app::SvgSwitch {
	CKSSThree() { box.size = mm2px(math::Vec(4.7, 9.5)); }
};
struct CKSSThreeHorizontal : app::SvgSwitch {
	CKSSThreeHorizontal() { box.size = mm2px(math::Vec(9.5, 4.7)); }
};
struct CKD6 : app::SvgSwitch {
	CKD6() {
		momentary = true;
		box.size = mm2px(math::Vec(9.5, 9.5));
	}
};
struct TL1105 : app::SvgSwitch {
	TL1105() {
		momentary = true;
		box.size = mm2px(math::Vec(5.5, 5.5));
	}
};
struct VCVButton : app::SvgSwitch {
	VCVButton() {
		momentary = true;
		box.size = mm2px(math::Vec(6.0, 6.0));
	}
};
typedef VCVButton LEDButton;
struct VCVLatch : VCVButton {
	VCVLatch() {
		momentary = false;
		latch = true;
	}
};
/// A button with a light in its middle.
template <typename TBase, typename TLight = WhiteLight>
struct LightButton : TBase {
	app::ModuleLightWidget* light;
	LightButton() {
		light = new TLight;
		light->box.pos = this->box.size.minus(light->box.size).div(2);
		this->addChild(light);
	}
	app::ModuleLightWidget* getLight() { return light; }
};
template <typename TLight = WhiteLight>
using VCVLightButton = LightButton<VCVButton, TLight>;
template <typename TLight = WhiteLight>
using LEDLightButton = VCVLightButton<TLight>;
template <typename TLight = WhiteLight>
struct VCVLightLatch : VCVLightButton<TLight> {
	VCVLightLatch() {
		this->momentary = false;
		this->latch = true;
	}
};
struct BefacoSwitch : app::SvgSwitch {
	BefacoSwitch() { box.size = mm2px(math::Vec(9.0, 13.0)); }
};
struct BefacoPush : app::SvgSwitch {
	BefacoPush() {
		momentary = true;
		box.size = mm2px(math::Vec(9.5, 9.5));
	}
};
struct VCVBezel : app::SvgSwitch {
	VCVBezel() {
		momentary = true;
		box.size = mm2px(math::Vec(8.0, 8.0));
	}
};
typedef VCVBezel LEDBezel;
struct VCVBezelLatch : VCVBezel {
	VCVBezelLatch() {
		momentary = false;
		latch = true;
	}
};
template <typename TLightBase = WhiteLight>
struct VCVLightBezel : VCVBezel {
	app::ModuleLightWidget* light;
	VCVLightBezel() {
		light = new VCVBezelLight<TLightBase>;
		addChild(light);
	}
	app::ModuleLightWidget* getLight() { return light; }
};
template <typename TLightBase = WhiteLight>
using LEDLightBezel = VCVLightBezel<TLightBase>;
template <typename TLightBase = WhiteLight>
struct VCVLightBezelLatch : VCVLightBezel<TLightBase> {
	VCVLightBezelLatch() {
		this->momentary = false;
		this->latch = true;
	}
};
struct PB61303 : app::SvgSwitch {
	PB61303() {
		momentary = true;
		box.size = mm2px(math::Vec(13.0, 13.0));
	}
};

// screws
struct ScrewSilver : app::SvgScrew {
	ScrewSilver() { box.size = math::Vec(15, 15); }
};
struct ScrewBlack : app::SvgScrew {
	ScrewBlack() { box.size = math::Vec(15, 15); }
};
struct ThemedScrew : app::ThemedSvgScrew {
	ThemedScrew() { box.size = math::Vec(15, 15); }
};
/// A row (or column) of box lights, a gap between each.
struct SegmentDisplay : widget::Widget {
	int lightsLen = 0;
	bool vertical = false;
	float margin = mm2px(0.5);

	template <typename TLightBase = WhiteLight>
	void setLights(engine::Module* module, int firstLightId, int lightsLen) {
		clearChildren();
		this->lightsLen = lightsLen;
		float length = (vertical ? box.size.y : box.size.x) - margin;
		for (int i = 0; i < lightsLen; i++) {
			app::ModuleLightWidget* light = new RectangleLight<TLightBase>;
			float at = length * i / lightsLen + margin;
			float size = length / lightsLen - margin;
			light->box = vertical ? math::Rect(0, at, box.size.x, size) : math::Rect(at, 0, size, box.size.y);
			light->module = module;
			light->firstLightId = firstLightId;
			firstLightId += light->getNumColors();
			addChild(light);
		}
	}
};

// the audio and MIDI ports' sockets
struct AudioButton_ADAT : app::AudioButton {
	AudioButton_ADAT() { shadow->opacity = 0.f; }
};
struct AudioButton_USB_B : app::AudioButton {
	AudioButton_USB_B() { shadow->opacity = 0.f; }
};
struct MidiButton_MIDI_DIN : app::MidiButton {
	MidiButton_MIDI_DIN() { shadow->opacity = 0.f; }
};
} // namespace componentlibrary

// ---- helpers -------------------------------------------------------------------------------

/// Registers a kind of module: `TModule` is made for each instance;
/// `TModuleWidget` is its panel, which isn't made here.
template <class TModule, class TModuleWidget>
plugin::Model* createModel(std::string slug) {
	struct TModel : plugin::Model {
		engine::Module* createModule() override {
			engine::Module* m = new TModule;
			m->model = this;
			return m;
		}
		app::ModuleWidget* createModuleWidget(engine::Module* m) override {
			TModule* tm = nullptr;
			if (m)
				tm = dynamic_cast<TModule*>(m);
			app::ModuleWidget* mw = new TModuleWidget(tm);
			mw->setModel(this);
			return mw;
		}
	};
	plugin::Model* o = new TModel;
	o->slug = slug;
	plugin::models().push_back(o);
	return o;
}

/// Centres a widget made at `pos` there. One whose size isn't known (a
/// widget of the plugin's own drawn from pictures that aren't here) takes
/// 8 mm a side first, so it stays where its panel code put it rather than
/// going off to infinity.
inline void centred(widget::Widget* o) {
	math::Vec s = o->box.size;
	if (!(std::isfinite(s.x) && std::isfinite(s.y) && s.x > 0.f && s.y > 0.f))
		o->box.size = mm2px(math::Vec(8.0, 8.0));
	o->box.pos = o->box.pos.minus(o->box.size.div(2));
}

template <class TWidget>
TWidget* createWidget(math::Vec pos) {
	TWidget* o = new TWidget;
	o->box.pos = pos;
	return o;
}
template <class TWidget>
TWidget* createWidgetCentered(math::Vec pos) {
	TWidget* o = createWidget<TWidget>(pos);
	centred(o);
	return o;
}
inline app::SvgPanel* createPanel(std::string svgPath) {
	app::SvgPanel* panel = new app::SvgPanel;
	panel->setBackground(window::Svg::load(svgPath));
	return panel;
}
template <class TPanel = app::ThemedSvgPanel>
TPanel* createPanel(std::string lightSvgPath, std::string darkSvgPath) {
	TPanel* panel = new TPanel;
	panel->setBackground(window::Svg::load(lightSvgPath), window::Svg::load(darkSvgPath));
	return panel;
}
template <class TParamWidget>
TParamWidget* createParam(math::Vec pos, engine::Module* module, int paramId) {
	TParamWidget* o = new TParamWidget;
	o->box.pos = pos;
	o->app::ParamWidget::module = module;
	o->app::ParamWidget::paramId = paramId;
	o->initParamQuantity();
	return o;
}
template <class TParamWidget>
TParamWidget* createParamCentered(math::Vec pos, engine::Module* module, int paramId) {
	TParamWidget* o = createParam<TParamWidget>(pos, module, paramId);
	centred(o);
	return o;
}
template <class TPortWidget>
TPortWidget* createInput(math::Vec pos, engine::Module* module, int inputId) {
	TPortWidget* o = new TPortWidget;
	o->box.pos = pos;
	o->app::PortWidget::module = module;
	o->app::PortWidget::type = engine::Port::INPUT;
	o->app::PortWidget::portId = inputId;
	return o;
}
template <class TPortWidget>
TPortWidget* createInputCentered(math::Vec pos, engine::Module* module, int inputId) {
	TPortWidget* o = createInput<TPortWidget>(pos, module, inputId);
	centred(o);
	return o;
}
template <class TPortWidget>
TPortWidget* createOutput(math::Vec pos, engine::Module* module, int outputId) {
	TPortWidget* o = new TPortWidget;
	o->box.pos = pos;
	o->app::PortWidget::module = module;
	o->app::PortWidget::type = engine::Port::OUTPUT;
	o->app::PortWidget::portId = outputId;
	return o;
}
template <class TPortWidget>
TPortWidget* createOutputCentered(math::Vec pos, engine::Module* module, int outputId) {
	TPortWidget* o = createOutput<TPortWidget>(pos, module, outputId);
	centred(o);
	return o;
}
template <class TModuleLightWidget>
TModuleLightWidget* createLight(math::Vec pos, engine::Module* module, int firstLightId) {
	TModuleLightWidget* o = new TModuleLightWidget;
	o->box.pos = pos;
	o->app::ModuleLightWidget::module = module;
	o->app::ModuleLightWidget::firstLightId = firstLightId;
	return o;
}
template <class TModuleLightWidget>
TModuleLightWidget* createLightCentered(math::Vec pos, engine::Module* module, int firstLightId) {
	TModuleLightWidget* o = createLight<TModuleLightWidget>(pos, module, firstLightId);
	centred(o);
	return o;
}
template <class TParamWidget>
TParamWidget* createLightParam(math::Vec pos, engine::Module* module, int paramId, int firstLightId) {
	TParamWidget* o = createParam<TParamWidget>(pos, module, paramId);
	o->getLight()->module = module;
	o->getLight()->firstLightId = firstLightId;
	return o;
}
template <class TParamWidget>
TParamWidget* createLightParamCentered(math::Vec pos, engine::Module* module, int paramId, int firstLightId) {
	TParamWidget* o = createLightParam<TParamWidget>(pos, module, paramId, firstLightId);
	centred(o);
	return o;
}

template <class TMenu = ui::Menu>
TMenu* createMenu() {
	// (no window to show it in: whoever made it has it, and nothing frees it)
	return new TMenu;
}
template <class TMenuLabel = ui::MenuLabel>
TMenuLabel* createMenuLabel(std::string text) {
	TMenuLabel* o = new TMenuLabel;
	o->text = text;
	return o;
}
template <class TMenuItem = ui::MenuItem>
TMenuItem* createMenuItem(std::string text, std::string rightText = "") {
	TMenuItem* o = new TMenuItem;
	o->text = text;
	o->rightText = rightText;
	return o;
}
/// An item that does `action` when it's chosen.
template <class TMenuItem = ui::MenuItem>
TMenuItem* createMenuItem(std::string text, std::string rightText, std::function<void()> action, bool disabled = false,
	bool alwaysConsume = false) {
	struct Item : TMenuItem {
		std::function<void()> action;
		void onAction(const widget::Widget::ActionEvent& e) override {
			if (action)
				action();
		}
	};
	Item* item = createMenuItem<Item>(text, rightText);
	item->action = action;
	item->disabled = disabled;
	return item;
}
/// An item with a check mark while `checked()` says so.
template <class TMenuItem = ui::MenuItem>
TMenuItem* createCheckMenuItem(std::string text, std::string rightText, std::function<bool()> checked, std::function<void()> action,
	bool disabled = false, bool alwaysConsume = false) {
	struct Item : TMenuItem {
		std::string rightTextPrefix;
		std::function<bool()> checked;
		std::function<void()> action;
		void step() override {
			this->rightText = rightTextPrefix;
			if (checked && checked()) {
				if (!rightTextPrefix.empty())
					this->rightText += "  ";
				this->rightText += CHECKMARK_STRING;
			}
			TMenuItem::step();
		}
		void onAction(const widget::Widget::ActionEvent& e) override {
			if (action)
				action();
		}
	};
	Item* item = createMenuItem<Item>(text);
	item->rightTextPrefix = rightText;
	item->checked = checked;
	item->action = action;
	item->disabled = disabled;
	return item;
}
/// An item that turns something on and off.
template <class TMenuItem = ui::MenuItem>
TMenuItem* createBoolMenuItem(std::string text, std::string rightText, std::function<bool()> getter, std::function<void(bool state)> setter,
	bool disabled = false, bool alwaysConsume = false) {
	struct Item : TMenuItem {
		std::string rightTextPrefix;
		std::function<bool()> getter;
		std::function<void(bool state)> setter;
		void step() override {
			this->rightText = rightTextPrefix;
			if (getter && getter()) {
				if (!rightTextPrefix.empty())
					this->rightText += "  ";
				this->rightText += CHECKMARK_STRING;
			}
			TMenuItem::step();
		}
		void onAction(const widget::Widget::ActionEvent& e) override {
			if (setter)
				setter(!(getter && getter()));
		}
	};
	Item* item = createMenuItem<Item>(text);
	item->rightTextPrefix = rightText;
	item->getter = getter;
	item->setter = setter;
	item->disabled = disabled;
	return item;
}
template <typename T>
ui::MenuItem* createBoolPtrMenuItem(std::string text, std::string rightText, T* ptr) {
	return createBoolMenuItem(
		text, rightText, [=]() { return ptr ? (bool) *ptr : false; },
		[=](bool val) {
			if (ptr)
				*ptr = val;
		});
}
/// An item that opens a menu, which `createMenu` fills.
template <class TMenuItem = ui::MenuItem>
TMenuItem* createSubmenuItem(std::string text, std::string rightText, std::function<void(ui::Menu* menu)> createMenu, bool disabled = false) {
	struct Item : TMenuItem {
		std::function<void(ui::Menu* menu)> createMenu;
		ui::Menu* createChildMenu() override {
			ui::Menu* menu = new ui::Menu;
			if (createMenu)
				createMenu(menu);
			return menu;
		}
	};
	Item* item = createMenuItem<Item>(text, rightText + (rightText.empty() ? "" : "  ") + RIGHT_ARROW);
	item->createMenu = createMenu;
	item->disabled = disabled;
	return item;
}
/// An item that opens a choice of `labels`: the one `getter()` names is
/// checked, and choosing one calls `setter`.
template <class TMenuItem = ui::MenuItem>
TMenuItem* createIndexSubmenuItem(std::string text, std::vector<std::string> labels, std::function<size_t()> getter,
	std::function<void(size_t val)> setter, bool disabled = false, bool alwaysConsume = false) {
	struct IndexItem : ui::MenuItem {
		std::function<size_t()> getter;
		std::function<void(size_t)> setter;
		size_t index = 0;
		void step() override {
			size_t current = getter ? getter() : 0;
			this->rightText = CHECKMARK(current == index);
			ui::MenuItem::step();
		}
		void onAction(const widget::Widget::ActionEvent& e) override {
			if (setter)
				setter(index);
		}
	};
	struct Item : TMenuItem {
		std::function<size_t()> getter;
		std::function<void(size_t)> setter;
		std::vector<std::string> labels;
		void step() override {
			size_t current = getter ? getter() : 0;
			std::string label = (current < labels.size()) ? labels[current] : "";
			this->rightText = label + "  " + RIGHT_ARROW;
			TMenuItem::step();
		}
		ui::Menu* createChildMenu() override {
			ui::Menu* menu = new ui::Menu;
			for (size_t i = 0; i < labels.size(); i++) {
				IndexItem* item = createMenuItem<IndexItem>(labels[i]);
				item->getter = getter;
				item->setter = setter;
				item->index = i;
				menu->addChild(item);
			}
			return menu;
		}
	};
	Item* item = createMenuItem<Item>(text);
	item->getter = getter;
	item->setter = setter;
	item->labels = labels;
	item->disabled = disabled;
	return item;
}
template <typename T>
ui::MenuItem* createIndexPtrSubmenuItem(std::string text, std::vector<std::string> labels, T* ptr) {
	return createIndexSubmenuItem(
		text, labels, [=]() { return ptr ? (size_t) *ptr : (size_t) 0; },
		[=](size_t index) {
			if (ptr)
				*ptr = T(index);
		});
}

/// A MIDI port's menu: the plugin's MIDI is its one driver and device, so
/// what's chosen is the channel (an input: all, or one of 16).
inline void app::appendMidiMenu(ui::Menu* menu, midi::Port* port) {
	if (!menu || !port)
		return;
	std::vector<int> channels = port->getChannels();
	std::vector<std::string> labels;
	for (int c : channels)
		labels.push_back(port->getChannelName(c));
	menu->addChild(createIndexSubmenuItem(
		"MIDI channel", labels,
		[=]() {
			for (size_t i = 0; i < channels.size(); i++)
				if (channels[i] == port->getChannel())
					return i;
			return (size_t) 0;
		},
		[=](size_t i) {
			if (i < channels.size())
				port->setChannel(channels[i]);
		}));
}

} // namespace rack
