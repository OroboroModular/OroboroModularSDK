// SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-Oroboro-Rack-Bridge-Exception (rack/LICENSE-EXCEPTION.md)
// What the SDK's `rack.hpp` declares and doesn't define in place: strings,
// logging, random numbers, files, a module's bookkeeping, the FFT, the
// minBLEP's impulse. The SDK's own code (docs/rack-modules.md).

#include <cstring>
#include <filesystem>

#include <rack.hpp>

namespace fs = std::filesystem;

/* Where the plugin's own files are (its source folder): the library's entry
 * file says. */
extern "C" const char* oroboro_rack_plugin_dir;
/* The sizes of the plugin's SVG files, as `oromod rack build` found them
 * where the plugin's folder was: for where it isn't. Paths from the plugin's
 * folder ("res/Panel.svg"); the list ends with a null path. */
extern "C" {
struct OroboroRackSvg {
	const char* path;
	float width, height;
};
extern const OroboroRackSvg oroboro_rack_svgs[];
/* The plugin's SVG files themselves, as the build packed them: what
 * `window::svgDraw` draws, by their number here (oroboro_module_picture).
 * The list ends with a null path. */
struct OroboroRackPicture {
	const char* path;
	const unsigned char* bytes;
	unsigned int size;
};
extern const OroboroRackPicture oroboro_rack_pictures[];
}

