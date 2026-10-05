// `rack::midi`, `rack::audio` and the MIDI helpers of `rack::dsp`
// (MidiGenerator, MidiParser): a Rack module's MIDI and audio ports. The
// SDK's own code, to the same interface as VCV Rack 2's
// (docs/rack-modules.md).
//
// MIDI in comes from the plugin: there's one driver, Oroboro Modular, with
// one input device, the MIDI the plugin gives the module's instance
// (`oroboro_module_midi`: in the voice area its voice's notes, in the FX
// area all of it). Every MIDI input a module makes as it's made is on it,
// by the channel it's set to. MIDI out and audio reach no devices: an
// output sends nowhere, an audio port has no device.
#pragma once

#include <jansson.h>

#include "rack_core.hpp"
#include "rack_dsp.hpp"

namespace rack {

struct Context;

// ---- MIDI ----------------------------------------------------------------------------------
namespace midi {

/// A MIDI message: its bytes (status and channel first), and the engine
/// frame it's for.
struct Message {
	std::vector<uint8_t> bytes;
	int64_t frame = -1;

	Message() : bytes(3) {}
	int getSize() const { return (int) bytes.size(); }
	void setSize(int size) { bytes.resize(size); }
	uint8_t getChannel() const { return bytes.empty() ? 0 : bytes[0] & 0xf; }
	void setChannel(uint8_t channel) {
		if (!bytes.empty())
			bytes[0] = (bytes[0] & 0xf0) | (channel & 0xf);
	}
	uint8_t getStatus() const { return bytes.empty() ? 0 : bytes[0] >> 4; }
	void setStatus(uint8_t status) {
		if (!bytes.empty())
			bytes[0] = (bytes[0] & 0xf) | (status << 4);
	}
	uint8_t getNote() const { return bytes.size() > 1 ? bytes[1] : 0; }
	void setNote(uint8_t note) {
		if (bytes.size() > 1)
			bytes[1] = note & 0x7f;
	}
	uint8_t getValue() const { return bytes.size() > 2 ? bytes[2] : 0; }
	void setValue(uint8_t value) {
		if (bytes.size() > 2)
			bytes[2] = value & 0x7f;
	}
	/// Its bytes in hex: "90 3c 64".
	std::string toString() const {
		std::string s;
		for (size_t i = 0; i < bytes.size(); i++)
			s += string::f(i == 0 ? "%02x" : " %02x", bytes[i]);
		return s;
	}
	int64_t getFrame() const { return frame; }
	void setFrame(int64_t frame) { this->frame = frame; }
};

struct InputDevice;
struct Input;
struct OutputDevice;
struct Output;

struct Driver {
	virtual ~Driver() {}
	virtual std::string getName() { return ""; }
	virtual std::vector<int> getInputDeviceIds() { return {}; }
	virtual int getDefaultInputDeviceId() { return -1; }
	virtual std::string getInputDeviceName(int deviceId) { return ""; }
	virtual InputDevice* subscribeInput(int deviceId, Input* input) { return nullptr; }
	virtual void unsubscribeInput(int deviceId, Input* input) {}
	virtual std::vector<int> getOutputDeviceIds() { return {}; }
	virtual int getDefaultOutputDeviceId() { return -1; }
	virtual std::string getOutputDeviceName(int deviceId) { return ""; }
	virtual OutputDevice* subscribeOutput(int deviceId, Output* output) { return nullptr; }
	virtual void unsubscribeOutput(int deviceId, Output* output) {}
};

struct Device {
	virtual ~Device() {}
	virtual std::string getName() { return ""; }
};

struct InputDevice : Device {
	std::set<Input*> subscribed;
	void subscribe(Input* input) { subscribed.insert(input); }
	void unsubscribe(Input* input) { subscribed.erase(input); }
	void onMessage(const Message& message);
};

struct OutputDevice : Device {
	std::set<Output*> subscribed;
	void subscribe(Output* output) { subscribed.insert(output); }
	void unsubscribe(Output* output) { subscribed.erase(output); }
	virtual void sendMessage(const Message& message) {}
};

/// The one driver: the plugin's MIDI, which has one input device (its
/// instance's own, so its id is the same for every instance).
static const int OROBORO_DRIVER_ID = 1001;
struct OroboroDriver : Driver {
	std::string getName() override { return "Oroboro Modular"; }
	std::vector<int> getInputDeviceIds() override { return {0}; }
	int getDefaultInputDeviceId() override { return 0; }
	std::string getInputDeviceName(int deviceId) override { return deviceId == 0 ? "Oroboro Modular" : ""; }
};
Driver* oroboroDriver();
/// The input device of the instance being made (the bridge sets it while
/// the module's constructor runs): the MIDI inputs made then are on it.
InputDevice*& oroboroMaking();

inline void addDriver(int driverId, Driver* driver) { delete driver; }
inline std::vector<int> getDriverIds() { return {OROBORO_DRIVER_ID}; }
inline Driver* getDriver(int driverId) { return driverId == OROBORO_DRIVER_ID ? oroboroDriver() : nullptr; }

/// A MIDI port: which driver, device and channel it's set to (none are
/// there to set it to here; what's set is kept with the module).
struct Port {
	int channel = -1;
	int driverId = -1;
	int deviceId = -1;
	Driver* driver = nullptr;
	Device* device = nullptr;
	Context* context = nullptr;

