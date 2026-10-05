// Tremor: a tremolo, written the way a VCV Rack 2 module is. An example
// for `oromod rack build` (docs/rack-modules.md), and what its test builds.
// The Oroboro Modular SDK's own code.
#include "plugin.hpp"

struct Tremor : Module {
	enum ParamId { RATE_PARAM, DEPTH_PARAM, SHAPE_PARAM, HOLD_PARAM, PARAMS_LEN };
	enum InputId { IN_INPUT, RATE_INPUT, DEPTH_INPUT, SYNC_INPUT, INPUTS_LEN };
	enum OutputId { OUT_OUTPUT, LFO_OUTPUT, OUTPUTS_LEN };
	enum LightId { RATE_LIGHT, LIGHTS_LEN };

	float phase = 0.f;
	/// Where the LFO stands now, -1 to 1 (its panel draws it).
	float lfoNow = 0.f;
	dsp::SchmittTrigger sync;
	dsp::PulseGenerator blink;
	// its menu's options, kept with the patch (dataToJson)
	bool invert = false;
	int range = 0;

	Tremor() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(RATE_PARAM, 0.1f, 20.f, 4.f, "Rate", " Hz");
		configParam(DEPTH_PARAM, 0.f, 1.f, 0.5f, "Depth (how far the level falls)", "%", 0.f, 100.f);
		configSwitch(SHAPE_PARAM, 0.f, 2.f, 0.f, "Shape", {"Sine", "Triangle", "Square"});
		configButton(HOLD_PARAM, "Hold");
		configInput(IN_INPUT, "Audio");
		configInput(RATE_INPUT, "Rate CV (1 V doubles it)");
		configInput(DEPTH_INPUT, "Depth CV");
		configInput(SYNC_INPUT, "Sync");
		configOutput(OUT_OUTPUT, "Audio");
		configOutput(LFO_OUTPUT, "LFO (0 to 10 V)");
		configBypass(IN_INPUT, OUT_OUTPUT);
	}

	void onReset() override { phase = 0.f; }

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "invert", json_boolean(invert));
		json_object_set_new(root, "range", json_integer(range));
		return root;
	}
	void dataFromJson(json_t* root) override {
		if (json_t* j = json_object_get(root, "invert"))
			invert = json_boolean_value(j);
		if (json_t* j = json_object_get(root, "range"))
			range = (int) json_integer_value(j);
	}

	void process(const ProcessArgs& args) override {
		float rate = params[RATE_PARAM].getValue() * std::pow(2.f, inputs[RATE_INPUT].getVoltage());
		float depth = clamp(params[DEPTH_PARAM].getValue() + inputs[DEPTH_INPUT].getVoltage() / 10.f, 0.f, 1.f);
		if (sync.process(inputs[SYNC_INPUT].getVoltage(), 0.1f, 1.f))
			phase = 0.f;
		if (params[HOLD_PARAM].getValue() < 0.5f) {
			phase += rate * args.sampleTime;
			if (phase >= 1.f) {
				phase -= std::floor(phase);
				blink.trigger(0.05f);
			}
		}
		float lfo;
		switch ((int) params[SHAPE_PARAM].getValue()) {
			case 1: lfo = 1.f - 2.f * std::fabs(2.f * phase - 1.f); break;
			case 2: lfo = phase < 0.5f ? 1.f : -1.f; break;
			default: lfo = std::sin(2.f * (float) M_PI * phase);
		}
		// without anything at Audio it's an LFO of its own: a steady 5 V goes through
		float in = inputs[IN_INPUT].isConnected() ? inputs[IN_INPUT].getVoltage() : 5.f;
		if (invert)
			lfo = -lfo;
		if (range == 1)
			depth *= 0.5f;
		lfoNow = lfo;
		float level = 1.f - depth * (0.5f - 0.5f * lfo);
		outputs[OUT_OUTPUT].setVoltage(in * level);
		outputs[LFO_OUTPUT].setVoltage(5.f + 5.f * lfo);
		lights[RATE_LIGHT].setBrightnessSmooth(blink.process(args.sampleTime) ? 1.f : 0.f, args.sampleTime);
	}
};