namespace rack {

Exception::Exception(const char* format, ...) {
	va_list args;
	va_start(args, format);
	msg = string::fV(format, args);
	va_end(args);
}

// ---- logging -------------------------------------------------------------------------------
namespace logger {
void log(Level level, const char* filename, int line, const char* func, const char* format, ...) {
	static const bool on = std::getenv("OROBORO_RACK_LOG") != nullptr;
	if (!on)
		return;
	static const char* const names[] = {"debug", "info", "warn", "fatal"};
	va_list args;
	va_start(args, format);
	std::string text = string::fV(format, args);
	va_end(args);
	std::fprintf(stderr, "[%s %s:%d %s] %s\n", names[level], filename, line, func, text.c_str());
}
} // namespace logger

// ---- string --------------------------------------------------------------------------------
namespace string {
std::string fV(const char* format, va_list args) {
	va_list copy;
	va_copy(copy, args);
	int size = std::vsnprintf(nullptr, 0, format, copy);
	va_end(copy);
	if (size < 0)
		return "";
	std::string s(size, '\0');
	std::vsnprintf(&s[0], size + 1, format, args);
	return s;
}
std::string f(const char* format, ...) {
	va_list args;
	va_start(args, format);
	std::string s = fV(format, args);
	va_end(args);
	return s;
}
} // namespace string

// ---- random --------------------------------------------------------------------------------
namespace random {
Xoroshiro128Plus& local() {
	static thread_local Xoroshiro128Plus rng;
	if (!rng.isSeeded()) {
		// each thread its own, and no two runs alike
		static std::atomic<uint64_t> count{0};
		uint64_t now = (uint64_t) std::chrono::high_resolution_clock::now().time_since_epoch().count();
		uint64_t n = ++count;
		rng.seed(now ^ (n * 0x9e3779b97f4a7c15ull), (now << 21) ^ (n * 0xbf58476d1ce4e5b9ull) ^ (uint64_t) (uintptr_t) &rng);
		for (int i = 0; i < 16; i++)
			rng();
	}
	return rng;
}
} // namespace random

// ---- system --------------------------------------------------------------------------------
namespace system {

std::string joinTwo(const std::string& path1, const std::string& path2) {
	if (path1.empty())
		return path2;
	return (fs::u8path(path1) / fs::u8path(path2)).generic_u8string();
}

static void appendEntries(std::vector<std::string>& entries, const fs::path& dir, int depth) {
	std::error_code ec;
	for (const auto& entry : fs::directory_iterator(dir, ec)) {
		entries.push_back(entry.path().generic_u8string());
		if (depth != 0 && entry.is_directory(ec))
			appendEntries(entries, entry.path(), depth - 1);
	}
}
std::vector<std::string> getEntries(const std::string& dirPath, int depth) {
	std::vector<std::string> entries;
	appendEntries(entries, fs::u8path(dirPath), depth);
	std::sort(entries.begin(), entries.end());
	return entries;
}
bool exists(const std::string& path) {
	std::error_code ec;
	return fs::exists(fs::u8path(path), ec);
}
bool isFile(const std::string& path) {
	std::error_code ec;
	return fs::is_regular_file(fs::u8path(path), ec);
}
bool isDirectory(const std::string& path) {
	std::error_code ec;
	return fs::is_directory(fs::u8path(path), ec);
}
uint64_t getFileSize(const std::string& path) {
	std::error_code ec;
	auto size = fs::file_size(fs::u8path(path), ec);
	return ec ? 0 : (uint64_t) size;
}
bool rename(const std::string& srcPath, const std::string& destPath) {
	std::error_code ec;
	fs::rename(fs::u8path(srcPath), fs::u8path(destPath), ec);
	return !ec;
}
bool copy(const std::string& srcPath, const std::string& destPath) {
	std::error_code ec;
	fs::copy(fs::u8path(srcPath), fs::u8path(destPath), fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
	return !ec;
}
bool createDirectory(const std::string& path) {
	std::error_code ec;
	return fs::create_directory(fs::u8path(path), ec);
}
bool createDirectories(const std::string& path) {
	std::error_code ec;
	return fs::create_directories(fs::u8path(path), ec);
}
bool createSymbolicLink(const std::string& target, const std::string& link) {
	std::error_code ec;
	fs::create_symlink(fs::u8path(target), fs::u8path(link), ec);
	return !ec;
}
bool remove(const std::string& path) {
	std::error_code ec;
	return fs::remove(fs::u8path(path), ec);
}
int removeRecursively(const std::string& path) {
	std::error_code ec;
	return (int) fs::remove_all(fs::u8path(path), ec);
}
std::string getWorkingDirectory() {
	std::error_code ec;
	return fs::current_path(ec).generic_u8string();
}
void setWorkingDirectory(const std::string& path) {
	// (a library doesn't move its host's working directory)
}
std::string getTempDirectory() {
	std::error_code ec;
	return fs::temp_directory_path(ec).generic_u8string();
}
std::string getAbsolute(const std::string& path) {
	std::error_code ec;
	return fs::absolute(fs::u8path(path), ec).generic_u8string();
}
std::string getCanonical(const std::string& path) {
	std::error_code ec;
	return fs::weakly_canonical(fs::u8path(path), ec).generic_u8string();
}
std::string getDirectory(const std::string& path) { return fs::u8path(path).parent_path().generic_u8string(); }
std::string getFilename(const std::string& path) { return fs::u8path(path).filename().generic_u8string(); }
std::string getStem(const std::string& path) { return fs::u8path(path).stem().generic_u8string(); }
std::string getExtension(const std::string& path) { return fs::u8path(path).extension().generic_u8string(); }

std::vector<uint8_t> readFile(const std::string& path) {
	std::vector<uint8_t> data;
	FILE* f = std::fopen(path.c_str(), "rb");
	if (!f)
		throw Exception("Cannot read file %s", path.c_str());
	std::fseek(f, 0, SEEK_END);
	long len = std::ftell(f);
	std::fseek(f, 0, SEEK_SET);
	if (len > 0) {
		data.resize(len);
		size_t got = std::fread(data.data(), 1, len, f);
		data.resize(got);
	}
	std::fclose(f);
	return data;
}
uint8_t* readFile(const std::string& path, size_t* size) {
	std::vector<uint8_t> data = readFile(path);
	uint8_t* out = (uint8_t*) std::malloc(data.size() ? data.size() : 1);
	std::memcpy(out, data.data(), data.size());
	if (size)
		*size = data.size();
	return out;
}
bool writeFile(const std::string& path, const std::vector<uint8_t>& data) {
	FILE* f = std::fopen(path.c_str(), "wb");
	if (!f)
		return false;
	size_t put = std::fwrite(data.data(), 1, data.size(), f);
	std::fclose(f);
	return put == data.size();
}

double getTime() {
	static const auto start = std::chrono::steady_clock::now();
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}
double getUnixTime() { return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); }
double getThreadTime() { return (double) std::clock() / CLOCKS_PER_SEC; }
void sleep(double time) {
	if (time > 0)
		std::this_thread::sleep_for(std::chrono::duration<double>(time));
}
int getLogicalCoreCount() { return (int) std::max(1u, std::thread::hardware_concurrency()); }
std::string getOperatingSystemInfo() { return "Oroboro Modular"; }
void openBrowser(const std::string& url) {}
void openDirectory(const std::string& path) {}
void runProcessDetached(const std::string& path) {}
void setThreadName(const std::string& name) {}
} // namespace system

// ---- asset ---------------------------------------------------------------------------------
namespace asset {
std::string systemDir;
std::string userDir;

static std::string home() {
	const char* dir = std::getenv("USERPROFILE");
	if (!dir || !*dir)
		dir = std::getenv("HOME");
	return dir ? dir : ".";
}
std::string system(std::string filename) {
	// there is no Rack here: its own files are nowhere
	std::string dir = systemDir.empty() ? system::join(system::getTempDirectory(), "oroboro-rack-none") : systemDir;
	return system::join(dir, filename);
}
std::string user(std::string filename) {
	std::string dir = userDir.empty() ? system::join(home(), "Documents", "Oroboro Modular", "Rack") : userDir;
	return system::join(dir, filename);
}
std::string plugin(plugin::Plugin* plugin, std::string filename) {
	std::string dir = (plugin && !plugin->path.empty()) ? plugin->path : std::string(oroboro_rack_plugin_dir ? oroboro_rack_plugin_dir : ".");
	return system::join(dir, filename);
}
} // namespace asset

// ---- window: SVG files' sizes ----------------------------------------------------------------
namespace window {

namespace {
/// A length of an SVG's root in pixels at `dpi` (Rack's: 75 to the inch); 0
/// if it isn't one.
float svgLength(const std::string& text, double dpi = 75.0) {
	char* end = nullptr;
	double x = std::strtod(text.c_str(), &end);
	if (end == text.c_str())
		return 0.f;
	std::string unit = string::trim(end);
	if (unit == "mm")
		x *= dpi / 25.4;
	else if (unit == "cm")
		x *= dpi / 2.54;
	else if (unit == "in")
		x *= dpi;
	else if (unit == "pt")
		x *= dpi / 72.0;
	else if (unit == "pc")
		x *= dpi / 6.0;
	else if (unit == "%")
		return 0.f;
	return (float) x;
}

/// The value of attribute `name` in a tag's text.
std::string svgAttribute(const std::string& tag, const std::string& name) {
	size_t at = 0;
	while ((at = tag.find(name, at)) != std::string::npos) {
		size_t after = at + name.size();
		// the whole name, not the end of another ("stroke-width")
		bool starts = at == 0 || std::isspace((unsigned char) tag[at - 1]);
		size_t eq = tag.find_first_not_of(" \t\r\n", after);
		if (starts && eq != std::string::npos && tag[eq] == '=') {
			size_t quote = tag.find_first_of("\"'", eq);
			if (quote == std::string::npos)
				return "";
			size_t close = tag.find(tag[quote], quote + 1);
			if (close == std::string::npos)
				return "";
			return tag.substr(quote + 1, close - quote - 1);
		}
		at = after;
	}
	return "";
}

/// An SVG file's size from its root element: its width and height, else
/// its viewBox's.
math::Vec svgSize(const std::string& text, double dpi = 75.0) {
	size_t open = text.find("<svg");
	if (open == std::string::npos)
		return math::Vec();
	size_t close = text.find('>', open);
	std::string tag = text.substr(open, close == std::string::npos ? std::string::npos : close - open);
	float w = svgLength(svgAttribute(tag, "width"), dpi);
	float h = svgLength(svgAttribute(tag, "height"), dpi);
	float box[4] = {0.f, 0.f, 0.f, 0.f};
	std::string view = svgAttribute(tag, "viewBox");
	for (char& c : view)
		if (c == ',')
			c = ' ';
	bool viewed = std::sscanf(view.c_str(), "%f %f %f %f", &box[0], &box[1], &box[2], &box[3]) == 4 && box[2] > 0.f && box[3] > 0.f;
	if (w > 0.f && h > 0.f)
		return math::Vec(w, h);
	if (!viewed)
		return math::Vec(w, h);
	if (w > 0.f)
		return math::Vec(w, w * box[3] / box[2]);
	if (h > 0.f)
		return math::Vec(h * box[2] / box[3], h);
	return math::Vec(box[2], box[3]);
}

/// A path as the list has it: from the plugin's folder, with forward slashes.
std::string fromPluginDir(const std::string& filename) {
	std::string path = filename;
	for (char& c : path)
		if (c == '\\')
			c = '/';
	std::string dir = oroboro_rack_plugin_dir ? oroboro_rack_plugin_dir : "";
	for (char& c : dir)
		if (c == '\\')
			c = '/';
	while (!dir.empty() && dir.back() == '/')
		dir.pop_back();
	if (!dir.empty() && path.size() > dir.size() + 1 && path.compare(0, dir.size(), dir) == 0 && path[dir.size()] == '/')
		return path.substr(dir.size() + 1);
	return path;
}
} // namespace

/// The sizes (mm) of Rack's stock components, as this interface makes
/// them: a widget of a plugin's own that loads one's picture (as
/// `res/ComponentLibrary/CKSSThreeHorizontal_0.svg`, from Rack's own
/// folder, which isn't here) takes its size from its name here.
struct StockSize {
	const char* name;
	float width, height;
};
static const StockSize STOCK[] = {
	{"CKSS", 4.7f, 7.0f}, {"CKSSThree", 4.7f, 9.5f}, {"CKSSThreeHorizontal", 9.5f, 4.7f}, {"CKD6", 9.5f, 9.5f},
	{"TL1105", 5.5f, 5.5f}, {"VCVButton", 6.0f, 6.0f}, {"VCVBezel", 8.0f, 8.0f}, {"PB61303", 13.0f, 13.0f},
	{"BefacoSwitch", 9.0f, 13.0f}, {"BefacoPush", 9.5f, 9.5f}, {"NKK", 9.0f, 13.0f},
	{"RoundBlackKnob", 10.0f, 10.0f}, {"RoundSmallBlackKnob", 8.0f, 8.0f}, {"RoundLargeBlackKnob", 12.7f, 12.7f},
	{"RoundBigBlackKnob", 15.2f, 15.2f}, {"RoundHugeBlackKnob", 19.0f, 19.0f}, {"Trimpot", 6.3f, 6.3f},
	{"Davies1900hWhite", 12.0f, 12.0f}, {"Davies1900hBlack", 12.0f, 12.0f}, {"Davies1900hRed", 12.0f, 12.0f},
	{"Davies1900hLargeWhite", 18.0f, 18.0f}, {"Davies1900hLargeBlack", 18.0f, 18.0f}, {"Davies1900hLargeRed", 18.0f, 18.0f},
	{"BefacoBigKnob", 25.0f, 25.0f}, {"BefacoTinyKnob", 10.0f, 10.0f}, {"SynthTechAlco", 15.0f, 15.0f},
	{"PJ301M", 8.2f, 8.2f}, {"PJ3410", 10.8f, 10.8f}, {"CL1362", 11.0f, 11.0f},
	{"VCVSlider", 7.0f, 30.0f}, {"VCVSliderHorizontal", 30.0f, 7.0f}, {"BefacoSlidePot", 4.5f, 41.0f},
};

/// The size of the stock component whose picture `path` is (its file's
/// name, without a frame's number or a part's ending: `CKSS_1`, `Trimpot_bg`).
static math::Vec stockSize(const std::string& path) {
	if (path.find("ComponentLibrary/") == std::string::npos)
		return math::Vec();
	std::string name = path.substr(path.find_last_of('/') + 1);
	if (name.size() > 4 && name.compare(name.size() - 4, 4, ".svg") == 0)
		name.resize(name.size() - 4);
	for (const char* part : {"_bg", "_fg", "-bg", "-fg", "_dark"})
		if (name.size() > std::strlen(part) && name.compare(name.size() - std::strlen(part), std::strlen(part), part) == 0)
			name.resize(name.size() - std::strlen(part));
	size_t digits = name.find_last_not_of("0123456789");
	if (digits != std::string::npos && digits + 1 < name.size() && name[digits] == '_')
		name.resize(digits);
	for (const StockSize& s : STOCK)
		if (name == s.name)
			return mm2px(math::Vec(s.width, s.height));
	return math::Vec();
}

void Svg::loadFile(const std::string& filename) {
	path = fromPluginDir(filename);
	size = math::Vec();
	// the file, where the plugin's folder is (the computer it was built on)
	FILE* f = std::fopen(filename.c_str(), "rb");
	if (f) {
		// (its root element is at its start)
		std::string head(16384, '\0');
		size_t n = std::fread(&head[0], 1, head.size(), f);
		std::fclose(f);
		head.resize(n);
		size = svgSize(head);
	}
	// else what the build wrote down
	for (const OroboroRackSvg* s = oroboro_rack_svgs; s && s->path && !(size.x > 0.f && size.y > 0.f); s++) {
		if (path == s->path)
			size = math::Vec(s->width, s->height);
	}
	// else one of Rack's own components': as big as this interface makes it
	if (!(size.x > 0.f && size.y > 0.f))
		size = stockSize(path);
	image.width = size.x;
	image.height = size.y;
	// (there even where its size isn't known: panel code reads it unasked)
	handle = &image;
}

/// The loaded SVG of `svg`, and its number among the pictures the build
/// packed (-1: not among them, so it can't be drawn).
static int pictureOf(NSVGimage* svg, std::shared_ptr<Svg>* found = nullptr) {
	if (!svg)
		return -1;
	for (const std::shared_ptr<Svg>& s : Svg::loaded()) {
		if (s->handle != svg)
			continue;
		int i = 0;
		for (const OroboroRackPicture* p = oroboro_rack_pictures; p && p->path; p++, i++) {
			if (s->path == p->path) {
				if (found)
					*found = s;
				return i;
			}
		}
		return -1;
	}
	return -1;
}

void svgDraw(NVGcontext* vg, NSVGimage* svg) {
	std::shared_ptr<Svg> s;
	int i = pictureOf(svg, &s);
	if (vg && i >= 0)
		oroboro_nvg_picture(vg, i, s->size.x, s->size.y);
}

std::vector<std::shared_ptr<Svg>>& Svg::loaded() {
	static std::vector<std::shared_ptr<Svg>>* all = new std::vector<std::shared_ptr<Svg>>;
	return *all;
}

std::shared_ptr<Svg> Svg::load(const std::string& filename) {
	std::string path = fromPluginDir(filename);
	for (const std::shared_ptr<Svg>& svg : loaded())
		if (svg->path == path)
			return svg;
	std::shared_ptr<Svg> svg = std::make_shared<Svg>();
	svg->loadFile(filename);
	loaded().push_back(svg);
	return svg;
}

} // namespace window

// ---- nanosvg: a picture's size ------------------------------------------------------------
extern "C" {
NSVGimage* nsvgParse(char* input, const char* units, float dpi) {
	if (!input)
		return nullptr;
	math::Vec px = window::svgSize(input, dpi);
	// (from pixels at `dpi` to what's asked)
	std::string unit = units ? units : "px";
	float per = 1.f;
	if (unit == "pt")
		per = 72.f / dpi;
	else if (unit == "pc")
		per = 6.f / dpi;
	else if (unit == "mm")
		per = 25.4f / dpi;
	else if (unit == "cm")
		per = 2.54f / dpi;
	else if (unit == "in")
		per = 1.f / dpi;
	NSVGimage* image = new NSVGimage;
	image->width = px.x * per;
	image->height = px.y * per;
	return image;
}
NSVGimage* nsvgParseFromFile(const char* filename, const char* units, float dpi) {
	FILE* f = filename ? std::fopen(filename, "rb") : nullptr;
	if (!f)
		return nullptr;
	std::string text;
	char buffer[4096];
	size_t n;
	while ((n = std::fread(buffer, 1, sizeof(buffer), f)) > 0)
		text.append(buffer, n);
	std::fclose(f);
	return nsvgParse(&text[0], units, dpi);
}
NSVGpath* nsvgDuplicatePath(NSVGpath* p) {
	if (!p)
		return nullptr;
	NSVGpath* copy = (NSVGpath*) std::calloc(1, sizeof(NSVGpath));
	*copy = *p;
	copy->next = nullptr;
	copy->pts = (float*) std::malloc(sizeof(float) * 2 * std::max(p->npts, 1));
	if (p->pts && p->npts > 0)
		std::memcpy(copy->pts, p->pts, sizeof(float) * 2 * p->npts);
	return copy;
}
void nsvgDelete(NSVGimage* image) { delete image; }
}

namespace app {

void ParamWidget::draw(const DrawArgs& args) {
	if (oroboroOwn()) {
		// its picture as it is now (a switch held: its next frame), where
		// the build packed it; else a plain round button
		std::shared_ptr<window::Svg> frame;
		if (SvgSwitch* sw = dynamic_cast<SvgSwitch*>(this))
			if (!sw->frames.empty())
				frame = sw->frames[std::min(sw->frames.size() - 1, (size_t) (oroboroHeld ? 1 : 0))];
		std::shared_ptr<window::Svg> packed;
		int picture = frame ? window::pictureOf(frame->handle, &packed) : -1;
		float r = std::min(box.size.x, box.size.y) / 2.f;
		if (picture >= 0)
			oroboro_nvg_picture(args.vg, picture, packed->size.x, packed->size.y);
		else if (r > 1.f) {
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, box.size.x / 2.f, box.size.y / 2.f, r - 0.75f);
			nvgFillColor(args.vg, oroboroHeld ? nvgRGB(0x6a, 0x6a, 0x6a) : nvgRGB(0x2c, 0x2c, 0x2c));
			nvgFill(args.vg);
			nvgStrokeWidth(args.vg, 1.5f);
			nvgStrokeColor(args.vg, nvgRGB(0xb4, 0xb4, 0xb4));
			nvgStroke(args.vg);
		}
	}
	Widget::draw(args);
}

} // namespace app

