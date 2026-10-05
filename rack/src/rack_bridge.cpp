// A Rack module as an Oroboro Modular module: the module ABI
// (native/include/oroboro_module.h) over one model of a Rack plugin's
// sources. One library holds one module; which model it is, what it's
// called and where it's listed are the library's entry file's to say
// (`oromod rack build` writes it). The SDK's own code.

#include <rack.hpp>

#include "../../native/include/oroboro_module.h"
#include "../include/osdialog.h"

extern "C" {
/// The model's slug (the name its source gives `createModel`).
extern const char* oroboro_rack_slug;
/// The module's name here: `native/<Vendor>/<Name>`.
extern const char* oroboro_rack_module;
/// Where Add module lists it: osc, filter, env, …
extern const char* oroboro_rack_category;
/// Who makes it (the plugin's brand, as its plugin.json says): Add module
/// lists compiled modules by it. Empty: it doesn't say.
extern const char* oroboro_rack_vendor;
/// The inputs the plugin doesn't show, one name a line (the project's
/// `oroboro-rack.json`, `"hide"`): as the source names one, or as it's
/// named here, case aside. Empty: all show.
extern const char* oroboro_rack_hide;
extern const char* oroboro_rack_signals;
/// Whether the module's panel object may be made (0: its panel code
/// doesn't run here, so it has no menu; `oromod rack build` finds out).
extern int oroboro_rack_panel;
/// The panel's picture, as `oromod rack build` drew it from its SVG (a
/// PNG; none: a null pointer and no bytes).
extern const unsigned char* oroboro_rack_art;
extern unsigned int oroboro_rack_art_size;
/// The SVG files its panel code loads, themselves (`oromod rack build`
/// packs them): what `window::svgDraw` draws, by their number here.
struct OroboroRackPicture {
	const char* path;
	const unsigned char* bytes;
	unsigned int size;
};
extern const OroboroRackPicture oroboro_rack_pictures[];
/// The panel its maker defined in Oroboro Modular's own look, as
/// `oroboro_module_panel` gives it (`oromod rack build` makes it from the
/// module's oroboro/<Module>.json). Empty: the plugin lays the module out
/// itself. Null: none was defined, and the module's Rack panel is its panel.
extern const char* oroboro_rack_ui;
/// Whatever the plugin does when Rack loads it (the entry file's).
void oroboro_rack_init(void);
}

using namespace rack;

