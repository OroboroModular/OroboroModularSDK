// SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-Oroboro-Rack-Bridge-Exception (rack/LICENSE-EXCEPTION.md)
// `rack::engine`: what a Rack module is made of (its knobs, jacks and
// lights, how it describes them, and the `process` it's called with each
// sample), with `rack::Quantity`, which a knob's description builds on.
// The SDK's own code, to the same interface as VCV Rack 2's
// (docs/rack-modules.md).
#pragma once

#include <jansson.h>
#include <nanovg.h>

#include "rack_core.hpp"

namespace rack {

namespace widget {
struct Widget;
}

// ---- Quantity ------------------------------------------------------------------------------

/// A value with a range, as sliders and menus show one.
struct Quantity {
	virtual ~Quantity() {}

	virtual void setValue(float value) { setImmediateValue(value); }
	virtual float getValue() { return 0.f; }
	virtual void setImmediateValue(float value) {}
	virtual float getMinValue() { return 0.f; }
	virtual float getMaxValue() { return 1.f; }
	virtual float getDefaultValue() { return 0.f; }
	virtual float getDisplayValue() { return getValue(); }
	virtual void setDisplayValue(float displayValue) { setValue(displayValue); }
	virtual int getDisplayPrecision() { return 5; }
	virtual std::string getDisplayValueString() {
		char text[32];
		std::snprintf(text, sizeof(text), "%.*g", getDisplayPrecision(), (double) math::normalizeZero(getDisplayValue()));
		return text;
	}
	virtual void setDisplayValueString(std::string s) {
		char* end = nullptr;
		float v = std::strtof(s.c_str(), &end);
		if (end != s.c_str() && std::isfinite(v))
			setDisplayValue(v);
	}
	virtual std::string getLabel() { return ""; }
	virtual std::string getUnit() { return ""; }
	virtual std::string getString() {
		std::string s;
		std::string label = getLabel();
		if (!label.empty())
			s += label + ": ";
		s += getDisplayValueString() + getUnit();
		return s;
	}
	virtual void reset() { setValue(getDefaultValue()); }
	virtual void randomize() {
		if (isBounded())
			setScaledValue(random::uniform());
	}

	bool isMin() { return getValue() <= getMinValue(); }
	bool isMax() { return getValue() >= getMaxValue(); }
	void setMin() { setValue(getMinValue()); }
	void setMax() { setValue(getMaxValue()); }
	void toggle() { setValue(isMin() ? getMaxValue() : getMinValue()); }
	void moveValue(float deltaValue) { setValue(getValue() + deltaValue); }
	float getRange() { return getMaxValue() - getMinValue(); }
	bool isBounded() { return std::isfinite(getMinValue()) && std::isfinite(getMaxValue()); }
	float toScaled(float value) {
		if (!isBounded())
			return value;
		if (getMinValue() == getMaxValue())
			return 0.f;
		return math::rescale(value, getMinValue(), getMaxValue(), 0.f, 1.f);
	}
	float fromScaled(float scaledValue) {
		if (!isBounded())
			return scaledValue;
		return math::rescale(scaledValue, 0.f, 1.f, getMinValue(), getMaxValue());
	}
	void setScaledValue(float scaledValue) { setValue(fromScaled(scaledValue)); }
	float getScaledValue() { return toScaled(getValue()); }
	void moveScaledValue(float deltaScaledValue) {
		if (!isBounded())
			moveValue(deltaScaledValue);
		else
			moveValue(deltaScaledValue * getRange());
	}
};

namespace ui {
using rack::Quantity;
}

namespace engine {

/// The most channels a cable carries.
static const int PORT_MAX_CHANNELS = 16;

struct Module;

/// A knob's value.
struct Param {
	float value = 0.f;

	float getValue() { return value; }
	void setValue(float value) { this->value = value; }
};

/// A light's brightness.
struct Light {
	float value = 0.f;

	void setBrightness(float brightness) { value = brightness; }
	float getBrightness() { return value; }
	/// Rises at once, falls smoothly (`lambda` a second).
	void setBrightnessSmooth(float brightness, float deltaTime, float lambda = 30.f) {
		if (brightness < value)
			brightness += (value - brightness) * std::exp(-lambda * deltaTime);
		value = brightness;
	}
	void setSmoothBrightness(float brightness, float deltaTime) { setBrightnessSmooth(brightness, deltaTime); }
	DEPRECATED void setBrightnessSmooth(float brightness, int frames = 1) { setBrightnessSmooth(brightness, frames / 44100.f); }
};

/// A jack: up to 16 channels of voltage. Here a cable carries one (each
/// voice of a patch has its own instance of the module).
struct Port {
	union {
		float voltages[PORT_MAX_CHANNELS] = {};
		float value;
	};
	union {
		uint8_t channels = 0;
		uint8_t active;
	};
	Light plugLights[3];