// ---- MIDI: the plugin's ---------------------------------------------------------------------
namespace midi {
Driver* oroboroDriver() {
	static OroboroDriver driver;
	return &driver;
}
InputDevice*& oroboroMaking() {
	// (per thread: the plugin may make instances on more than one)
	static thread_local InputDevice* making = nullptr;
	return making;
}
} // namespace midi

// ---- plugin --------------------------------------------------------------------------------
namespace plugin {
std::vector<Model*>& models() {
	// (made on first use: models register while the library's statics are still being made)
	static std::vector<Model*>* list = new std::vector<Model*>;
	return *list;
}
Plugin* instance() {
	static Plugin* plugin = [] {
		Plugin* p = new Plugin;
		p->path = oroboro_rack_plugin_dir ? oroboro_rack_plugin_dir : "";
		return p;
	}();
	return plugin;
}
std::vector<Plugin*> plugins = {instance()};
Plugin* getPlugin(const std::string& pluginSlug) {
	Plugin* p = instance();
	return p->slug == pluginSlug ? p : nullptr;
}
Model* getModel(const std::string& pluginSlug, const std::string& modelSlug) {
	if (!getPlugin(pluginSlug))
		return nullptr;
	for (Model* m : models())
		if (m->slug == modelSlug)
			return m;
	return nullptr;
}
Model* modelFromJson(json_t* moduleJ) {
	const char* pluginSlug = json_string_value(json_object_get(moduleJ, "plugin"));
	const char* modelSlug = json_string_value(json_object_get(moduleJ, "model"));
	if (!pluginSlug || !modelSlug)
		return nullptr;
	return getModel(normalizeSlug(pluginSlug), normalizeSlug(modelSlug));
}
static bool slugChar(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
}
bool isSlugValid(const std::string& slug) {
	for (char c : slug)
		if (!slugChar(c))
			return false;
	return true;
}
std::string normalizeSlug(const std::string& slug) {
	std::string s;
	for (char c : slug)
		if (slugChar(c))
			s += c;
	return s;
}
std::string Model::getManualUrl() { return manualUrl.empty() && plugin ? plugin->manualUrl : manualUrl; }
std::string Model::getFullName() { return (plugin ? plugin->getBrand() + " " : std::string()) + name; }
std::string Model::getFactoryPresetDirectory() { return asset::plugin(plugin, system::join("presets", slug)); }
std::string Model::getUserPresetDirectory() { return asset::user(system::join("presets", plugin ? plugin->slug : "", slug)); }
} // namespace plugin