namespace {

std::once_flag started;

/// The model this library is, once the plugin is set up.
plugin::Model* model() {
	std::call_once(started, [] {
		try {
			oroboro_rack_init();
		}
		catch (...) {
		}
		plugin::Plugin* plugin = plugin::instance();
		for (plugin::Model* m : plugin::models())
			if (!m->plugin)
				m->plugin = plugin;
	});
	for (plugin::Model* m : plugin::models())
		if (m->slug == oroboro_rack_slug)
			return m;
	return nullptr;
}

std::string jsonText(const std::string& s) {
	std::string out = "\"";
	for (unsigned char c : s) {
		switch (c) {
			case '"': out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			default:
				if (c < 0x20)
					out += string::f("\\u%04x", c);
				else
					out += (char) c;
		}
	}
	return out + "\"";
}

/// The most letters a jack's or a knob's name has here: Rack's names are
/// often whole sentences.
const size_t NAME_LETTERS = 22;

/// A name for a panel from one of Rack's: what comes before a bracket, a
/// colon or a dash, without line breaks, and no longer than a panel reads.
std::string panelName(std::string name) {
	for (char& c : name)
		if (c == '\n' || c == '\r' || c == '\t')
			c = ' ';
	for (const char* cut : {" (", " [", ": ", " - ", " — ", " – "}) {
		size_t at = name.find(cut);
		if (at != std::string::npos && at > 0)
			name = name.substr(0, at);
	}
	name = string::trim(name);
	if (name.size() > NAME_LETTERS) {
		// at a word's end, and never inside a character
		size_t end = NAME_LETTERS;
		while (end > 0 && ((unsigned char) name[end] & 0xc0) == 0x80)
			end--;
		size_t space = name.rfind(' ', end);
		if (space != std::string::npos && space >= NAME_LETTERS / 2)
			end = space;
		name = string::trim(name.substr(0, end));
	}
	return name;
}

/// `name`, or `name 2`, `name 3` … while `taken` has it.
std::string unique(const std::string& name, std::vector<std::string>& taken) {
	std::string candidate = name;
	for (int n = 2; std::find(taken.begin(), taken.end(), candidate) != taken.end(); n++)
		candidate = name + " " + std::to_string(n);
	taken.push_back(candidate);
	return candidate;
}

std::string number(float x) {
	if (!std::isfinite(x))
		return "0";
	return string::f("%.9g", (double) x);
}

/// An input's name without what only says that it's an input.
std::string withoutSuffix(const std::string& name) {
	std::string lower = string::lowercase(name);
	for (const char* suffix : {" cv input", " cv in", " cv", " modulation", " mod", " input", " in"}) {
		if (string::endsWith(lower, suffix) && lower.size() > std::strlen(suffix))
			return string::trim(name.substr(0, name.size() - std::strlen(suffix)));
	}
	return name;
}

/// Whether the input Rack names `full`, named `name` here, is one the
/// plugin doesn't show (`oroboro_rack_hide`).
bool hidden(const std::string& full, const std::string& name) {
	if (!oroboro_rack_hide || !*oroboro_rack_hide)
		return false;
	std::string list = oroboro_rack_hide;
	size_t start = 0;
	while (start <= list.size()) {
		size_t end = list.find('\n', start);
		if (end == std::string::npos)
			end = list.size();
		std::string wanted = string::lowercase(string::trim(list.substr(start, end - start)));
		if (!wanted.empty()
			&& (wanted == string::lowercase(string::trim(full)) || wanted == string::lowercase(panelName(full)) || wanted == string::lowercase(name)))
			return true;
		start = end + 1;
	}
	return false;
}

/// What a jack carries where its maker says (`oroboro_rack_signals`: a line
/// each, the jack and the signal, a later one over an earlier one): by its
/// name as the source names it or as it reads here, or its place ("in 2",
/// "out 1"). Empty: the plugin goes by its name.
std::string signalOf(bool input, size_t index, const std::string& full, const std::string& name) {
	if (!oroboro_rack_signals || !*oroboro_rack_signals)
		return "";
	std::string list = oroboro_rack_signals, found;
	std::string place = string::f("%s %d", input ? "in" : "out", (int) index + 1);
	size_t start = 0;
	while (start <= list.size()) {
		size_t end = list.find('\n', start);
		if (end == std::string::npos)
			end = list.size();
		std::string line = list.substr(start, end - start);
		size_t tab = line.find('\t');
		if (tab != std::string::npos) {
			std::string jack = string::lowercase(string::trim(line.substr(0, tab)));
			if (!jack.empty()
				&& (jack == place || jack == string::lowercase(string::trim(full)) || jack == string::lowercase(panelName(full))
					|| jack == string::lowercase(name)))
				found = string::trim(line.substr(tab + 1));
		}
		start = end + 1;
	}
	return found;
}

/// A module of the plugin's model, made with its MIDI inputs on `midi`
/// (the device the plugin's MIDI comes to; null: none).
engine::Module* make(plugin::Model* m, midi::InputDevice* midi) {
	midi::InputDevice*& making = midi::oroboroMaking();
	making = midi;
	engine::Module* module = nullptr;
	try {
		module = m->createModule();
	}
	catch (...) {
		making = nullptr;
		throw;
	}
	making = nullptr;
	return module;
}

/// The interface, as `oroboro_module_spec` gives it: the module's knobs
/// and jacks as its source describes them (`configParam`, `configInput`,
/// …), named for a panel here, and whether it takes MIDI (a MIDI input
/// made as it's made).
std::string interface() {
	plugin::Model* m = model();
	if (!m)
		return "{}";
	midi::InputDevice midiDevice;
	std::unique_ptr<engine::Module> module(make(m, &midiDevice));
	if (!module)
		return "{}";
	bool takesMidi = !midiDevice.subscribed.empty();

	std::vector<std::string> knobs;
	std::string params;
	for (size_t i = 0; i < module->params.size(); i++) {
		engine::ParamQuantity* q = module->paramQuantities[i];
		float min = q ? q->getMinValue() : 0.f;
		float max = q ? q->getMaxValue() : 1.f;
		float def = q ? q->getDefaultValue() : 0.f;
		if (!std::isfinite(min) || !std::isfinite(max)) {
			min = 0.f;
			max = 1.f;
		}
		if (min > max)
			std::swap(min, max);
		def = std::isfinite(def) ? math::clamp(def, min, max) : min;
		std::string name = q ? panelName(q->name) : "";
		if (name.empty())
			name = string::f("Knob %d", (int) i + 1);
		name = unique(name, knobs);
		bool steps = q && q->snapEnabled;
		// its unit, when the knob's value is what's shown (Rack can show another value than the knob's)
		std::string unit;
		if (q && !steps && q->displayBase == 0.f && q->displayMultiplier == 1.f && q->displayOffset == 0.f)
			unit = string::trim(q->unit);
		std::string labels;
		if (steps && min == 0.f) {
			std::vector<std::string> names;
			if (engine::SwitchQuantity* sq = dynamic_cast<engine::SwitchQuantity*>(q))
				names = sq->labels;
			if (names.empty() && max == 1.f)
				names = {"Off", "On"};
			if ((float) names.size() == max + 1.f) {
				for (size_t k = 0; k < names.size(); k++)
					labels += (k ? ", " : "") + jsonText(panelName(names[k]));
				labels = ", \"labels\": [" + labels + "]";
			}
		}
		if (i)
			params += ", ";
		params += "{\"name\": " + jsonText(name) + ", \"min\": " + number(min) + ", \"max\": " + number(max) + ", \"default\": " + number(def)
			+ ", \"unit\": " + jsonText(unit) + ", \"scaling\": \"" + (steps ? "enum" : "lin") + "\"" + labels + "}";
	}

	// inputs: the pitch input is V/Oct here (the key plays it while nothing
	// is patched in), and an input that modulates a knob takes the knob's
	// name, so the plugin puts it on that knob
	std::vector<std::string> taken;
	std::string inputs;
	bool pitched = false;
	for (size_t i = 0; i < module->inputs.size(); i++) {
		engine::PortInfo* info = module->inputInfos[i];
		std::string full = info ? info->name : "";
		std::string name = panelName(full);
		std::string lower = string::lowercase(full);
		bool pitch = !pitched && (lower.find("v/oct") != std::string::npos || lower.find("1v/oct") != std::string::npos
			|| lower.find("volt/oct") != std::string::npos || lower.find("v/octave") != std::string::npos);
		if (pitch) {
			name = "V/Oct";
			pitched = true;
		}
		else {
			// the knob of its name ("Fold CV" and the Fold knob), else the one
			// knob whose name starts with it ("Rise CV" and "Rise time")
			std::string bare = string::lowercase(withoutSuffix(name));
			std::vector<std::string> exact, starting;
			for (const std::string& knob : knobs) {
				std::string k = string::lowercase(knob);
				if (k == bare)
					exact.push_back(knob);
				else if (bare != string::lowercase(name) && string::startsWith(k, bare + " "))
					starting.push_back(knob);
			}
			if (!exact.empty())
				name = exact[0];
			else if (starting.size() == 1)
				name = starting[0];
		}
		if (name.empty())
			name = string::f("In %d", (int) i + 1);
		name = unique(name, taken);
		if (i)
			inputs += ", ";
		std::string signal = signalOf(true, i, full, name);
		inputs += "{\"name\": " + jsonText(name) + (pitch ? ", \"pitch\": true" : "") + (hidden(full, name) ? ", \"hidden\": true" : "")
			+ (signal.empty() ? "" : ", \"signal\": " + jsonText(signal)) + "}";
	}
	taken.clear();
	std::string outputs;
	for (size_t i = 0; i < module->outputs.size(); i++) {
		engine::PortInfo* info = module->outputInfos[i];
		std::string full = info ? info->name : "";
		std::string name = panelName(full);
		if (name.empty())
			name = string::f("Out %d", (int) i + 1);
		name = unique(name, taken);
		if (i)
			outputs += ", ";
		// (a name alone, unless its maker says what it carries)
		std::string signal = signalOf(false, i, full, name);
		outputs += signal.empty() ? jsonText(name) : "{\"name\": " + jsonText(name) + ", \"signal\": " + jsonText(signal) + "}";
	}
	std::string vendor;
	if (oroboro_rack_vendor && *oroboro_rack_vendor)
		vendor = ", \"vendor\": " + jsonText(oroboro_rack_vendor);
	return "{\"module\": " + jsonText(oroboro_rack_module) + ", \"category\": " + jsonText(oroboro_rack_category) + ", \"inputs\": [" + inputs
		+ "], \"outputs\": [" + outputs + "], \"params\": [" + params + "], \"source_hash\": null" + vendor
		+ (takesMidi ? ", \"midi\": true" : "") + "}";
}

/// One instance: a Rack module, and what it's called with.
struct Instance {
	/// Where the plugin's MIDI comes to: its MIDI inputs are on it.
	std::unique_ptr<midi::InputDevice> midi{new midi::InputDevice};
	engine::Module* module = nullptr;
	engine::Module::ProcessArgs args = {};
	/// It threw once: silent from then on.
	bool broken = false;
	/// Its panel, made when its menu is first asked for (never shown: it's
	/// where the module's source puts its menu), or that it couldn't be made.
	app::ModuleWidget* widget = nullptr;
	bool no_widget = false;
	/// What the last `get_state`, `menu` and `panel` gave: theirs to keep until the next.
	std::string state;
	std::string menu;
	std::string panel;
	/// What its widgets' drawing is written down with (made when it's first asked to draw).
	NVGcontext* vg = nullptr;
	/// The pointer on its panel, as Rack's event state keeps it: the
	/// widget under it, the one dragged (and with which button), the one
	/// a drag is over, the one last clicked, where it is.
	widget::Widget* hovered = nullptr;
	widget::Widget* dragged = nullptr;
	int dragButton = 0;
	widget::Widget* dragHovered = nullptr;
	widget::Widget* clicked = nullptr;
	math::Vec mouse;
	/// The screen of the Oroboro panel the press it took was on (-1: none).
	int screen = -1;
};

/// A screen of an Oroboro panel (`oroboro_rack_ui`'s "screens"): the part
/// of the Rack panel (`from`) its widgets draw on, and where it is on the
/// Oroboro panel (x, y, w, h).
struct Screen {
	float rx, ry, rw, rh;
	float x, y, w, h;
};

/// The Oroboro panel's screens, read from it once.
static const std::vector<Screen>& screens() {
	static std::vector<Screen> list = [] {
		std::vector<Screen> found;
		if (!oroboro_rack_ui || !oroboro_rack_ui[0])
			return found;
		json_error_t error;
		json_t* root = json_loads(oroboro_rack_ui, 0, &error);
		if (!root)
			return found;
		json_t* array = json_object_get(root, "screens");
		for (size_t i = 0; array && i < json_array_size(array); i++) {
			json_t* s = json_array_get(array, i);
			json_t* from = json_object_get(s, "from");
			auto number = [](json_t* j) { return j ? (float) json_number_value(j) : 0.f; };
			Screen screen;
			screen.rx = number(json_array_get(from, 0));
			screen.ry = number(json_array_get(from, 1));
			screen.rw = number(json_array_get(from, 2));
			screen.rh = number(json_array_get(from, 3));
			screen.x = number(json_object_get(s, "x"));
			screen.y = number(json_object_get(s, "y"));
			screen.w = number(json_object_get(s, "w"));
			screen.h = number(json_object_get(s, "h"));
			if (screen.rw > 0.f && screen.rh > 0.f && screen.w > 0.f && screen.h > 0.f)
				found.push_back(screen);
		}
		json_decref(root);
		return found;
	}();
	return list;
}

/// The module's panel object, made once.
app::ModuleWidget* widget_of(Instance* instance) {
	if (!oroboro_rack_panel)
		return nullptr;
	if (!instance->widget && !instance->no_widget) {
		try {
			plugin::Model* m = instance->module->model;
			instance->widget = m ? m->createModuleWidget(instance->module) : nullptr;
		}
		catch (...) {
			instance->widget = nullptr;
		}
		instance->no_widget = !instance->widget;
	}
	return instance->widget;
}

/// A place on the panel as JSON members: a widget's box, from the panel's corner.
std::string boxJson(math::Vec at, math::Vec size) {
	return "\"x\": " + number(at.x) + ", \"y\": " + number(at.y) + ", \"w\": " + number(size.x) + ", \"h\": " + number(size.y);
}

/// What the panel has, by kind: each a list of JSON objects.
struct Layout {
	std::string params, inputs, outputs, lights;
	static void add(std::string& list, const std::string& entry) {
		if (!list.empty())
			list += ", ";
		list += entry;
	}
};

/// The knobs, jacks and lights among `widget`'s children, and theirs, each
/// with where it is on the panel (`at`: where `widget` is).
void layoutOf(widget::Widget* widget, math::Vec at, Layout& out, engine::Module* module, int depth) {
	if (depth > 12)
		return;
	for (widget::Widget* child : widget->children) {
		if (!child->visible)
			continue;
		math::Vec corner = at.plus(child->box.pos);
		if (app::ParamWidget* param = dynamic_cast<app::ParamWidget*>(child)) {
			if (param->paramId >= 0 && (size_t) param->paramId < module->params.size()) {
				const char* kind = "knob";
				app::SliderKnob* slider = dynamic_cast<app::SliderKnob*>(child);
				app::Switch* sw = dynamic_cast<app::Switch*>(child);
				app::SvgSwitch* frames = dynamic_cast<app::SvgSwitch*>(child);
				if (slider)
					kind = "slider";
				else if (sw)
					kind = (sw->momentary || (frames && frames->latch)) ? "button" : "switch";
				Layout::add(out.params, "{\"index\": " + std::to_string(param->paramId) + ", " + boxJson(corner, child->box.size) + ", \"kind\": \""
					+ kind + "\"" + (sw && sw->momentary ? ", \"momentary\": true" : "") + "}");
			}
		}
		else if (app::PortWidget* port = dynamic_cast<app::PortWidget*>(child)) {
			bool input = port->type == engine::Port::INPUT;
			size_t count = input ? module->inputs.size() : module->outputs.size();
			if (port->portId >= 0 && (size_t) port->portId < count)
				Layout::add(input ? out.inputs : out.outputs, "{\"index\": " + std::to_string(port->portId) + ", " + boxJson(corner, child->box.size) + "}");
		}
		else if (app::ModuleLightWidget* light = dynamic_cast<app::ModuleLightWidget*>(child)) {
			if (light->firstLightId >= 0 && (size_t) light->firstLightId < module->lights.size()) {
				// its colours: one light of the module each
				std::string colors;
				for (const NVGcolor& c : light->baseColors)
					Layout::add(colors, jsonText(string::f("#%02x%02x%02x", (int) std::round(math::clamp(c.r, 0.f, 1.f) * 255.f),
						(int) std::round(math::clamp(c.g, 0.f, 1.f) * 255.f), (int) std::round(math::clamp(c.b, 0.f, 1.f) * 255.f))));
				Layout::add(out.lights, "{\"first\": " + std::to_string(light->firstLightId) + ", " + boxJson(corner, child->box.size) + ", \"colors\": ["
					+ colors + "]}");
			}
		}
		// (a light inside a button, a knob inside a group)
		layoutOf(child, corner, out, module, depth + 1);
	}
}

/// The module's own menu, as its panel fills it.
ui::Menu* menu_of(Instance* instance) {
	app::ModuleWidget* widget = widget_of(instance);
	if (!widget)
		return nullptr;
	ui::Menu* menu = new ui::Menu;
	widget->appendContextMenu(menu);
	return menu;
}

/// A menu's entries as JSON, each item numbered as it comes (`next`), the
/// menus it opens inside it. Items are brought up to date first (`step`:
/// their check marks, the values they stand at).
std::string entries(ui::Menu* menu, uint32_t& next, int depth) {
	std::string out;
	auto add = [&](const std::string& entry) {
		if (!out.empty())
			out += ", ";
		out += entry;
	};
	for (widget::Widget* child : menu->children) {
		if (dynamic_cast<ui::MenuSeparator*>(child)) {
			add("{\"kind\": \"separator\"}");
		}
		else if (ui::MenuLabel* label = dynamic_cast<ui::MenuLabel*>(child)) {
			add("{\"kind\": \"label\", \"text\": " + jsonText(label->text) + "}");
		}
		else if (ui::MenuItem* item = dynamic_cast<ui::MenuItem*>(child)) {
			item->step();
			uint32_t id = next++;
			std::string right = item->rightText;
			bool checked = false, opens = false;
			size_t at;
			while ((at = right.find(CHECKMARK_STRING)) != std::string::npos) {
				right.erase(at, std::strlen(CHECKMARK_STRING));
				checked = true;
			}
			while ((at = right.find(RIGHT_ARROW)) != std::string::npos) {
				right.erase(at, std::strlen(RIGHT_ARROW));
				opens = true;
			}
			std::string inner;
			if (depth < 6) {
				if (ui::Menu* sub = item->createChildMenu()) {
					opens = true;
					inner = entries(sub, next, depth + 1);
					delete sub;
				}
			}
			add("{\"kind\": \"item\", \"id\": " + std::to_string(id) + ", \"text\": " + jsonText(item->text) + ", \"right\": "
				+ jsonText(string::trim(right)) + ", \"checked\": " + (checked ? "true" : "false") + ", \"disabled\": "
				+ (item->disabled ? "true" : "false") + (opens ? ", \"items\": [" + inner + "]" : "") + "}");
		}
		else if (ui::Slider* slider = dynamic_cast<ui::Slider*>(child)) {
			// a value to set: where it stands between its ends, and how it reads
			uint32_t id = next++;
			Quantity* q = slider->quantity;
			if (!q)
				continue;
			add("{\"kind\": \"slider\", \"id\": " + std::to_string(id) + ", \"text\": " + jsonText(q->getString()) + ", \"value\": "
				+ number(math::clamp(q->getScaledValue(), 0.f, 1.f)) + "}");
		}
	}
	return out;
}

/// The entry numbered `id` in `menu` (numbered as `entries` numbers them):
/// an item, or a slider. Menus opened on the way are kept in `opened`, for
/// whoever asked to free once it's done with the entry.
widget::Widget* entry(ui::Menu* menu, uint32_t id, uint32_t& next, int depth, std::vector<ui::Menu*>& opened) {
	for (widget::Widget* child : menu->children) {
		if (ui::MenuItem* item = dynamic_cast<ui::MenuItem*>(child)) {
			item->step();
			if (next++ == id)
				return item;
			if (depth < 6) {
				if (ui::Menu* sub = item->createChildMenu()) {
					opened.push_back(sub);
					if (widget::Widget* found = entry(sub, id, next, depth + 1, opened))
						return found;
				}
			}
		}
		else if (dynamic_cast<ui::Slider*>(child)) {
			if (next++ == id)
				return child;
		}
	}
	return nullptr;
}

std::atomic<int64_t> made{0};

} // namespace