	enum Type { INPUT, OUTPUT };

	void setVoltage(float voltage, int channel = 0) { voltages[channel] = voltage; }
	float getVoltage(int channel = 0) { return voltages[channel]; }
	/// The channel's voltage, or the first's when the cable has only one.
	float getPolyVoltage(int channel) { return isMonophonic() ? getVoltage(0) : getVoltage(channel); }
	/// The voltage, or `normalVoltage` without a cable.
	float getNormalVoltage(float normalVoltage, int channel = 0) { return isConnected() ? getVoltage(channel) : normalVoltage; }
	float getNormalPolyVoltage(float normalVoltage, int channel) { return isConnected() ? getPolyVoltage(channel) : normalVoltage; }
	float* getVoltages(int firstChannel = 0) { return &voltages[firstChannel]; }
	void readVoltages(float* v) {
		for (int c = 0; c < channels; c++)
			v[c] = voltages[c];
	}
	void writeVoltages(const float* v) {
		for (int c = 0; c < channels; c++)
			voltages[c] = v[c];
	}
	void clearVoltages() {
		for (int c = 0; c < channels; c++)
			voltages[c] = 0.f;
	}
	float getVoltageSum() {
		float sum = 0.f;
		for (int c = 0; c < channels; c++)
			sum += voltages[c];
		return sum;
	}
	float getVoltageRMS() {
		if (channels == 0)
			return 0.f;
		if (channels == 1)
			return std::fabs(voltages[0]);
		float sum = 0.f;
		for (int c = 0; c < channels; c++)
			sum += voltages[c] * voltages[c];
		return std::sqrt(sum);
	}
	template <typename T>
	T getVoltageSimd(int firstChannel) {
		return T::load(&voltages[firstChannel]);
	}
	template <typename T>
	T getPolyVoltageSimd(int firstChannel) {
		return isMonophonic() ? T(getVoltage(0)) : getVoltageSimd<T>(firstChannel);
	}
	template <typename T>
	T getNormalVoltageSimd(T normalVoltage, int firstChannel) {
		return isConnected() ? getVoltageSimd<T>(firstChannel) : normalVoltage;
	}
	template <typename T>
	T getNormalPolyVoltageSimd(T normalVoltage, int firstChannel) {
		return isConnected() ? getPolyVoltageSimd<T>(firstChannel) : normalVoltage;
	}
	template <typename T>
	void setVoltageSimd(T voltage, int firstChannel) {
		voltage.store(&voltages[firstChannel]);
	}
	/// How many channels an output carries. Nothing changes a jack that has
	/// no cable; here a cable always carries one.
	void setChannels(int channels) {
		if (this->channels == 0)
			return;
		this->channels = 1;
	}
	int getChannels() { return channels; }
	bool isConnected() { return channels > 0; }
	bool isMonophonic() { return channels == 1; }
	bool isPolyphonic() { return channels > 1; }
	DEPRECATED float normalize(float normalVoltage) { return getNormalVoltage(normalVoltage); }
};

struct Output : Port {};
struct Input : Port {};

/// What a knob is: its range, its name, how its value reads.
struct ParamQuantity : Quantity {
	Module* module = nullptr;
	int paramId = -1;

	float minValue = 0.f;
	float maxValue = 1.f;
	float defaultValue = 0.f;
	std::string name;
	std::string unit;
	float displayBase = 0.f;
	float displayMultiplier = 1.f;
	float displayOffset = 0.f;
	int displayPrecision = 5;
	std::string description;
	bool resetEnabled = true;
	bool randomizeEnabled = true;
	bool smoothEnabled = false;
	bool snapEnabled = false;

	Param* getParam();
	void setSmoothValue(float value) { setValue(value); }
	float getSmoothValue() { return getValue(); }