	Port() {}
	virtual ~Port() {}
	Driver* getDriver() { return driver; }
	int getDriverId() { return driverId; }
	void setDriverId(int driverId) { this->driverId = driverId; }
	Device* getDevice() { return device; }
	virtual std::vector<int> getDeviceIds() = 0;
	virtual int getDefaultDeviceId() = 0;
	int getDeviceId() { return deviceId; }
	virtual void setDeviceId(int deviceId) = 0;
	virtual std::string getDeviceName(int deviceId) = 0;
	virtual std::vector<int> getChannels() = 0;
	int getChannel() { return channel; }
	void setChannel(int channel) { this->channel = channel; }
	std::string getChannelName(int channel) { return channel < 0 ? "All channels" : string::f("Channel %d", channel + 1); }
	json_t* toJson() {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "driver", json_integer(driverId));
		json_object_set_new(rootJ, "channel", json_integer(channel));
		return rootJ;
	}
	/// Its channel as it was kept (the driver and device a Rack patch kept
	/// were Rack's: an input stays on the plugin's MIDI).
	void fromJson(json_t* rootJ) {
		if (json_t* channelJ = json_object_get(rootJ, "channel"))
			channel = (int) json_integer_value(channelJ);
	}
};

struct Input : Port {
	InputDevice* inputDevice = nullptr;
	/// Its instance's device, which it's on unless it's set to none.
	InputDevice* oroboroDevice = nullptr;

	Input() {
		oroboroDevice = oroboroMaking();
		if (oroboroDevice) {
			driverId = OROBORO_DRIVER_ID;
			driver = oroboroDriver();
			setDeviceId(0);
		}
	}
	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;
	~Input() { setDeviceId(-1); }
	void reset() { channel = -1; }
	std::vector<int> getDeviceIds() override { return oroboroDevice ? std::vector<int>{0} : std::vector<int>{}; }
	int getDefaultDeviceId() override { return oroboroDevice ? 0 : -1; }
	/// 0: the plugin's MIDI; -1: none.
	void setDeviceId(int deviceId) override {
		if (inputDevice)
			inputDevice->unsubscribe(this);
		inputDevice = nullptr;
		device = nullptr;
		this->deviceId = -1;
		if (deviceId == 0 && oroboroDevice) {
			inputDevice = oroboroDevice;
			device = inputDevice;
			inputDevice->subscribe(this);
			this->deviceId = 0;
		}
	}
	std::string getDeviceName(int deviceId) override { return deviceId == 0 && oroboroDevice ? "Oroboro Modular" : ""; }
	/// All channels (-1), or one of 16.
	std::vector<int> getChannels() override {
		std::vector<int> channels;
		for (int c = -1; c < 16; c++)
			channels.push_back(c);
		return channels;
	}
	virtual void onMessage(const Message& message) {}
};

inline void InputDevice::onMessage(const Message& message) {
	for (Input* input : subscribed)
		if (input->channel < 0 || message.getStatus() == 0xf || input->channel == message.getChannel())
			input->onMessage(message);
}

/// The messages that came, to take in the engine's time: none come here.
struct InputQueue : Input {
	std::deque<Message> queue;
	std::mutex mutex;