// ---- the context ---------------------------------------------------------------------------
Context* contextGet() {
	static Context* context = [] {
		Context* c = new Context;
		c->event = new event::State;
		c->scene = new app::Scene;
		c->scene->rackScroll = new app::RackScrollWidget;
		c->scene->rack = new app::RackWidget;
		c->scene->rackScroll->rackWidget = c->scene->rack;
		c->engine = new engine::Engine;
		c->window = new window::Window;
		// (for panel code that measures a text outside its drawing)
		c->window->vg = oroboro_nvg_new();
		c->history = new history::State;
		c->patch = new patch::Manager;
		return c;
	}();
	return context;
}

namespace settings {
bool preferDarkPanels = false;
float rackBrightness = 1.f;
float haloBrightness = 0.25f;
float knobScrollSensitivity = 0.001f;
bool tooltips = true;
bool cpuMeter = false;
float sampleRate = 0.f;
int threadCount = 1;
bool paramTooltip = true;
bool lockModules = false;
float cableOpacity = 0.5f;
float cableTension = 1.f;
bool devMode = false;
bool headless = true;
bool isPlugin = true;
std::string language = "en";
float pixelRatio = 0.f;
std::string uiTheme = "dark";
bool invertZoom = false;
bool mouseWheelZoom = false;
bool allowCursorLock = true;
KnobMode knobMode = KNOB_MODE_LINEAR;
bool knobScroll = false;
float knobLinearSensitivity = 0.001f;
bool squeezeModules = true;
float frameRateLimit = 60.f;
float autosaveInterval = 15.f;
// Rack's default cable colours
std::vector<NVGcolor> cableColors = {
	nvgRGB(0xf3, 0x37, 0x4b), nvgRGB(0xff, 0xb4, 0x37), nvgRGB(0x00, 0xb5, 0x6e),
	nvgRGB(0x36, 0x95, 0xef), nvgRGB(0x8b, 0x4a, 0xde),
};
std::vector<std::string> cableLabels = {"Red", "Yellow", "Green", "Blue", "Purple"};
bool cableAutoRotate = true;
BrowserSort browserSort = BROWSER_SORT_UPDATED;
float browserZoom = -1.f;
} // namespace settings