	void setValue(float value) override;
	float getValue() override;
	void setImmediateValue(float value) override;
	float getImmediateValue() { return getValue(); }
	float getMinValue() override { return minValue; }
	float getMaxValue() override { return maxValue; }
	float getDefaultValue() override { return defaultValue; }
	float getDisplayValue() override;
	void setDisplayValue(float displayValue) override;
	int getDisplayPrecision() override { return displayPrecision; }
	std::string getDisplayValueString() override;
	void setDisplayValueString(std::string s) override;
	std::string getLabel() override { return name; }
	std::string getUnit() override { return unit; }
	void reset() override;
	void randomize() override;
	virtual std::string getDescription() { return description; }
	virtual json_t* toJson();
	virtual void fromJson(json_t* rootJ);
};

/// A knob with named positions.
struct SwitchQuantity : ParamQuantity {
	std::vector<std::string> labels;

	std::string getDisplayValueString() override;
	void setDisplayValueString(std::string s) override;
};

struct PortInfo {
	Module* module = nullptr;
	Port::Type type = Port::INPUT;
	int portId = -1;
	std::string name;
	std::string description;

	virtual ~PortInfo() {}
	virtual std::string getName() { return name; }
	std::string getFullName() { return getName() + (type == Port::INPUT ? " input" : " output"); }
	virtual std::string getDescription() { return description; }
};

struct LightInfo {
	Module* module = nullptr;
	int lightId = -1;
	std::string name;
	std::string description;

	virtual ~LightInfo() {}
	virtual std::string getName() { return name; }
	virtual std::string getDescription() { return description; }
};

/// A module: knobs, jacks, lights, and what it does each sample.
struct Module : WeakBase {
	plugin::Model* model = nullptr;
	int64_t id = -1;

	std::vector<Param> params;
	std::vector<Input> inputs;
	std::vector<Output> outputs;
	std::vector<Light> lights;

	std::vector<ParamQuantity*> paramQuantities;
	std::vector<PortInfo*> inputInfos;
	std::vector<PortInfo*> outputInfos;
	std::vector<LightInfo*> lightInfos;

	/// The module beside this one, for modules that work in pairs. There is
	/// never one here.
	struct Expander {
		int64_t moduleId = -1;
		Module* module = nullptr;
		void* producerMessage = nullptr;
		void* consumerMessage = nullptr;
		bool messageFlipRequested = false;
		void requestMessageFlip() { messageFlipRequested = true; }
	};
	Expander leftExpander;
	Expander rightExpander;

	struct BypassRoute {
		int inputId = -1;
		int outputId = -1;
	};
	std::vector<BypassRoute> bypassRoutes;

	Module() {}
	Module(const Module&) = delete;
	Module& operator=(const Module&) = delete;
	DEPRECATED Module(int numParams, int numInputs, int numOutputs, int numLights = 0) : Module() {
		config(numParams, numInputs, numOutputs, numLights);
	}
	virtual ~Module();

	void config(int numParams, int numInputs, int numOutputs, int numLights = 0);

	template <class TParamQuantity = ParamQuantity>
	TParamQuantity* configParam(int paramId, float minValue, float maxValue, float defaultValue, std::string name = "", std::string unit = "",
		float displayBase = 0.f, float displayMultiplier = 1.f, float displayOffset = 0.f) {
		assert(paramId >= 0 && (size_t) paramId < params.size() && (size_t) paramId < paramQuantities.size());
		if (paramQuantities[paramId])
			delete paramQuantities[paramId];
		TParamQuantity* q = new TParamQuantity;
		q->ParamQuantity::module = this;
		q->ParamQuantity::paramId = paramId;
		q->ParamQuantity::minValue = minValue;
		q->ParamQuantity::maxValue = maxValue;
		q->ParamQuantity::defaultValue = defaultValue;
		q->ParamQuantity::name = name;
		q->ParamQuantity::unit = unit;
		q->ParamQuantity::displayBase = displayBase;
		q->ParamQuantity::displayMultiplier = displayMultiplier;
		q->ParamQuantity::displayOffset = displayOffset;
		paramQuantities[paramId] = q;
		params[paramId].value = q->getDefaultValue();
		return q;
	}

	template <class TSwitchQuantity = SwitchQuantity>
	TSwitchQuantity* configSwitch(int paramId, float minValue, float maxValue, float defaultValue, std::string name = "",
		std::vector<std::string> labels = {}) {
		TSwitchQuantity* sq = configParam<TSwitchQuantity>(paramId, minValue, maxValue, defaultValue, name);
		sq->SwitchQuantity::labels = labels;
		sq->smoothEnabled = false;
		sq->snapEnabled = true;
		return sq;
	}