/// What a panel draws itself: the LFO's shape with a dot where it stands
/// now, and its rate in figures beside it.
struct TremorDisplay : OpaqueWidget {
	Tremor* module = nullptr;

	// a click on the screen picks the next shape, as the Shape switch does
	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && module) {
			float shape = module->params[Tremor::SHAPE_PARAM].getValue();
			module->params[Tremor::SHAPE_PARAM].setValue(std::fmod(std::round(shape) + 1.f, 3.f));
			e.consume(this);
		}
	}

	void draw(const DrawArgs& args) override {
		// (the wave in the left of its box, the figures in the right)
		float w = box.size.x * 0.6f, h = box.size.y;
		int shape = module ? (int) module->params[Tremor::SHAPE_PARAM].getValue() : 0;
		auto wave = [shape](float phase) {
			switch (shape) {
				case 1: return 1.f - 2.f * std::fabs(2.f * phase - 1.f);
				case 2: return phase < 0.5f ? 1.f : -1.f;
				default: return std::sin(2.f * (float) M_PI * phase);
			}
		};
		nvgBeginPath(args.vg);
		for (int i = 0; i <= 32; i++) {
			float phase = i / 32.f;
			float x = phase * w, y = h * 0.5f - wave(phase) * h * 0.4f;
			if (i == 0)
				nvgMoveTo(args.vg, x, y);
			else
				nvgLineTo(args.vg, x, y);
		}
		nvgStrokeColor(args.vg, nvgRGB(0x2b, 0x2b, 0x2b));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);
		if (!module)
			return;
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, module->phase * w, h * 0.5f - module->lfoNow * h * 0.4f, 2.5f);
		nvgFillColor(args.vg, nvgRGB(0xd0, 0x40, 0x30));
		nvgFill(args.vg);
		nvgFontSize(args.vg, 7.f);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgFillColor(args.vg, nvgRGB(0x2b, 0x2b, 0x2b));
		nvgText(args.vg, box.size.x * 0.68f, h * 0.5f, string::f("%.1f", module->params[Tremor::RATE_PARAM].getValue()).c_str(), NULL);
	}
};

/// A button with no knob behind it, that only acts (as a LOAD button does):
/// let go, it flips the LFO, as the menu's Invert does.
struct FlipButton : VCVButton {
	Tremor* module = nullptr;
	void onDragEnd(const DragEndEvent& e) override {
		VCVButton::onDragEnd(e);
		if (module)
			module->invert = !module->invert;
	}
};

struct TremorWidget : ModuleWidget {
	TremorWidget(Tremor* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Tremor.svg")));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 22.0)), module, Tremor::RATE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 42.0)), module, Tremor::DEPTH_PARAM));
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(10.16, 58.0)), module, Tremor::SHAPE_PARAM));
		addParam(createParamCentered<VCVLatch>(mm2px(Vec(10.16, 70.0)), module, Tremor::HOLD_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.0, 84.0)), module, Tremor::RATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.0, 84.0)), module, Tremor::DEPTH_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.0, 98.0)), module, Tremor::SYNC_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.0, 98.0)), module, Tremor::IN_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.0, 112.0)), module, Tremor::LFO_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.0, 112.0)), module, Tremor::OUT_OUTPUT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(10.16, 12.0)), module, Tremor::RATE_LIGHT));
		TremorDisplay* display = createWidget<TremorDisplay>(mm2px(Vec(3.0, 121.0)));
		display->box.size = mm2px(Vec(14.32, 4.6));
		display->module = module;
		addChild(display);
		FlipButton* flip = createWidgetCentered<FlipButton>(mm2px(Vec(16.5, 58.0)));
		flip->module = module;
		addChild(flip);
	}

	void appendContextMenu(Menu* menu) override {
		Tremor* tremor = dynamic_cast<Tremor*>(module);
		if (!tremor)
			return;
		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Tremor"));
		menu->addChild(createBoolPtrMenuItem("Invert the LFO", "", &tremor->invert));
		menu->addChild(createIndexPtrSubmenuItem("Depth range", {"Full", "Half"}, &tremor->range));
	}
};

Model* modelTremor = createModel<Tremor, TremorWidget>("Tremor");