// ---- engine --------------------------------------------------------------------------------
namespace engine {

Module::~Module() {
	for (ParamQuantity* q : paramQuantities)
		delete q;
	for (PortInfo* info : inputInfos)
		delete info;
	for (PortInfo* info : outputInfos)
		delete info;
	for (LightInfo* info : lightInfos)
		delete info;
}

void Module::config(int numParams, int numInputs, int numOutputs, int numLights) {
	assert(params.empty() && inputs.empty() && outputs.empty() && lights.empty() && paramQuantities.empty());
	params.resize(numParams);
	inputs.resize(numInputs);
	outputs.resize(numOutputs);
	lights.resize(numLights);
	paramQuantities.resize(numParams);
	for (int i = 0; i < numParams; i++)
		configParam(i, 0.f, 1.f, 0.f);
	inputInfos.resize(numInputs);
	for (int i = 0; i < numInputs; i++)
		configInput(i);
	outputInfos.resize(numOutputs);
	for (int i = 0; i < numOutputs; i++)
		configOutput(i);
	lightInfos.resize(numLights);
}

void Module::processBypass(const ProcessArgs& args) {
	for (BypassRoute& route : bypassRoutes) {
		if (route.inputId < 0 || route.outputId < 0)
			continue;
		Input& input = inputs[route.inputId];
		Output& output = outputs[route.outputId];
		output.setVoltage(input.getVoltage());
	}
}

json_t* Module::paramsToJson() {
	json_t* rootJ = json_array();
	for (size_t paramId = 0; paramId < paramQuantities.size(); paramId++) {
		if (!paramQuantities[paramId])
			continue;
		json_t* paramJ = paramQuantities[paramId]->toJson();
		json_object_set_new(paramJ, "id", json_integer(paramId));
		json_array_append_new(rootJ, paramJ);
	}
	return rootJ;
}
void Module::paramsFromJson(json_t* rootJ) {
	size_t i;
	json_t* paramJ;
	json_array_foreach(rootJ, i, paramJ) {
		json_t* idJ = json_object_get(paramJ, "id");
		size_t paramId = idJ ? (size_t) json_integer_value(idJ) : i;
		if (paramId < paramQuantities.size() && paramQuantities[paramId])
			paramQuantities[paramId]->fromJson(paramJ);
	}
}
json_t* Cable::toJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "id", json_integer(id));
	json_object_set_new(rootJ, "outputModuleId", json_integer(outputModule ? outputModule->id : -1));
	json_object_set_new(rootJ, "outputId", json_integer(outputId));
	json_object_set_new(rootJ, "inputModuleId", json_integer(inputModule ? inputModule->id : -1));
	json_object_set_new(rootJ, "inputId", json_integer(inputId));
	return rootJ;
}
json_t* Module::toJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "id", json_integer(id));
	json_object_set_new(rootJ, "params", paramsToJson());
	if (json_t* dataJ = dataToJson())
		json_object_set_new(rootJ, "data", dataJ);
	return rootJ;
}
void Module::fromJson(json_t* rootJ) {
	if (json_t* paramsJ = json_object_get(rootJ, "params"))
		paramsFromJson(paramsJ);
	if (json_t* dataJ = json_object_get(rootJ, "data"))
		dataFromJson(dataJ);
}
void Module::onReset(const ResetEvent& e) {
	for (ParamQuantity* q : paramQuantities)
		if (q && q->resetEnabled && q->isBounded())
			q->reset();
	onReset();
}
void Module::onRandomize(const RandomizeEvent& e) {
	for (ParamQuantity* q : paramQuantities)
		if (q && q->randomizeEnabled && q->isBounded())
			q->randomize();
	onRandomize();
}
std::string Module::createPatchStorageDirectory() {
	std::string path = getPatchStorageDirectory();
	system::createDirectories(path);
	return path;
}
std::string Module::getPatchStorageDirectory() {
	// a folder of this instance's own, among the musician's files
	return asset::user(system::join("modules", string::f("%p", (void*) this)));
}