	template <class TSwitchQuantity = SwitchQuantity>
	TSwitchQuantity* configButton(int paramId, std::string name = "") {
		TSwitchQuantity* sq = configParam<TSwitchQuantity>(paramId, 0.f, 1.f, 0.f, name);
		sq->randomizeEnabled = false;
		sq->smoothEnabled = false;
		sq->snapEnabled = true;
		return sq;
	}

	template <class TPortInfo = PortInfo>
	TPortInfo* configInput(int portId, std::string name = "") {
		assert(portId >= 0 && (size_t) portId < inputs.size() && (size_t) portId < inputInfos.size());
		if (inputInfos[portId])
			delete inputInfos[portId];
		TPortInfo* info = new TPortInfo;
		info->PortInfo::module = this;
		info->PortInfo::type = Port::INPUT;
		info->PortInfo::portId = portId;
		info->PortInfo::name = name;
		inputInfos[portId] = info;
		return info;
	}

	template <class TPortInfo = PortInfo>
	TPortInfo* configOutput(int portId, std::string name = "") {
		assert(portId >= 0 && (size_t) portId < outputs.size() && (size_t) portId < outputInfos.size());
		if (outputInfos[portId])
			delete outputInfos[portId];
		TPortInfo* info = new TPortInfo;
		info->PortInfo::module = this;
		info->PortInfo::type = Port::OUTPUT;
		info->PortInfo::portId = portId;
		info->PortInfo::name = name;
		outputInfos[portId] = info;
		return info;
	}

	template <class TLightInfo = LightInfo>
	TLightInfo* configLight(int lightId, std::string name = "") {
		assert(lightId >= 0 && (size_t) lightId < lights.size() && (size_t) lightId < lightInfos.size());
		if (lightInfos[lightId])
			delete lightInfos[lightId];
		TLightInfo* info = new TLightInfo;
		info->LightInfo::module = this;
		info->LightInfo::lightId = lightId;
		info->LightInfo::name = name;
		lightInfos[lightId] = info;
		return info;
	}

	void configBypass(int inputId, int outputId) {
		BypassRoute br;
		br.inputId = inputId;
		br.outputId = outputId;
		bypassRoutes.push_back(br);
	}

	std::string createPatchStorageDirectory();
	std::string getPatchStorageDirectory();

	plugin::Model* getModel() { return model; }
	int64_t getId() { return id; }
	int getNumParams() { return (int) params.size(); }
	Param& getParam(int index) { return params[index]; }
	int getNumInputs() { return (int) inputs.size(); }
	Input& getInput(int index) { return inputs[index]; }
	int getNumOutputs() { return (int) outputs.size(); }
	Output& getOutput(int index) { return outputs[index]; }
	int getNumLights() { return (int) lights.size(); }
	Light& getLight(int index) { return lights[index]; }
	ParamQuantity* getParamQuantity(int index) { return paramQuantities[index]; }
	PortInfo* getInputInfo(int index) { return inputInfos[index]; }
	PortInfo* getOutputInfo(int index) { return outputInfos[index]; }
	LightInfo* getLightInfo(int index) { return lightInfos[index]; }
	Expander& getLeftExpander() { return leftExpander; }
	Expander& getRightExpander() { return rightExpander; }
	Expander& getExpander(uint8_t side) { return side ? rightExpander : leftExpander; }

	struct ProcessArgs {
		float sampleRate;
		float sampleTime;
		int64_t frame;
	};
	/// One sample.
	virtual void process(const ProcessArgs& args) { step(); }
	virtual void step() {}
	virtual void processBypass(const ProcessArgs& args);

	virtual json_t* toJson();
	virtual void fromJson(json_t* rootJ);
	virtual json_t* paramsToJson();
	virtual void paramsFromJson(json_t* rootJ);
	/// Its settings beyond the knobs, kept with a Rack patch.
	virtual json_t* dataToJson() { return nullptr; }
	virtual void dataFromJson(json_t* rootJ) {}