extern "C" {

OROBORO_EXPORT uint32_t oroboro_module_abi(void) {
	return OROBORO_MODULE_ABI;
}

OROBORO_EXPORT const char* oroboro_module_spec(void) {
	static const std::string spec = [] {
		try {
			return interface();
		}
		catch (...) {
			return std::string("{}");
		}
	}();
	return spec.c_str();
}

OROBORO_EXPORT void* oroboro_module_new(float sample_rate) {
	try {
		plugin::Model* m = model();
		if (!m || !(sample_rate > 0.f))
			return nullptr;
		// (what a module asks the engine is what it's run at)
		APP->engine->sampleRate = sample_rate;
		settings::sampleRate = sample_rate;
		std::unique_ptr<Instance> instance(new Instance);
		instance->module = make(m, instance->midi.get());
		if (!instance->module)
			return nullptr;
		engine::Module* module = instance->module;
		module->id = ++made;
		instance->args.sampleRate = sample_rate;
		instance->args.sampleTime = 1.f / sample_rate;
		instance->args.frame = 0;
		// every jack has a cable of one channel, until the plugin says which inputs have none
		for (engine::Input& input : module->inputs)
			input.channels = 1;
		for (engine::Output& output : module->outputs)
			output.channels = 1;
		engine::Module::SampleRateChangeEvent rate = {sample_rate, 1.f / sample_rate};
		module->onSampleRateChange(rate);
		module->onAdd(engine::Module::AddEvent());
		return instance.release();
	}
	catch (...) {
		return nullptr;
	}
}

OROBORO_EXPORT void oroboro_module_free(void* m) {
	Instance* instance = static_cast<Instance*>(m);
	if (!instance)
		return;
	try {
		delete instance->widget;
		instance->widget = nullptr;
		oroboro_nvg_free(instance->vg);
		instance->vg = nullptr;
		if (instance->module) {
			instance->module->onRemove(engine::Module::RemoveEvent());
			delete instance->module;
		}
	}
	catch (...) {
	}
	delete instance;
}

OROBORO_EXPORT void oroboro_module_set_param(void* m, uint32_t index, float value) {
	Instance* instance = static_cast<Instance*>(m);
	if (instance && !instance->broken && index < instance->module->params.size())
		instance->module->params[index].value = value;
}

OROBORO_EXPORT void oroboro_module_reset(void* m) {
	Instance* instance = static_cast<Instance*>(m);
	if (!instance || instance->broken)
		return;
	try {
		// Rack's reset puts the knobs back too; here they stay where the patch has them
		engine::Module* module = instance->module;
		std::vector<engine::Param> kept = module->params;
		module->onReset(engine::Module::ResetEvent());
		module->params = kept;
	}
	catch (...) {
		instance->broken = true;
	}
}

OROBORO_EXPORT void oroboro_module_connected(void* m, const uint8_t* inputs, uint32_t count) {
	Instance* instance = static_cast<Instance*>(m);
	if (!instance || instance->broken)
		return;
	try {
		engine::Module* module = instance->module;
		for (size_t i = 0; i < module->inputs.size(); i++) {
			bool connected = i < count && inputs[i] != 0;
			engine::Input& input = module->inputs[i];
			if (connected == input.isConnected())
				continue;
			input.channels = connected ? 1 : 0;
			if (!connected)
				input.voltages[0] = 0.f;
			engine::Module::PortChangeEvent e = {connected, engine::Port::INPUT, (int) i};
			module->onPortChange(e);
		}
	}
	catch (...) {
		instance->broken = true;
	}
}

OROBORO_EXPORT float oroboro_module_get_param(void* m, uint32_t index) {
	Instance* instance = static_cast<Instance*>(m);
	if (!instance || index >= instance->module->params.size())
		return 0.f;
	return instance->module->params[index].value;
}

OROBORO_EXPORT const char* oroboro_module_get_state(void* m) {
	Instance* instance = static_cast<Instance*>(m);
	if (!instance || instance->broken)
		return nullptr;
	try {
		// what the module keeps with a Rack patch, beyond its knobs
		json_t* data = instance->module->dataToJson();
		if (!data)
			return nullptr;
		char* text = json_dumps(data, JSON_COMPACT | JSON_ENCODE_ANY);
		json_decref(data);
		if (!text)
			return nullptr;
		instance->state = text;
		std::free(text);
		return instance->state.c_str();
	}
	catch (...) {
		return nullptr;
	}
}

OROBORO_EXPORT void oroboro_module_set_state(void* m, const char* state) {
	Instance* instance = static_cast<Instance*>(m);
	if (!instance || instance->broken || !state)
		return;
	try {
		json_error_t error;
		json_t* data = json_loads(state, JSON_DECODE_ANY, &error);
		if (!data)
			return;
		instance->module->dataFromJson(data);
		json_decref(data);
	}
	catch (...) {
		instance->broken = true;
	}
}

} // extern "C"