void ParamQuantity::setValue(float value) {
	if (Param* param = getParam()) {
		value = math::clampSafe(value, getMinValue(), getMaxValue());
		if (snapEnabled)
			value = std::round(value);
		param->setValue(value);
	}
}
float ParamQuantity::getValue() {
	Param* param = getParam();
	return param ? param->getValue() : 0.f;
}
void ParamQuantity::setImmediateValue(float value) { setValue(value); }
float ParamQuantity::getDisplayValue() {
	float v = getValue();
	if (displayBase == 0.f) {
		// linear
	}
	else if (displayBase < 0.f) {
		v = std::log(v) / std::log(-displayBase);
	}
	else {
		v = std::pow(displayBase, v);
	}
	return v * displayMultiplier + displayOffset;
}
void ParamQuantity::setDisplayValue(float displayValue) {
	float v = displayValue - displayOffset;
	if (displayMultiplier == 0.f)
		v = 0.f;
	else
		v /= displayMultiplier;
	if (displayBase == 0.f) {
		// linear
	}
	else if (displayBase < 0.f) {
		v = std::pow(-displayBase, v);
	}
	else {
		v = std::log(v) / std::log(displayBase);
	}
	if (std::isfinite(v))
		setValue(v);
}
std::string ParamQuantity::getDisplayValueString() { return Quantity::getDisplayValueString(); }
void ParamQuantity::setDisplayValueString(std::string s) { Quantity::setDisplayValueString(s); }
void ParamQuantity::reset() { Quantity::reset(); }
void ParamQuantity::randomize() {
	if (!isBounded())
		return;
	if (snapEnabled) {
		float value = math::rescale(random::uniform(), 0.f, 1.f, getMinValue(), getMaxValue() + 1.f);
		setValue(std::floor(value));
	}
	else {
		setScaledValue(random::uniform());
	}
}
json_t* ParamQuantity::toJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "value", json_real(getValue()));
	return rootJ;
}
void ParamQuantity::fromJson(json_t* rootJ) {
	if (json_t* valueJ = json_object_get(rootJ, "value"))
		setValue((float) json_number_value(valueJ));
}
std::string SwitchQuantity::getDisplayValueString() {
	int index = (int) std::floor(getValue() - getMinValue());
	if (!(0 <= index && index < (int) labels.size()))
		return ParamQuantity::getDisplayValueString();
	return labels[index];
}
void SwitchQuantity::setDisplayValueString(std::string s) {
	auto it = std::find(labels.begin(), labels.end(), s);
	if (it != labels.end())
		setValue((float) std::distance(labels.begin(), it) + getMinValue());
	else
		ParamQuantity::setDisplayValueString(s);
}
} // namespace engine