	struct AddEvent {};
	virtual void onAdd(const AddEvent& e) { onAdd(); }
	struct RemoveEvent {};
	virtual void onRemove(const RemoveEvent& e) { onRemove(); }
	struct BypassEvent {};
	virtual void onBypass(const BypassEvent& e) {}
	struct UnBypassEvent {};
	virtual void onUnBypass(const UnBypassEvent& e) {}
	struct PortChangeEvent {
		bool connecting;
		Port::Type type;
		int portId;
	};
	virtual void onPortChange(const PortChangeEvent& e) {}
	struct SampleRateChangeEvent {
		float sampleRate;
		float sampleTime;
	};
	virtual void onSampleRateChange(const SampleRateChangeEvent& e) { onSampleRateChange(); }
	struct ExpanderChangeEvent {
		uint8_t side;
	};
	virtual void onExpanderChange(const ExpanderChangeEvent& e) {}
	struct ResetEvent {};
	virtual void onReset(const ResetEvent& e);
	struct RandomizeEvent {};
	virtual void onRandomize(const RandomizeEvent& e);
	struct SaveEvent {};
	virtual void onSave(const SaveEvent& e) {}
	struct SetMasterEvent {};
	virtual void onSetMaster(const SetMasterEvent& e) {}
	struct UnsetMasterEvent {};
	virtual void onUnsetMaster(const UnsetMasterEvent& e) {}

	virtual void onAdd() {}
	virtual void onRemove() {}
	virtual void onReset() {}
	virtual void onRandomize() {}
	virtual void onSampleRateChange() {}

	bool isBypassed() { return false; }
	void setBypassed(bool) {}
	DEPRECATED void bypass(bool) {}
};

/// A knob of another module, mapped by this one (a MIDI map's): here a
/// module plays alone, so a handle maps nothing, but it's kept.
struct ParamHandle {
	int64_t moduleId = -1;
	int paramId = 0;
	Module* module = nullptr;
	std::string text;
	NVGcolor color = nvgRGBA(0, 0, 0, 0);
};

/// A cable, as Rack's rack has them. There are none to look at here.
struct Cable {
	int64_t id = -1;
	Module* inputModule = nullptr;
	int inputId = -1;
	Module* outputModule = nullptr;
	int outputId = -1;
	json_t* toJson();
	/// (there are no other modules here to connect it to)
	void fromJson(json_t* rootJ) {}
};

/// The engine the modules run in, as far as a module asks about it.
struct Engine {
	float sampleRate = 44100.f;
	int64_t frame = 0;

	float getSampleRate() { return sampleRate; }
	float getSampleTime() { return 1.f / sampleRate; }
	int64_t getFrame() { return frame; }
	int64_t getBlock() { return frame; }
	int64_t getBlockFrame() { return frame; }
	int getBlockFrames() { return 1; }
	double getBlockTime() { return 0.0; }
	double getBlockDuration() { return 1.0 / sampleRate; }
	double getMeterAverage() { return 0.0; }
	double getMeterMax() { return 0.0; }
	bool isPaused() { return false; }
	Module* getModule(int64_t moduleId) { return nullptr; }
	Module* getMasterModule() { return nullptr; }
	std::vector<int64_t> getModuleIds() { return {}; }
	size_t getNumModules() { return 0; }
	float getParamValue(Module* module, int paramId) { return module->params[paramId].getValue(); }
	void setParamValue(Module* module, int paramId, float value) { module->params[paramId].setValue(value); }
	float getParamSmoothValue(Module* module, int paramId) { return module->params[paramId].getValue(); }
	void setParamSmoothValue(Module* module, int paramId, float value) { module->params[paramId].setValue(value); }
	void yieldWorkers() {}
	/// A module can't add modules to a patch here (a Rack plugin's menu
	/// that makes its expander, say): what's added is kept, and plays
	/// nowhere.
	void addModule(Module* module) { added.emplace_back(module); }
	std::vector<std::unique_ptr<Module>> added;

	// The rest of the engine's interface. A module plays alone here: there
	// are no other modules to find, and cables and mapped knobs are kept as
	// they're given, connecting nothing.
	void clear() {}
	void stepBlock(int frames) {}
	void setMasterModule(Module* module) {}
	void setMasterModule_NoLock(Module* module) {}
	void setSuggestedSampleRate(float suggestedSampleRate) {}
	size_t getModuleIds(int64_t* moduleIds, size_t len) { return 0; }
	void removeModule(Module* module) {}
	bool hasModule(Module* module) { return false; }
	Module* getModule_NoLock(int64_t moduleId) { return nullptr; }
	void resetModule(Module* module);
	void randomizeModule(Module* module);
	void bypassModule(Module* module, bool bypassed) {}
	json_t* moduleToJson(Module* module);
	void moduleFromJson(Module* module, json_t* rootJ);
	void prepareSaveModule(Module* module) {}
	void prepareSave() {}