	void onMessage(const Message& message) override {
		std::lock_guard<std::mutex> lock(mutex);
		if (queue.size() < 8192)
			queue.push_back(message);
	}
	/// The oldest message for `maxFrame` or before, if there is one.
	bool tryPop(Message* messageOut, int64_t maxFrame) {
		std::lock_guard<std::mutex> lock(mutex);
		if (queue.empty() || queue.front().frame > maxFrame)
			return false;
		*messageOut = queue.front();
		queue.pop_front();
		return true;
	}
	size_t size() {
		std::lock_guard<std::mutex> lock(mutex);
		return queue.size();
	}
};

struct Output : Port {
	OutputDevice* outputDevice = nullptr;
	Output() { channel = 0; }
	void reset() { channel = 0; }
	std::vector<int> getDeviceIds() override { return {}; }
	int getDefaultDeviceId() override { return -1; }
	void setDeviceId(int deviceId) override { this->deviceId = deviceId; }
	std::string getDeviceName(int deviceId) override { return ""; }
	std::vector<int> getChannels() override {
		std::vector<int> channels;
		for (int c = 0; c < 16; c++)
			channels.push_back(c);
		return channels;
	}
	void sendMessage(const Message& message) {
		if (outputDevice)
			outputDevice->sendMessage(message);
	}
};

} // namespace midi

namespace midiloopback {
struct Device;
struct Context {
	std::vector<Device*> devices;
};
} // namespace midiloopback

// ---- audio ---------------------------------------------------------------------------------
namespace audio {

struct Device;
struct Port;

struct Driver {
	virtual ~Driver() {}
	virtual std::string getName() { return ""; }
	virtual std::vector<int> getDeviceIds() { return {}; }
	virtual int getDefaultDeviceId() { return -1; }
	virtual std::string getDeviceName(int deviceId) { return ""; }
	virtual int getDeviceNumInputs(int deviceId) { return 0; }
	virtual int getDeviceNumOutputs(int deviceId) { return 0; }
	virtual Device* subscribe(int deviceId, Port* port) { return nullptr; }
	virtual void unsubscribe(int deviceId, Port* port) {}
};

struct Device {
	std::set<Port*> subscribed;
	std::mutex processMutex;
	virtual ~Device() {}
	virtual std::string getName() { return ""; }
	virtual int getNumInputs() { return 0; }
	virtual int getNumOutputs() { return 0; }
	virtual std::set<float> getSampleRates() { return {}; }
	virtual float getSampleRate() { return 0.f; }
	virtual void setSampleRate(float sampleRate) {}
	virtual std::set<int> getBlockSizes() { return {}; }
	virtual int getBlockSize() { return 0; }
	virtual void setBlockSize(int blockSize) {}
	virtual void subscribe(Port* port) { subscribed.insert(port); }
	virtual void unsubscribe(Port* port) { subscribed.erase(port); }
	void processBuffer(const float* input, int inputStride, float* output, int outputStride, int frames);
	void onStartStream();
	void onStopStream();
};

inline void addDriver(int driverId, Driver* driver) { delete driver; }
inline std::vector<int> getDriverIds() { return {}; }
inline Driver* getDriver(int driverId) { return nullptr; }

/// An audio port: which driver and device it's set to (none here).
struct Port {
	int inputOffset = 0;
	int outputOffset = 0;
	int maxInputs = 8;
	int maxOutputs = 8;
	int driverId = -1;
	int deviceId = -1;
	Driver* driver = nullptr;
	Device* device = nullptr;
	Context* context = nullptr;