// ---- the FFT -------------------------------------------------------------------------------
namespace dsp {

typedef std::complex<double> Complex;

void* alignedAllocate(size_t bytes) {
	// room in front for the pointer the memory really starts at
	void* raw = std::malloc(bytes + 32 + sizeof(void*));
	if (!raw)
		throw std::bad_alloc();
	uintptr_t start = ((uintptr_t) raw + sizeof(void*) + 31) & ~(uintptr_t) 31;
	((void**) start)[-1] = raw;
	return (void*) start;
}
void alignedRelease(void* p) {
	if (p)
		std::free(((void**) p)[-1]);
}

struct RealFFT::Plan {
	int n;
	bool pow2;
	std::vector<Complex> twiddle; // e^(-2 pi i k / n)
	std::vector<int> reversed;
	std::vector<Complex> work;

	Plan(int n) : n(n), pow2(n > 0 && (n & (n - 1)) == 0), twiddle(n), work(n) {
		for (int k = 0; k < n; k++)
			twiddle[k] = std::polar(1.0, -2.0 * M_PI * k / n);
		if (pow2) {
			reversed.resize(n);
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
	}

	/// `work` to its transform, forward or back (unscaled either way).
	void transform(bool inverse) {
		if (!pow2) {
			// any length: the sum as it stands
			std::vector<Complex> out(n);
			for (int k = 0; k < n; k++) {
				Complex sum = 0;
				for (int t = 0; t < n; t++) {
					Complex w = twiddle[(int) (((int64_t) k * t) % n)];
					sum += work[t] * (inverse ? std::conj(w) : w);
				}
				out[k] = sum;
			}
			work = out;
			return;
		}
		for (int i = 0; i < n; i++)
			if (i < reversed[i])
				std::swap(work[i], work[reversed[i]]);
		for (int len = 2; len <= n; len <<= 1) {
			int step = n / len;
			for (int i = 0; i < n; i += len) {
				for (int j = 0; j < len / 2; j++) {
					Complex w = twiddle[j * step];
					if (inverse)
						w = std::conj(w);
					Complex u = work[i + j];
					Complex v = work[i + j + len / 2] * w;
					work[i + j] = u + v;
					work[i + j + len / 2] = u - v;
				}
			}
		}
	}
};

RealFFT::RealFFT(size_t length) : length((int) length), plan(new Plan((int) length)) {}
RealFFT::~RealFFT() { delete plan; }

void RealFFT::rfft(const float* input, float* output) {
	int n = length;
	for (int i = 0; i < n; i++)
		plan->work[i] = Complex(input[i], 0.0);
	plan->transform(false);
	output[0] = (float) plan->work[0].real();
	if (n > 1)
		output[1] = (float) plan->work[n / 2].real();
	for (int k = 1; k < n / 2; k++) {
		output[2 * k] = (float) plan->work[k].real();
		output[2 * k + 1] = (float) plan->work[k].imag();
	}
}

void RealFFT::irfft(const float* input, float* output) {
	int n = length;
	plan->work[0] = Complex(input[0], 0.0);
	if (n > 1)
		plan->work[n / 2] = Complex(input[1], 0.0);
	for (int k = 1; k < n / 2; k++) {
		plan->work[k] = Complex(input[2 * k], input[2 * k + 1]);
		plan->work[n - k] = std::conj(plan->work[k]);
	}
	plan->transform(true);
	for (int i = 0; i < n; i++)
		output[i] = (float) plan->work[i].real();
}

// ---- the minBLEP's impulse -----------------------------------------------------------------

/// The transform of `x` in place, any power-of-two length.
static void fft(std::vector<Complex>& x, bool inverse) {
	int n = (int) x.size();
	for (int i = 1, j = 0; i < n; i++) {
		int bit = n >> 1;
		for (; j & bit; bit >>= 1)
			j ^= bit;
		j ^= bit;
		if (i < j)
			std::swap(x[i], x[j]);
	}
	for (int len = 2; len <= n; len <<= 1) {
		double angle = 2.0 * M_PI / len * (inverse ? 1 : -1);
		Complex wlen = std::polar(1.0, angle);
		for (int i = 0; i < n; i += len) {
			Complex w = 1;
			for (int j = 0; j < len / 2; j++) {
				Complex u = x[i + j], v = x[i + j + len / 2] * w;
				x[i + j] = u + v;
				x[i + j + len / 2] = u - v;
				w *= wlen;
			}
		}
	}
	if (inverse)
		for (Complex& c : x)
			c /= n;
}

void minBlepImpulse(int z, int o, float* output) {
	// a windowed sinc: a band-limited impulse, symmetric
	int n = 2 * z * o;
	int size = 1;
	while (size < n)
		size <<= 1;
	std::vector<Complex> x(size, 0.0);
	for (int i = 0; i < n; i++) {
		double p = ((double) i / n - 0.5) * 2 * z;
		double s = (p == 0.0) ? 1.0 : std::sin(M_PI * p) / (M_PI * p);
		double w = (double) i / (n - 1);
		double window = 0.35875 - 0.48829 * std::cos(2 * M_PI * w) + 0.14128 * std::cos(4 * M_PI * w) - 0.01168 * std::cos(6 * M_PI * w);
		x[i] = s * window;
	}
	// its minimum-phase version, through the cepstrum: the same spectrum's
	// size, with all its energy as early as can be
	fft(x, false);
	for (Complex& c : x)
		c = std::log(std::max(std::abs(c), 1e-12));
	fft(x, true);
	for (int i = 1; i < size / 2; i++)
		x[i] *= 2.0;
	for (int i = size / 2 + 1; i < size; i++)
		x[i] = 0.0;
	fft(x, false);
	for (Complex& c : x)
		c = std::exp(c);
	fft(x, true);
	// summed, it is the step, from 0 to 1
	double total = 0.0;
	std::vector<double> step(n);
	for (int i = 0; i < n; i++) {
		total += x[i].real();
		step[i] = total;
	}
	double norm = (std::fabs(step[n - 1]) > 1e-12) ? 1.0 / step[n - 1] : 1.0;
	for (int i = 0; i < n; i++)
		output[i] = (float) (step[i] * norm);
}

} // namespace dsp
} // namespace rack

// ---- a colour by hue ----------------------------------------------------------------------
extern "C" NVGcolor nvgHSLA(float h, float s, float l, unsigned char a) {
	h = std::fmod(h, 1.0f);
	if (h < 0.0f)
		h += 1.0f;
	s = rack::math::clamp(s, 0.f, 1.f);
	l = rack::math::clamp(l, 0.f, 1.f);
	float m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
	float m1 = 2 * l - m2;
	auto hue = [](float h, float m1, float m2) {
		if (h < 0)
			h += 1;
		if (h > 1)
			h -= 1;
		if (h < 1.0f / 6.0f)
			return m1 + (m2 - m1) * h * 6.0f;
		if (h < 3.0f / 6.0f)
			return m2;
		if (h < 4.0f / 6.0f)
			return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
		return m1;
	};
	NVGcolor col;
	col.r = rack::math::clamp(hue(h + 1.0f / 3.0f, m1, m2), 0.f, 1.f);
	col.g = rack::math::clamp(hue(h, m1, m2), 0.f, 1.f);
	col.b = rack::math::clamp(hue(h - 1.0f / 3.0f, m1, m2), 0.f, 1.f);
	col.a = a / 255.0f;
	return col;
}