	std::vector<Cable*> cables;
	size_t getNumCables() { return cables.size(); }
	size_t getCableIds(int64_t* cableIds, size_t len);
	std::vector<int64_t> getCableIds();
	void addCable(Cable* cable) { cables.push_back(cable); }
	void removeCable(Cable* cable) { cables.erase(std::remove(cables.begin(), cables.end(), cable), cables.end()); }
	bool hasCable(Cable* cable) { return std::find(cables.begin(), cables.end(), cable) != cables.end(); }
	Cable* getCable(int64_t cableId);

	std::vector<ParamHandle*> paramHandles;
	void addParamHandle(ParamHandle* paramHandle) { paramHandles.push_back(paramHandle); }
	void removeParamHandle(ParamHandle* paramHandle) {
		paramHandles.erase(std::remove(paramHandles.begin(), paramHandles.end(), paramHandle), paramHandles.end());
	}
	ParamHandle* getParamHandle(int64_t moduleId, int paramId);
	ParamHandle* getParamHandle_NoLock(int64_t moduleId, int paramId) { return getParamHandle(moduleId, paramId); }
	DEPRECATED ParamHandle* getParamHandle(Module* module, int paramId);
	void updateParamHandle(ParamHandle* paramHandle, int64_t moduleId, int paramId, bool overwrite = true);
	void updateParamHandle_NoLock(ParamHandle* paramHandle, int64_t moduleId, int paramId, bool overwrite = true) {
		updateParamHandle(paramHandle, moduleId, paramId, overwrite);
	}
	json_t* toJson();
	void fromJson(json_t* rootJ) {}
};

inline void Engine::resetModule(Module* module) {
	if (!module)
		return;
	for (size_t i = 0; i < module->params.size(); i++) {
		ParamQuantity* pq = module->getParamQuantity((int) i);
		if (pq && pq->resetEnabled)
			pq->reset();
	}
	Module::ResetEvent e;
	module->onReset(e);
}
inline void Engine::randomizeModule(Module* module) {
	if (!module)
		return;
	for (size_t i = 0; i < module->params.size(); i++) {
		ParamQuantity* pq = module->getParamQuantity((int) i);
		if (pq && pq->randomizeEnabled)
			pq->randomize();
	}
	Module::RandomizeEvent e;
	module->onRandomize(e);
}
inline json_t* Engine::moduleToJson(Module* module) { return module ? module->toJson() : nullptr; }
inline void Engine::moduleFromJson(Module* module, json_t* rootJ) {
	if (module)
		module->fromJson(rootJ);
}
inline size_t Engine::getCableIds(int64_t* cableIds, size_t len) {
	size_t n = 0;
	for (Cable* c : cables)
		if (n < len)
			cableIds[n++] = c->id;
	return n;
}
inline std::vector<int64_t> Engine::getCableIds() {
	std::vector<int64_t> ids;
	for (Cable* c : cables)
		ids.push_back(c->id);
	return ids;
}
inline Cable* Engine::getCable(int64_t cableId) {
	for (Cable* c : cables)
		if (c->id == cableId)
			return c;
	return nullptr;
}
inline ParamHandle* Engine::getParamHandle(int64_t moduleId, int paramId) {
	for (ParamHandle* h : paramHandles)
		if (h->moduleId == moduleId && h->paramId == paramId)
			return h;
	return nullptr;
}
inline ParamHandle* Engine::getParamHandle(Module* module, int paramId) {
	return module ? getParamHandle(module->id, paramId) : nullptr;
}
inline void Engine::updateParamHandle(ParamHandle* paramHandle, int64_t moduleId, int paramId, bool overwrite) {
	paramHandle->moduleId = moduleId;
	paramHandle->paramId = paramId;
	paramHandle->module = nullptr;
}
inline json_t* Engine::toJson() { return json_object(); }

inline Param* ParamQuantity::getParam() {
	if (!module || paramId < 0 || (size_t) paramId >= module->params.size())
		return nullptr;
	return &module->params[paramId];
}

} // namespace engine
} // namespace rack