	Port() {}
	virtual ~Port() {}
	void reset() {
		driverId = deviceId = -1;
		driver = nullptr;
		device = nullptr;
	}
	Driver* getDriver() { return driver; }
	int getDriverId() { return driverId; }
	void setDriverId(int driverId) { this->driverId = driverId; }
	std::string getDriverName() { return ""; }
	Device* getDevice() { return device; }
	std::vector<int> getDeviceIds() { return {}; }
	int getDeviceId() { return deviceId; }
	void setDeviceId(int deviceId) { this->deviceId = deviceId; }
	int getDeviceNumInputs(int deviceId) { return 0; }
	int getDeviceNumOutputs(int deviceId) { return 0; }
	std::string getDeviceName(int deviceId) { return ""; }
	std::string getDeviceDetail(int deviceId, int offset) { return ""; }
	std::set<float> getSampleRates() { return device ? device->getSampleRates() : std::set<float>(); }
	float getSampleRate() { return device ? device->getSampleRate() : 0.f; }
	void setSampleRate(float sampleRate) {
		if (device)
			device->setSampleRate(sampleRate);
	}
	std::set<int> getBlockSizes() { return device ? device->getBlockSizes() : std::set<int>(); }
	int getBlockSize() { return device ? device->getBlockSize() : 0; }
	void setBlockSize(int blockSize) {
		if (device)
			device->setBlockSize(blockSize);
	}
	int getNumInputs() { return device ? std::min(device->getNumInputs() - inputOffset, maxInputs) : 0; }
	int getNumOutputs() { return device ? std::min(device->getNumOutputs() - outputOffset, maxOutputs) : 0; }
	json_t* toJson() {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "driver", json_integer(driverId));
		return rootJ;
	}
	void fromJson(json_t* rootJ) {
		if (json_t* driverJ = json_object_get(rootJ, "driver"))
			driverId = (int) json_integer_value(driverJ);
	}
	virtual void processBuffer(const float* input, int inputStride, float* output, int outputStride, int frames) {}
	virtual void processInput(const float* input, int inputStride, int frames) {}
	virtual void processOutput(float* output, int outputStride, int frames) {}
	virtual void onStartStream() {}
	virtual void onStopStream() {}
};

inline void Device::processBuffer(const float* input, int inputStride, float* output, int outputStride, int frames) {
	for (Port* port : subscribed)
		port->processBuffer(input, inputStride, output, outputStride, frames);
}
inline void Device::onStartStream() {
	for (Port* port : subscribed)
		port->onStartStream();
}
inline void Device::onStopStream() {
	for (Port* port : subscribed)
		port->onStopStream();
}

} // namespace audio

// ---- MIDI in dsp ---------------------------------------------------------------------------
namespace dsp {

/// Turns gates, notes and controller values into MIDI messages, each to
/// `onMessage`, as they change. CHANNELS: its polyphony.
template <int CHANNELS>
struct MidiGenerator {
	int8_t vels[CHANNELS];
	int8_t notes[CHANNELS];
	bool gates[CHANNELS];
	int8_t keyPressures[CHANNELS];
	int8_t channelPressure;
	int8_t ccs[128];
	int16_t pw;
	bool clk;
	bool start;
	bool stop;
	bool cont;
	int64_t frame = -1;