/// A module's panel picture as Rack shows it: its panel (`setPanel`), else
/// the SVG panel at the bottom of its children (a plugin that adds its own:
/// Bogaudio's skins, `addChildBottom`).
static app::SvgPanel* panelOf(app::ModuleWidget* mw) {
	if (app::SvgPanel* panel = dynamic_cast<app::SvgPanel*>(mw->panel))
		return panel;
	for (widget::Widget* child : mw->children)
		if (app::SvgPanel* panel = dynamic_cast<app::SvgPanel*>(child))
			return panel;
	return nullptr;
}

extern "C" {
OROBORO_EXPORT const char* oroboro_module_panel(void* m) {
	Instance* instance = static_cast<Instance*>(m);
	if (!instance || instance->broken)
		return nullptr;
	if (oroboro_rack_ui)
		return oroboro_rack_ui[0] ? oroboro_rack_ui : nullptr;
	try {
		app::ModuleWidget* widget = widget_of(instance);
		if (!widget || widget->box.size.x <= 0.f || widget->box.size.y <= 0.f)
			return nullptr;
		Layout layout;
		layoutOf(widget, math::Vec(), layout, instance->module, 0);
		// its picture's file, and every SVG file its panel code loaded, with their sizes
		std::string art;
		if (app::SvgPanel* panel = panelOf(widget))
			if (panel->svg)
				art = panel->svg->path;
		std::string svgs;
		for (const std::shared_ptr<window::Svg>& svg : window::Svg::loaded())
			if (svg->size.x > 0.f && svg->size.y > 0.f)
				Layout::add(svgs, "{\"path\": " + jsonText(svg->path) + ", \"w\": " + number(svg->size.x) + ", \"h\": " + number(svg->size.y) + "}");
		instance->panel = "{\"width\": " + number(widget->box.size.x) + ", \"height\": " + number(widget->box.size.y) + ", \"art\": " + jsonText(art)
			+ ", \"params\": [" + layout.params + "], \"inputs\": [" + layout.inputs + "], \"outputs\": [" + layout.outputs + "], \"lights\": ["
			+ layout.lights + "], \"svgs\": [" + svgs + "]}";
		return instance->panel.c_str();
	}
	catch (...) {
		return nullptr;
	}
}

OROBORO_EXPORT uint32_t oroboro_module_lights(void* m, float* out, uint32_t count) {
	Instance* instance = static_cast<Instance*>(m);
	if (!instance)
		return 0;
	std::vector<engine::Light>& lights = instance->module->lights;
	uint32_t n = (uint32_t) lights.size();
	for (uint32_t i = 0; i < count && i < n; i++)
		out[i] = lights[i].value;
	return n;
}

OROBORO_EXPORT const float* oroboro_module_draw(void* m, uint32_t* count) {
	Instance* instance = static_cast<Instance*>(m);
	if (count)
		*count = 0;
	// (a panel in the plugin's own look has nothing of the Rack panel's on
	// it, but its screens: the parts of it they show, each where it is)
	if (!instance || instance->broken || (oroboro_rack_ui && screens().empty()))
		return nullptr;
	try {
		app::ModuleWidget* widget = widget_of(instance);
		if (!widget)
			return nullptr;
		if (!instance->vg)
			instance->vg = oroboro_nvg_new();
		// as Rack does each frame: every widget's step, then what each draws,
		// and over that what lights up by itself (layer 1)
		widget->step();
		oroboro_nvg_begin(instance->vg);
		widget::Widget::DrawArgs args;
		args.vg = instance->vg;
		if (oroboro_rack_ui) {
			for (const Screen& s : screens()) {
				nvgSave(args.vg);
				nvgScissor(args.vg, s.x, s.y, s.w, s.h);
				nvgTranslate(args.vg, s.x, s.y);
				nvgScale(args.vg, s.w / s.rw, s.h / s.rh);
				nvgTranslate(args.vg, -s.rx, -s.ry);
				args.clipBox = math::Rect(math::Vec(s.rx, s.ry), math::Vec(s.rw, s.rh));
				widget->draw(args);
				widget->drawLayer(args, 1);
				nvgRestore(args.vg);
			}
		}
		else {
			args.clipBox = math::Rect(math::Vec(), widget->box.size);
			widget->draw(args);
			widget->drawLayer(args, 1);
		}
		unsigned int n = 0;
		const float* list = oroboro_nvg_list(instance->vg, &n);
		if (count)
			*count = n;
		return list;
	}
	catch (...) {
		return nullptr;
	}
}

/// Whether `w` is `root` or among its widgets (a widget kept from an
/// earlier event may have been taken away since: never touched then).
static bool among(widget::Widget* root, widget::Widget* w) {
	if (!w)
		return false;
	if (root == w)
		return true;
	for (widget::Widget* child : root->children)
		if (among(child, w))
			return true;
	return false;
}

/// A widget the module itself took the pointer with: not its panel's
/// background (the module widget, its panel picture), where the plugin
/// moves the module instead.
static bool taken_by_module(app::ModuleWidget* mw, widget::Widget* target) {
	return target && target != mw && target != mw->panel && target != panelOf(mw);
}

OROBORO_EXPORT uint32_t oroboro_module_pointer(void* m, uint32_t kind, float x, float y, float dx, float dy, uint32_t button, uint32_t mods) {
	Instance* instance = static_cast<Instance*>(m);
	if (!instance || instance->broken || (oroboro_rack_ui && screens().empty()))
		return 0;
	// an Oroboro panel: the pointer on one of its screens is the Rack
	// panel's there; a drag stays with the screen it started on
	if (oroboro_rack_ui) {
		const std::vector<Screen>& list = screens();
		int on = -1;
		for (size_t i = 0; i < list.size(); i++)
			if (x >= list[i].x && x < list[i].x + list[i].w && y >= list[i].y && y < list[i].y + list[i].h)
				on = (int) i;
		bool dragging = instance->dragged && instance->screen >= 0 && instance->screen < (int) list.size();
		int which = dragging && (kind == 2 || kind == 3) ? instance->screen : on;
		if (which < 0) {
			// off every screen: as off the panel
			if (kind == 3 || kind == 6)
				kind = 6;
			else
				return 0;
		}
		else {
			const Screen& s = list[which];
			float sx = s.rw / s.w, sy = s.rh / s.h;
			x = s.rx + (x - s.x) * sx;
			y = s.ry + (y - s.y) * sy;
			dx *= sx;
			dy *= sy;
			if (kind == 1)
				instance->screen = which;
		}
	}
	uint32_t taken = 0;
	// a dialog opens only where the plugin says it may: not on the thread
	// where it draws its window
	oroboro_osdialog_closed = (mods & OROBORO_POINTER_MAY_ASK) ? 0 : 1;
	mods &= ~OROBORO_POINTER_MAY_ASK;
	try {
		app::ModuleWidget* mw = widget_of(instance);
		if (!mw) {
			oroboro_osdialog_closed = 0;
			return 0;
		}
		event::State* state = APP->event;
		auto keep = [&](widget::Widget*& w) {
			if (!among(mw, w))
				w = nullptr;
		};
		keep(instance->hovered);
		keep(instance->dragged);
		keep(instance->dragHovered);
		keep(instance->clicked);
		state->rootWidget = mw;
		state->hoveredWidget = instance->hovered;
		state->draggedWidget = instance->dragged;
		state->dragButton = instance->dragButton;
		state->dragHoveredWidget = instance->dragHovered;
		state->lastClickedWidget = instance->clicked;
		math::Vec pos(x, y);
		instance->mouse = pos;
		APP->scene->mousePos = pos;
		switch (kind) {
		case 1: { // press
			widget::EventContext context;
			widget::Widget::ButtonEvent e;
			e.context = &context;
			e.pos = pos;
			e.button = (int) button;
			e.action = GLFW_PRESS;
			e.mods = (int) mods;
			mw->onButton(e);
			widget::Widget* target = context.target;
			if (context.consumed && taken_by_module(mw, target)) {
				taken = 1;
				if (button == GLFW_MOUSE_BUTTON_LEFT) {
					// it's dragged, and selected, as in Rack
					if (instance->clicked != target && instance->clicked) {
						widget::EventContext c2;
						widget::Widget::DeselectEvent d;
						d.context = &c2;
						instance->clicked->onDeselect(d);
					}
					instance->clicked = target;
					widget::EventContext c3;
					widget::Widget::SelectEvent sel;
					sel.context = &c3;
					target->onSelect(sel);
					instance->dragged = target;
					instance->dragButton = (int) button;
					widget::EventContext c4;
					widget::Widget::DragStartEvent start;
					start.context = &c4;
					start.button = (int) button;
					target->onDragStart(start);
				}
			}
			break;
		}
		case 2: { // release
			if (instance->dragged) {
				widget::Widget* dragged = instance->dragged;
				if (instance->dragHovered) {
					widget::EventContext c1;
					widget::Widget::DragDropEvent drop;
					drop.context = &c1;
					drop.button = instance->dragButton;
					drop.origin = dragged;
					instance->dragHovered->onDragDrop(drop);
				}
				widget::EventContext c2;
				widget::Widget::DragEndEvent end;
				end.context = &c2;
				end.button = instance->dragButton;
				if (among(mw, dragged))
					dragged->onDragEnd(end);
				instance->dragged = nullptr;
				instance->dragHovered = nullptr;
				taken = 1;
			}
			widget::EventContext context;
			widget::Widget::ButtonEvent e;
			e.context = &context;
			e.pos = pos;
			e.button = (int) button;
			e.action = GLFW_RELEASE;
			e.mods = (int) mods;
			mw->onButton(e);
			break;
		}
		case 3: { // moved: a drag, else the pointer over it
			if (instance->dragged) {
				widget::EventContext c1;
				widget::Widget::DragMoveEvent move;
				move.context = &c1;
				move.button = instance->dragButton;
				move.mouseDelta = math::Vec(dx, dy);
				instance->dragged->onDragMove(move);
				// what it's dragged over
				widget::EventContext c2;
				widget::Widget::DragHoverEvent over;
				over.context = &c2;
				over.button = instance->dragButton;
				over.pos = pos;
				over.origin = instance->dragged;
				over.mouseDelta = math::Vec(dx, dy);
				if (among(mw, instance->dragged))
					mw->onDragHover(over);
				widget::Widget* now = c2.consumed ? c2.target : nullptr;
				if (now != instance->dragHovered) {
					if (instance->dragHovered) {
						widget::EventContext c3;
						widget::Widget::DragLeaveEvent leave;
						leave.context = &c3;
						leave.origin = instance->dragged;
						instance->dragHovered->onDragLeave(leave);
					}
					if (now) {
						widget::EventContext c3;
						widget::Widget::DragEnterEvent enter;
						enter.context = &c3;
						enter.origin = instance->dragged;
						now->onDragEnter(enter);
					}
					instance->dragHovered = now;
				}
				taken = 1;
			}
			else {
				widget::EventContext context;
				widget::Widget::HoverEvent e;
				e.context = &context;
				e.pos = pos;
				e.mouseDelta = math::Vec(dx, dy);
				mw->onHover(e);
				widget::Widget* now = context.consumed ? context.target : nullptr;
				if (now != instance->hovered) {
					if (instance->hovered) {
						widget::EventContext c2;
						widget::Widget::LeaveEvent leave;
						leave.context = &c2;
						instance->hovered->onLeave(leave);
					}
					if (now) {
						widget::EventContext c2;
						widget::Widget::EnterEvent enter;
						enter.context = &c2;
						now->onEnter(enter);
					}
					instance->hovered = now;
				}
				taken = taken_by_module(mw, now) ? 1 : 0;
			}
			break;
		}
		case 4: { // the wheel
			widget::EventContext context;
			widget::Widget::HoverScrollEvent e;
			e.context = &context;
			e.pos = pos;
			e.scrollDelta = math::Vec(dx, dy);
			mw->onHoverScroll(e);
			taken = context.consumed && taken_by_module(mw, context.target) ? 1 : 0;
			break;
		}
		case 5: { // a double click: to what took the click
			if (instance->clicked && among(mw, instance->clicked)) {
				widget::EventContext context;
				widget::Widget::DoubleClickEvent e;
				e.context = &context;
				instance->clicked->onDoubleClick(e);
				taken = 1;
			}
			break;
		}
		case 6: { // off the panel
			if (instance->hovered) {
				widget::EventContext context;
				widget::Widget::LeaveEvent leave;
				leave.context = &context;
				instance->hovered->onLeave(leave);
				instance->hovered = nullptr;
			}
			break;
		}
		default:
			break;
		}
		state->hoveredWidget = instance->hovered;
		state->draggedWidget = instance->dragged;
		state->dragHoveredWidget = instance->dragHovered;
		state->selectedWidget = instance->clicked;
		state->lastClickedWidget = instance->clicked;
	}
	catch (...) {
		taken = 0;
	}
	oroboro_osdialog_closed = 0;
	return taken;
}

OROBORO_EXPORT const uint8_t* oroboro_module_picture(uint32_t index, uint32_t* size) {
	uint32_t i = 0;
	for (const OroboroRackPicture* p = oroboro_rack_pictures; p && p->path; p++, i++) {
		if (i == index) {
			if (size)
				*size = p->size;
			return p->bytes;
		}
	}
	if (size)
		*size = 0;
	return nullptr;
}

OROBORO_EXPORT const uint8_t* oroboro_module_art(uint32_t* size) {
	// (a panel in the plugin's own look has no picture)
	bool has = oroboro_rack_art && !oroboro_rack_ui;
	if (size)
		*size = has ? oroboro_rack_art_size : 0;
	return has ? oroboro_rack_art : nullptr;
}

/// A Rack module's menu, state and knobs are asked while it plays, as Rack
/// asks them from its window while its engine runs.
OROBORO_EXPORT uint32_t oroboro_module_live(void) {
	return 1;
}

OROBORO_EXPORT const char* oroboro_module_menu(void* m) {
	Instance* instance = static_cast<Instance*>(m);
	if (!instance || instance->broken)
		return nullptr;
	try {
		std::unique_ptr<ui::Menu> menu(menu_of(instance));
		if (!menu || menu->children.empty())
			return nullptr;
		uint32_t next = 0;
		instance->menu = "[" + entries(menu.get(), next, 0) + "]";
		return instance->menu.c_str();
	}
	catch (...) {
		return nullptr;
	}
}

OROBORO_EXPORT void oroboro_module_menu_choose(void* m, uint32_t id, float value) {
	Instance* instance = static_cast<Instance*>(m);
	if (!instance || instance->broken)
		return;
	std::vector<ui::Menu*> opened;
	try {
		std::unique_ptr<ui::Menu> menu(menu_of(instance));
		if (menu) {
			uint32_t next = 0;
			widget::Widget* found = entry(menu.get(), id, next, 0, opened);
			if (ui::MenuItem* item = dynamic_cast<ui::MenuItem*>(found)) {
				if (!item->disabled)
					item->doAction();
			}
			else if (ui::Slider* slider = dynamic_cast<ui::Slider*>(found)) {
				if (slider->quantity)
					slider->quantity->setScaledValue(math::clamp(value, 0.f, 1.f));
			}
		}
	}
	catch (...) {
	}
	for (ui::Menu* sub : opened)
		delete sub;
}

OROBORO_EXPORT void oroboro_module_midi(void* m, const uint8_t* message, uint32_t size) {
	Instance* instance = static_cast<Instance*>(m);
	if (!instance || !message || size == 0 || size > 3 || instance->broken || instance->midi->subscribed.empty())
		return;
	try {
		midi::Message msg;
		msg.bytes.assign(message, message + size);
		// (for the tick that comes next: a queue gives it to the module then)
		msg.setFrame(instance->args.frame);
		instance->midi->onMessage(msg);
	}
	catch (...) {
		instance->broken = true;
	}
}

OROBORO_EXPORT void oroboro_module_tick(void* m, const float* inputs, float* outputs) {
	Instance* instance = static_cast<Instance*>(m);
	engine::Module* module = instance->module;
	size_t nIn = module->inputs.size(), nOut = module->outputs.size();
	if (!instance->broken) {
		try {
			for (size_t i = 0; i < nIn; i++)
				if (module->inputs[i].channels)
					module->inputs[i].voltages[0] = inputs[i];
			APP->engine->frame = instance->args.frame;
			module->process(instance->args);
			instance->args.frame++;
			for (size_t i = 0; i < nOut; i++) {
				float v = module->outputs[i].voltages[0];
				outputs[i] = std::isfinite(v) ? v : 0.f;
			}
			return;
		}
		catch (...) {
			instance->broken = true;
		}
	}
	for (size_t i = 0; i < nOut; i++)
		outputs[i] = 0.f;
}

} // extern "C"