	MidiGenerator() { reset(); }
	virtual ~MidiGenerator() {}
	void reset() {
		for (int c = 0; c < CHANNELS; c++) {
			vels[c] = 100;
			notes[c] = 60;
			gates[c] = false;
			keyPressures[c] = -1;
		}
		channelPressure = -1;
		for (int i = 0; i < 128; i++)
			ccs[i] = -1;
		pw = 0x2000;
		clk = start = stop = cont = false;
	}
	/// Every note off, and back to the start.
	void panic() {
		reset();
		for (int note = 0; note <= 127; note++)
			send(0x8, note, 0);
	}
	void setVelocity(int8_t vel, int c) { vels[c] = vel; }
	void setNoteGate(int8_t note, bool gate, int c) {
		bool changedNote = gate && gates[c] && note != notes[c];
		bool opened = gate && !gates[c];
		bool closed = !gate && gates[c];
		if (changedNote || closed)
			send(0x8, notes[c], vels[c]);
		if (changedNote || opened)
			send(0x9, note, vels[c]);
		notes[c] = note;
		gates[c] = gate;
	}
	void setKeyPressure(int8_t val, int c) {
		if (keyPressures[c] == val)
			return;
		keyPressures[c] = val;
		send(0xa, notes[c], val);
	}
	void setChannelPressure(int8_t val) {
		if (channelPressure == val)
			return;
		channelPressure = val;
		midi::Message m;
		m.setSize(2);
		m.setStatus(0xd);
		m.setNote(val);
		m.setFrame(frame);
		onMessage(m);
	}
	void setCc(int8_t cc, int id) {
		if (ccs[id] == cc)
			return;
		ccs[id] = cc;
		send(0xb, id, cc);
	}
	void setModWheel(int8_t cc) { setCc(cc, 0x01); }
	void setVolume(int8_t cc) { setCc(cc, 0x07); }
	void setBalance(int8_t cc) { setCc(cc, 0x08); }
	void setPan(int8_t cc) { setCc(cc, 0x0a); }
	void setSustainPedal(int8_t cc) { setCc(cc, 0x40); }
	void setPitchWheel(int16_t pw) {
		if (this->pw == pw)
			return;
		this->pw = pw;
		send(0xe, pw & 0x7f, (pw >> 7) & 0x7f);
	}
	void setClock(bool clk) { system(this->clk, clk, 0x8); }
	void setStart(bool start) { system(this->start, start, 0xa); }
	void setContinue(bool cont) { system(this->cont, cont, 0xb); }
	void setStop(bool stop) { system(this->stop, stop, 0xc); }
	void setFrame(int64_t frame) { this->frame = frame; }
	virtual void onMessage(const midi::Message& message) {}

private:
	void send(uint8_t status, int note, int value) {
		midi::Message m;
		m.setStatus(status);
		m.setNote((uint8_t) note);
		m.setValue((uint8_t) value);
		m.setFrame(frame);
		onMessage(m);
	}
	/// A system real-time message (0xf8, 0xfa …) when `now` turns true.
	void system(bool& was, bool now, uint8_t which) {
		if (was == now)
			return;
		was = now;
		if (!now)
			return;
		midi::Message m;
		m.setSize(1);
		m.setStatus(0xf);
		m.setChannel(which);
		m.setFrame(frame);
		onMessage(m);
	}
};

/// Turns MIDI messages into notes, gates, velocities, wheels and clock
/// pulses, over up to MAX_CHANNELS voices (as Rack's MIDI to CV does).
template <uint8_t MAX_CHANNELS>
struct MidiParser {
	float pwRange;
	bool smooth;
	uint32_t clockDivision;
	uint8_t channels;
	enum PolyMode { ROTATE_MODE, REUSE_MODE, RESET_MODE, MPE_MODE, NUM_POLY_MODES };
	PolyMode polyMode;
	int64_t clock;
	bool pedal;
	uint8_t notes[MAX_CHANNELS];
	bool gates[MAX_CHANNELS];
	uint8_t velocities[MAX_CHANNELS];
	uint8_t aftertouches[MAX_CHANNELS];
	std::vector<uint8_t> heldNotes;
	int8_t rotateIndex;
	int16_t pws[MAX_CHANNELS];
	uint8_t mods[MAX_CHANNELS];
	dsp::ExponentialFilter pwFilters[MAX_CHANNELS];
	dsp::ExponentialFilter modFilters[MAX_CHANNELS];
	dsp::PulseGenerator clockPulse;
	dsp::PulseGenerator clockDividerPulse;
	dsp::PulseGenerator retriggerPulses[MAX_CHANNELS];
	dsp::PulseGenerator startPulse;
	dsp::PulseGenerator stopPulse;
	dsp::PulseGenerator continuePulse;

	MidiParser() {
		heldNotes.reserve(128);
		reset();
	}
	void reset() {
		pwRange = 2.f;
		smooth = true;
		clockDivision = 24;
		channels = 1;
		polyMode = ROTATE_MODE;
		setFilterLambda(30.f);
		panic();
	}
	/// Every note off, the wheels centred.
	void panic() {
		for (uint8_t c = 0; c < MAX_CHANNELS; c++) {
			notes[c] = 60;
			gates[c] = false;
			velocities[c] = 0;
			aftertouches[c] = 0;
			pws[c] = 8192;
			mods[c] = 0;
			pwFilters[c].reset();
			modFilters[c].reset();
		}
		pedal = false;
		rotateIndex = -1;
		clock = 0;
		heldNotes.clear();
	}
	void processFilters(float deltaTime) {
		for (uint8_t c = 0; c < getWheelChannels(); c++) {
			pwFilters[c].process(deltaTime, getPw(c));
			modFilters[c].process(deltaTime, getMod(c));
		}
	}
	void processPulses(float deltaTime) {
		clockPulse.process(deltaTime);
		clockDividerPulse.process(deltaTime);
		startPulse.process(deltaTime);
		stopPulse.process(deltaTime);
		continuePulse.process(deltaTime);
		for (uint8_t c = 0; c < channels; c++)
			retriggerPulses[c].process(deltaTime);
	}
	void processMessage(const midi::Message& msg) {
		uint8_t channel = (polyMode == MPE_MODE) ? msg.getChannel() : 0;
		switch (msg.getStatus()) {
			case 0x8:
				releaseNote(msg.getNote());
				break;
			case 0x9:
				if (msg.getValue() > 0) {
					uint8_t c = (polyMode == MPE_MODE) ? (uint8_t) (msg.getChannel() % std::max<uint8_t>(channels, 1))
					                                   : assignChannel(msg.getNote());
					velocities[c] = msg.getValue();
					pressNote(msg.getNote(), c);
				} else {
					releaseNote(msg.getNote());
				}
				break;
			case 0xa:
				for (uint8_t c = 0; c < channels; c++)
					if (notes[c] == msg.getNote())
						aftertouches[c] = msg.getValue();
				break;
			case 0xb:
				processCC(msg);
				break;
			case 0xd:
				if (channel < MAX_CHANNELS)
					aftertouches[channel] = msg.getNote();
				break;
			case 0xe:
				if (channel < MAX_CHANNELS)
					pws[channel] = ((uint16_t) msg.getValue() << 7) | msg.getNote();
				break;
			case 0xf:
				processSystem(msg);
				break;
			default:
				break;
		}
	}
	void processCC(const midi::Message& msg) {
		uint8_t channel = (polyMode == MPE_MODE) ? msg.getChannel() : 0;
		switch (msg.getNote()) {
			case 0x01:
				if (channel < MAX_CHANNELS)
					mods[channel] = msg.getValue();
				break;
			case 0x40:
				if (msg.getValue() >= 64)
					pressPedal();
				else
					releasePedal();
				break;
			case 0x78:
			case 0x7b:
				panic();
				break;
			default:
				break;
		}
	}
	void processSystem(const midi::Message& msg) {
		switch (msg.getChannel()) {
			case 0x8:
				clockPulse.trigger(1e-3);
				if (clockDivision > 0 && clock % clockDivision == 0)
					clockDividerPulse.trigger(1e-3);
				clock++;
				break;
			case 0xa:
				startPulse.trigger(1e-3);
				clock = 0;
				break;
			case 0xb:
				continuePulse.trigger(1e-3);
				break;
			case 0xc:
				stopPulse.trigger(1e-3);
				break;
			default:
				break;
		}
	}
	/// The voice a new note goes to.
	uint8_t assignChannel(uint8_t note) {
		if (channels <= 1)
			return 0;
		switch (polyMode) {
			case REUSE_MODE:
				// the voice that last played this note, if any
				for (uint8_t c = 0; c < channels; c++)
					if (notes[c] == note)
						return c;
				// (else as rotate)
			case ROTATE_MODE:
				// the next voice without a gate, from the last one taken
				for (uint8_t i = 1; i <= channels; i++) {
					uint8_t c = (uint8_t) ((rotateIndex + i) % channels);
					if (!gates[c]) {
						rotateIndex = (int8_t) c;
						return c;
					}
				}
				rotateIndex = (int8_t) ((rotateIndex + 1) % channels);
				return (uint8_t) rotateIndex;
			case RESET_MODE:
				for (uint8_t c = 0; c < channels; c++)
					if (!gates[c])
						return c;
				return (uint8_t) (channels - 1);
			default:
				return 0;
		}
	}
	uint8_t pressNote(uint8_t note, uint8_t channel) {
		heldNotes.erase(std::remove(heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
		heldNotes.push_back(note);
		if (channel >= MAX_CHANNELS)
			channel = 0;
		notes[channel] = note;
		gates[channel] = true;
		retriggerPulses[channel].trigger(1e-3);
		return channel;
	}
	void releaseNote(uint8_t note) {
		heldNotes.erase(std::remove(heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
		if (pedal)
			return;
		for (uint8_t c = 0; c < channels; c++) {
			if (notes[c] == note)
				gates[c] = false;
		}
		// monophonic: back to the last note still held
		if (channels == 1 && !heldNotes.empty()) {
			notes[0] = heldNotes.back();
			gates[0] = true;
			retriggerPulses[0].trigger(1e-3);
		}
	}
	void pressPedal() { pedal = true; }
	void releasePedal() {
		pedal = false;
		for (uint8_t c = 0; c < channels; c++)
			if (std::find(heldNotes.begin(), heldNotes.end(), notes[c]) == heldNotes.end())
				gates[c] = false;
	}
	uint8_t getChannels() { return channels; }
	void setChannels(uint8_t channels) {
		if (channels == this->channels)
			return;
		this->channels = std::max<uint8_t>(1, std::min<uint8_t>(channels, MAX_CHANNELS));
		panic();
	}
	void setPolyMode(PolyMode polyMode) {
		if (polyMode == this->polyMode)
			return;
		this->polyMode = polyMode;
		panic();
	}
	/// A note's voltage, 1 V an octave, 0 V for C4, with the pitch wheel.
	float getPitchVoltage(uint8_t channel) {
		uint8_t wheel = (polyMode == MPE_MODE) ? channel : 0;
		float pw = smooth ? pwFilters[wheel].out : getPw(wheel);
		return (notes[channel] - 60.f + pw * pwRange) / 12.f;
	}
	void setFilterLambda(float lambda) {
		for (uint8_t c = 0; c < MAX_CHANNELS; c++) {
			pwFilters[c].setLambda(lambda);
			modFilters[c].setLambda(lambda);
		}
	}
	/// -1 to 1.
	float getPw(uint8_t channel) { return math::clamp((pws[channel] - 8192) / 8191.f, -1.f, 1.f); }
	/// 0 to 1.
	float getMod(uint8_t channel) { return mods[channel] / 127.f; }
	uint8_t getWheelChannels() { return (polyMode == MPE_MODE) ? MAX_CHANNELS : 1; }
	json_t* toJson() {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "pwRange", json_real(pwRange));
		json_object_set_new(rootJ, "smooth", json_boolean(smooth));
		json_object_set_new(rootJ, "channels", json_integer(channels));
		json_object_set_new(rootJ, "polyMode", json_integer(polyMode));
		json_object_set_new(rootJ, "clockDivision", json_integer(clockDivision));
		json_object_set_new(rootJ, "lastPitch", json_integer(pws[0]));
		json_object_set_new(rootJ, "lastMod", json_integer(mods[0]));
		return rootJ;
	}
	void fromJson(json_t* rootJ) {
		if (json_t* j = json_object_get(rootJ, "pwRange"))
			pwRange = (float) json_number_value(j);
		if (json_t* j = json_object_get(rootJ, "smooth"))
			smooth = json_boolean_value(j);
		if (json_t* j = json_object_get(rootJ, "channels"))
			setChannels((uint8_t) json_integer_value(j));
		if (json_t* j = json_object_get(rootJ, "polyMode"))
			polyMode = (PolyMode) json_integer_value(j);
		if (json_t* j = json_object_get(rootJ, "clockDivision"))
			clockDivision = (uint32_t) json_integer_value(j);
		if (json_t* j = json_object_get(rootJ, "lastPitch"))
			pws[0] = (int16_t) json_integer_value(j);
		if (json_t* j = json_object_get(rootJ, "lastMod"))
			mods[0] = (uint8_t) json_integer_value(j);
	}
};

} // namespace dsp

} // namespace rack
