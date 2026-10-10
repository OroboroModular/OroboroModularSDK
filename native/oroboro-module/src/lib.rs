//! Compiled modules for Oroboro Modular, in Rust.
//!
//! A compiled module is a native library (a `cdylib`) that exports the
//! module ABI, version 1 (`include/oroboro_module.h`). This crate does the
//! exporting: implement [`Module`] for a type, describe it with a [`Spec`],
//! and `export_module!(YourType);`. The plugin loads it from
//! `native/<Vendor>/` in its modules folder (signed by the Community
//! Library, or, in developer mode, your own unsigned build), and runs one
//! instance per voice, one sample per call. Its name says who makes it:
//! `native/Fold` by Acme is `native/Acme/Fold` to the plugin.
//!
//! ```ignore
//! use oroboro_module::{export_module, Module, Param, Spec};
//!
//! pub struct Fold { amount: f32 }
//!
//! impl Module for Fold {
//!     fn spec() -> Spec {
//!         Spec::new("native/Fold").vendor("Acme").category("shaper").input("In").output("Out")
//!             .param(Param::new("Amount", 1.0, 8.0, 2.0))
//!     }
//!     fn new(_sample_rate: f32) -> Self { Fold { amount: 2.0 } }
//!     fn set_param(&mut self, index: usize, value: f32) { if index == 0 { self.amount = value } }
//!     fn tick(&mut self, inputs: &[f32], outputs: &mut [f32]) {
//!         outputs[0] = 5.0 * (inputs[0] * self.amount / 5.0).sin();
//!     }
//! }
//!
//! export_module!(Fold);
//! ```
//!
//! Signals are volts, as Oroboro's own modules: audio about ±5 V, gates
//! 0/10 V, pitch 1 V an octave with 0 V = C4. Inputs named `V/Oct`, `Gate`
//! and `Vel` read the voice's key while no cable is in them: the plugin
//! does that before calling `tick`. A panic in a module is caught (a module
//! never unwinds into the host): that instance falls silent.
//!
//! What a module keeps beyond its knobs (a mode, a sequence, a table it was
//! taught) is its **state**: [`Module::state`] says it as text, the plugin
//! keeps it with the patch, and [`Module::set_state`] gets it back. Options
//! that aren't knobs go in the module's own **menu** ([`Module::menu`],
//! [`Module::choose`]), which the plugin shows in the module's right-click
//! menu.
//!
//! A module can have a **panel of its own** ([`Module::panel`]: where its
//! knobs, jacks and lights are; [`Module::art`]: its picture, an SVG) with a
//! **screen** on it ([`Module::screen`]: what it shows as it plays, drawn by
//! the plugin each frame, and the pointer there), and **lights**
//! ([`Module::lights`]). Export it with `export_module!(YourType, panel)`.
//!
//! [`faust`] runs Faust-generated Rust (`faust -lang rust -single`) as a
//! module, with the interface the plugin gives the same source as a script:
//! `export_faust!` (what `oromod faust build` writes).

use std::cell::UnsafeCell;
use std::ffi::{c_char, c_void, CStr, CString};
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::sync::{Arc, Mutex};

pub mod faust;
pub mod panel;

pub use panel::{Align, Color, Drawing, Panel, Pointer, Screen};

/// The module ABI this crate exports.
pub const ABI: u32 = 1;

/// How a knob runs between its ends.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Scaling {
    /// Evenly.
    Lin,
    /// Each stretch the same ratio (its minimum must be above 0): Hz, times.
    Exp,
    /// Whole steps: a switch or a selector.
    Enum,
}

impl Scaling {
    fn name(self) -> &'static str {
        match self {
            Scaling::Lin => "lin",
            Scaling::Exp => "exp",
            Scaling::Enum => "enum",
        }
    }
}

/// A knob: its name, range (in real units), default, unit and scaling.
#[derive(Clone, Debug, PartialEq)]
pub struct Param {
    pub name: String,
    pub min: f32,
    pub max: f32,
    pub default: f32,
    pub unit: String,
    pub scaling: Scaling,
    /// A selector's positions by name (`choice`); none: numbers.
    pub labels: Vec<String>,
}

impl Param {
    pub fn new(name: &str, min: f32, max: f32, default: f32) -> Param {
        Param { name: name.into(), min, max, default, unit: String::new(), scaling: Scaling::Lin, labels: Vec::new() }
    }

    /// A selector: one position per name, counted from 0, `default` the
    /// one it starts at. The plugin shows the names (as buttons, up to
    /// eight of them).
    pub fn choice(name: &str, labels: &[&str], default: usize) -> Param {
        let last = labels.len().saturating_sub(1);
        Param {
            name: name.into(),
            min: 0.0,
            max: last as f32,
            default: default.min(last) as f32,
            unit: String::new(),
            scaling: Scaling::Enum,
            labels: labels.iter().map(|l| l.to_string()).collect(),
        }
    }

    /// Shown after the value (`Hz`, `s`, `%`).
    pub fn unit(mut self, unit: &str) -> Param {
        self.unit = unit.into();
        self
    }

    /// Runs exponentially (its minimum above 0).
    pub fn exp(mut self) -> Param {
        self.scaling = Scaling::Exp;
        self
    }

    /// Whole steps from `min` to `max`.
    pub fn steps(mut self) -> Param {
        self.scaling = Scaling::Enum;
        self
    }
}

/// JSON's text for `s`.
fn json_text(s: &str) -> String {
    let mut out = String::from("\"");
    for c in s.chars() {
        match c {
            '"' => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            c if (c as u32) < 0x20 => out.push_str(&format!("\\u{:04x}", c as u32)),
            c => out.push(c),
        }
    }
    out.push('"');
    out
}

/// An entry of a module's own menu ([`Module::menu`]).
#[derive(Clone, Debug, PartialEq)]
pub enum MenuEntry {
    /// Something to choose: [`Module::choose`] gets its `id`. `checked`: it's
    /// the one in force; `right`: what shows after its name (a value).
    Item { id: u32, text: String, right: String, checked: bool, disabled: bool },
    /// An item that opens a menu of its own.
    Submenu { text: String, right: String, items: Vec<MenuEntry> },
    /// A value to set between two ends: `value` 0 to 1 is where it stands,
    /// `text` how it reads there ("Feedback: 40%"). [`Module::choose`] gets
    /// its `id` and the new place.
    Slider { id: u32, text: String, value: f32 },
    /// A heading.
    Label(String),
    /// A line between groups.
    Separator,
}

impl MenuEntry {
    /// An item to choose.
    pub fn item(id: u32, text: &str) -> MenuEntry {
        MenuEntry::Item { id, text: text.into(), right: String::new(), checked: false, disabled: false }
    }

    /// An item with a check mark while `checked`.
    pub fn check(id: u32, text: &str, checked: bool) -> MenuEntry {
        MenuEntry::Item { id, text: text.into(), right: String::new(), checked, disabled: false }
    }

    /// An item that opens `items`.
    pub fn submenu(text: &str, items: Vec<MenuEntry>) -> MenuEntry {
        MenuEntry::Submenu { text: text.into(), right: String::new(), items }
    }

    /// A choice of `labels` under `text`: the one at `current` is checked
    /// and named beside `text`; choosing label `k` gives `first_id + k`.
    pub fn choice(text: &str, labels: &[&str], current: usize, first_id: u32) -> MenuEntry {
        let items = labels.iter().enumerate().map(|(k, label)| MenuEntry::check(first_id + k as u32, label, k == current)).collect();
        MenuEntry::Submenu { text: text.into(), right: labels.get(current).copied().unwrap_or_default().into(), items }
    }

    pub fn slider(id: u32, text: &str, value: f32) -> MenuEntry {
        MenuEntry::Slider { id, text: text.into(), value }
    }

    pub fn label(text: &str) -> MenuEntry {
        MenuEntry::Label(text.into())
    }

    /// As `oroboro_module_menu` gives it.
    fn to_json(&self) -> String {
        match self {
            MenuEntry::Item { id, text, right, checked, disabled } => format!(
                "{{\"kind\": \"item\", \"id\": {id}, \"text\": {}, \"right\": {}, \"checked\": {checked}, \"disabled\": {disabled}}}",
                json_text(text),
                json_text(right)
            ),
            MenuEntry::Submenu { text, right, items } => format!(
                "{{\"kind\": \"item\", \"id\": 0, \"text\": {}, \"right\": {}, \"items\": {}}}",
                json_text(text),
                json_text(right),
                menu_json(items)
            ),
            MenuEntry::Slider { id, text, value } => {
                let value = if value.is_finite() { value.clamp(0.0, 1.0) } else { 0.0 };
                format!("{{\"kind\": \"slider\", \"id\": {id}, \"text\": {}, \"value\": {value}}}", json_text(text))
            }
            MenuEntry::Label(text) => format!("{{\"kind\": \"label\", \"text\": {}}}", json_text(text)),
            MenuEntry::Separator => "{\"kind\": \"separator\"}".into(),
        }
    }
}

/// A menu as `oroboro_module_menu` gives it.
pub fn menu_json(entries: &[MenuEntry]) -> String {
    format!("[{}]", entries.iter().map(MenuEntry::to_json).collect::<Vec<_>>().join(", "))
}

/// A module's interface: what the plugin shows and patches refer to.
#[derive(Clone, Debug, PartialEq)]
pub struct Spec {
    /// `native/<Name>` (a module of your own), or `faust/<Name>` for one
    /// built from a Faust source (with its `source_hash`). The plugin knows
    /// it by its maker's name too, `native/<Vendor>/<Name>`
    /// ([`Spec::full_name`]).
    pub module: String,
    /// Where Add module lists it: osc, filter, env, lfo, level, mixer, fx,
    /// shaper, … (else Custom).
    pub category: String,
    pub inputs: Vec<String>,
    pub outputs: Vec<String>,
    pub params: Vec<Param>,
    /// The Faust source it's built from (`faust::source_hash`, 16 hex digits).
    pub source_hash: Option<String>,
    /// Who makes it (your name, or your brand's): part of its name
    /// (`native/<Vendor>/<Name>`, so two makers' modules may share a name),
    /// and Add module lists compiled modules under Third Party Modules by
    /// it, then by category. A module needs one: the plugin doesn't load a
    /// module whose name doesn't say who makes it.
    pub vendor: String,
    /// It takes MIDI ([`Module::midi`]): the plugin gives it what it gets.
    pub midi: bool,
    /// What each input carries where its maker says ([`Spec::input_as`]),
    /// one per input (none: the plugin goes by its name).
    pub input_signals: Vec<Option<Signal>>,
    /// The same for the outputs ([`Spec::output_as`]).
    pub output_signals: Vec<Option<Signal>>,
}

/// What a jack carries, which is its colour in the plugin. The plugin tells
/// by a jack's name (Gate, Clock, Reset: logic; V/Oct, CV, Env: control;
/// the rest audio); a jack whose name doesn't tell says it with
/// [`Spec::input_as`] or [`Spec::output_as`].
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Signal {
    Audio,
    /// A slow voltage: a CV, an envelope, a pitch.
    Control,
    /// On or off: a gate, a trigger, a clock.
    Logic,
}

impl Signal {
    fn name(self) -> &'static str {
        match self {
            Signal::Audio => "audio",
            Signal::Control => "control",
            Signal::Logic => "logic",
        }
    }
}

impl Spec {
    pub fn new(module: &str) -> Spec {
        Spec {
            module: module.into(),
            category: "custom".into(),
            inputs: Vec::new(),
            outputs: Vec::new(),
            params: Vec::new(),
            source_hash: None,
            vendor: String::new(),
            midi: false,
            input_signals: Vec::new(),
            output_signals: Vec::new(),
        }
    }

    /// It takes MIDI: the plugin gives [`Module::midi`] its messages (in
    /// the voice area its voice's notes and every channel's other
    /// messages; in the FX area all of them).
    pub fn midi(mut self) -> Spec {
        self.midi = true;
        self
    }

    /// Who makes it: part of its name, and what Add module lists it under.
    pub fn vendor(mut self, vendor: &str) -> Spec {
        self.vendor = vendor.into();
        self
    }

    pub fn category(mut self, category: &str) -> Spec {
        self.category = category.into();
        self
    }

    pub fn input(mut self, name: &str) -> Spec {
        self.inputs.push(name.into());
        self.input_signals.resize(self.inputs.len(), None);
        self
    }

    pub fn output(mut self, name: &str) -> Spec {
        self.outputs.push(name.into());
        self.output_signals.resize(self.outputs.len(), None);
        self
    }

    /// An input that carries `signal`, whatever its name says ("Kick",
    /// a trigger).
    pub fn input_as(mut self, name: &str, signal: Signal) -> Spec {
        self = self.input(name);
        self.input_signals[self.inputs.len() - 1] = Some(signal);
        self
    }

    /// An output that carries `signal`, whatever its name says.
    pub fn output_as(mut self, name: &str, signal: Signal) -> Spec {
        self = self.output(name);
        self.output_signals[self.outputs.len() - 1] = Some(signal);
        self
    }

    pub fn param(mut self, param: Param) -> Spec {
        self.params.push(param);
        self
    }

    pub fn source_hash(mut self, hash: &str) -> Spec {
        self.source_hash = Some(hash.into());
        self
    }

    /// Its name as the plugin knows it: `native/<Vendor>/<Name>`, from
    /// `native/<Name>` or `faust/<Name>` and its vendor. One that names its
    /// maker already stays as it is, and so does one without a vendor (the
    /// plugin refuses that, and `oromod` says why).
    pub fn full_name(&self) -> String {
        let vendor = self.vendor.trim();
        let name = self.module.strip_prefix("native/").or_else(|| self.module.strip_prefix("faust/"));
        match name {
            Some(name) if !vendor.is_empty() && !name.contains('/') => format!("native/{vendor}/{name}"),
            _ => self.module.clone(),
        }
    }

    /// As `oroboro_module_spec` gives it.
    pub fn to_json(&self) -> String {
        let text = json_text;
        let number = |x: f32| if x.is_finite() { format!("{x}") } else { "0".into() };
        let list = |items: Vec<String>| format!("[{}]", items.join(", "));
        let said = |signals: &[Option<Signal>], i: usize| signals.get(i).copied().flatten().map(Signal::name);
        let inputs = list(
            self.inputs
                .iter()
                .enumerate()
                .map(|(i, n)| match said(&self.input_signals, i) {
                    Some(signal) => format!("{{\"name\": {}, \"signal\": \"{signal}\"}}", text(n)),
                    None => format!("{{\"name\": {}}}", text(n)),
                })
                .collect(),
        );
        // (a name alone, unless its maker says what it carries)
        let outputs = list(
            self.outputs
                .iter()
                .enumerate()
                .map(|(i, n)| match said(&self.output_signals, i) {
                    Some(signal) => format!("{{\"name\": {}, \"signal\": \"{signal}\"}}", text(n)),
                    None => text(n),
                })
                .collect(),
        );
        let params = list(
            self.params
                .iter()
                .map(|p| {
                    let labels = match p.labels.is_empty() {
                        true => String::new(),
                        false => format!(", \"labels\": {}", list(p.labels.iter().map(|l| text(l)).collect())),
                    };
                    format!(
                        "{{\"name\": {}, \"min\": {}, \"max\": {}, \"default\": {}, \"unit\": {}, \"scaling\": \"{}\"{labels}}}",
                        text(&p.name),
                        number(p.min),
                        number(p.max),
                        number(p.default),
                        text(&p.unit),
                        p.scaling.name()
                    )
                })
                .collect(),
        );
        let hash = self.source_hash.as_deref().map_or_else(|| "null".into(), text);
        let vendor = match self.vendor.trim().is_empty() {
            true => String::new(),
            false => format!(", \"vendor\": {}", text(self.vendor.trim())),
        };
        let midi = if self.midi { ", \"midi\": true" } else { "" };
        format!(
            "{{\"module\": {}, \"category\": {}, \"inputs\": {inputs}, \"outputs\": {outputs}, \"params\": {params}, \
             \"source_hash\": {hash}{vendor}{midi}}}",
            text(&self.full_name()),
            text(&self.category)
        )
    }
}

/// A compiled module: one instance per voice, one sample per `tick`.
pub trait Module: Send + 'static {
    /// Its interface. Asked once, when the plugin loads the library.
    fn spec() -> Spec
    where
        Self: Sized;

    /// An instance at `sample_rate`, its knobs at their defaults.
    fn new(sample_rate: f32) -> Self
    where
        Self: Sized;

    /// Knob `index` (the spec's order) is `value`, in its own units.
    fn set_param(&mut self, index: usize, value: f32);

    /// Forget the past: silence, as when a voice starts afresh.
    fn reset(&mut self) {}

    /// Which inputs have something to read (a cable, or the key for an
    /// unpatched V/Oct, Gate or Vel), one per input of the spec: told when
    /// the patch's cables change, for a module that does something else
    /// with an input nobody feeds. Until it's told, take every input as fed.
    fn connected(&mut self, _inputs: &[bool]) {}

    /// One MIDI message (its status byte and data bytes, 1 to 3), for a
    /// module whose spec says it takes MIDI ([`Spec::midi`]): on the audio
    /// thread, before the `tick` of the sample it falls on.
    fn midi(&mut self, _message: &[u8]) {}

    /// One sample: `inputs` as many as the spec's inputs, `outputs` as many
    /// as its outputs (write each).
    fn tick(&mut self, inputs: &[f32], outputs: &mut [f32]);

    /// What the module keeps beyond its knobs, as text (JSON by custom):
    /// the plugin keeps it with the patch. None: nothing to keep. Never
    /// asked on the audio thread.
    fn state(&self) -> Option<String> {
        None
    }

    /// The state a patch kept (`state` of an earlier time, maybe of an
    /// earlier version of the module: take what's understood), given to
    /// each instance before its first `tick`.
    fn set_state(&mut self, _state: &str) {}

    /// The module's own menu, as it stands (its check marks and values):
    /// the plugin shows it in the module's right-click menu. It's asked of
    /// an instance the plugin keeps for that and never plays, with the
    /// patch's knobs and state.
    fn menu(&self) -> Vec<MenuEntry> {
        Vec::new()
    }

    /// Entry `id` of the menu was chosen (`value`: where a slider was set,
    /// 0 to 1). Change what `state` says: the plugin asks for the state
    /// afterwards, keeps it, and gives it to the instances that play.
    fn choose(&mut self, _id: u32, _value: f32) {}

    /// Knob `index` as the instance has it, for a module whose menu moves
    /// a knob (the plugin then moves it in the patch). None: as it was set.
    fn param(&self, _index: usize) -> Option<f32> {
        None
    }

    /// Its own panel: its size, where its knobs, jacks and lights are
    /// (`export_module!(YourType, panel)` exports it). None: the plugin
    /// lays the module out its own way.
    fn panel() -> Option<Panel>
    where
        Self: Sized,
    {
        None
    }

    /// The picture behind the panel's controls: an SVG's text (the plugin
    /// draws it as big as it shows it, in its theme's colours, its text
    /// elements in its own letters) or a PNG's bytes.
    fn art() -> Option<&'static [u8]>
    where
        Self: Sized,
    {
        None
    }

    /// How bright its lights are, 0 to 1 each, into `out` (room for as
    /// many as asked); returns how many it has. On the audio thread,
    /// between ticks: copy numbers, nothing else.
    fn lights(&self, _out: &mut [f32]) -> usize {
        0
    }

    /// What its panel shows of it as it plays, and does with the pointer
    /// there: asked once, as the instance is made. Keep a handle to it and
    /// write into it what it's to show; the plugin draws it on its own
    /// thread while `tick` runs on the audio thread.
    fn screen(&self) -> Option<Arc<dyn Screen>> {
        None
    }
}

/// The ABI's functions over a `Module` (what `export_module!` exports).
#[doc(hidden)]
pub mod abi {
    use super::*;

    /// An instance as the plugin has it: the module (the audio thread's,
    /// and while it doesn't play, the plugin's for its menu and state), and
    /// apart from it its screen, which the window's thread draws while the
    /// audio thread plays the module.
    pub struct Instance<M> {
        screen: Option<Arc<dyn Screen>>,
        /// Its screen turns its knobs (`Screen::knob`): they're asked of
        /// the screen, never the module.
        screen_knobs: bool,
        /// The drawing last given out: the window's thread's, until its next.
        drawn: Mutex<Vec<f32>>,
        inner: UnsafeCell<Inner<M>>,
    }

    pub struct Inner<M> {
        module: M,
        inputs: usize,
        outputs: usize,
        /// It panicked once: silent from then on.
        broken: bool,
        /// The knobs as they were set.
        params: Vec<f32>,
        /// The text last given out (its state, its menu): the instance's
        /// to keep until the next.
        text: CString,
    }

    /// The module of instance `m`, for the thread that has it (the audio
    /// thread's, or the plugin's while it doesn't play).
    ///
    /// # Safety
    /// `m` came from `new::<M>`; one thread at a time has the module.
    unsafe fn inner<'a, M>(m: *mut c_void) -> &'a mut Inner<M> {
        &mut *(*m.cast::<Instance<M>>()).inner.get()
    }

    /// Instance `m`'s screen side, for the window's thread.
    ///
    /// # Safety
    /// `m` came from `new::<M>`.
    unsafe fn shared<'a, M>(m: *mut c_void) -> &'a Instance<M> {
        &*m.cast::<Instance<M>>()
    }

    pub fn spec_json<M: Module>() -> CString {
        let json = catch_unwind(|| M::spec().to_json()).unwrap_or_else(|_| "{}".into());
        CString::new(json.replace('\0', "")).unwrap_or_default()
    }

    pub fn new<M: Module>(sample_rate: f32) -> *mut c_void {
        catch_unwind(|| {
            let spec = M::spec();
            let module = M::new(sample_rate);
            let screen = module.screen();
            let inner = Inner {
                module,
                inputs: spec.inputs.len(),
                outputs: spec.outputs.len(),
                broken: false,
                params: spec.params.iter().map(|p| p.default).collect(),
                text: CString::default(),
            };
            let screen_knobs = screen.as_ref().is_some_and(|s| (0..spec.params.len()).any(|i| s.knob(i).is_some()));
            let instance =
                Instance { screen, screen_knobs, drawn: Mutex::new(Vec::new()), inner: UnsafeCell::new(inner) };
            Box::into_raw(Box::new(instance)).cast::<c_void>()
        })
        .unwrap_or(std::ptr::null_mut())
    }

    /// # Safety
    /// `m` came from `new::<M>` and isn't used again.
    pub unsafe fn free<M: Module>(m: *mut c_void) {
        if !m.is_null() {
            let _ = catch_unwind(AssertUnwindSafe(|| drop(Box::from_raw(m.cast::<Instance<M>>()))));
        }
    }

    /// # Safety
    /// `m` came from `new::<M>`.
    pub unsafe fn set_param<M: Module>(m: *mut c_void, index: u32, value: f32) {
        let instance = inner::<M>(m);
        if let Some(kept) = instance.params.get_mut(index as usize) {
            *kept = value;
        }
        if !instance.broken && catch_unwind(AssertUnwindSafe(|| instance.module.set_param(index as usize, value))).is_err() {
            instance.broken = true;
        }
    }

    /// # Safety
    /// `m` came from `new::<M>`.
    pub unsafe fn get_param<M: Module>(m: *mut c_void, index: u32) -> f32 {
        // (a screen that turns the knobs has them: asked of it, as the
        // module may be playing; NaN for one it doesn't know)
        let shared = shared::<M>(m);
        if let (true, Some(screen)) = (shared.screen_knobs, &shared.screen) {
            return catch_unwind(AssertUnwindSafe(|| screen.knob(index as usize))).ok().flatten().unwrap_or(f32::NAN);
        }
        let instance = inner::<M>(m);
        let set = instance.params.get(index as usize).copied().unwrap_or(0.0);
        if instance.broken {
            return set;
        }
        catch_unwind(AssertUnwindSafe(|| instance.module.param(index as usize))).ok().flatten().unwrap_or(set)
    }

    /// Keeps `text` in the instance and gives it as C text (NULL for none).
    fn give<M>(instance: &mut Inner<M>, text: Option<String>) -> *const c_char {
        match text.and_then(|t| CString::new(t.replace('\0', "")).ok()) {
            Some(text) => {
                instance.text = text;
                instance.text.as_ptr()
            }
            None => std::ptr::null(),
        }
    }

    /// # Safety
    /// `m` came from `new::<M>`.
    pub unsafe fn get_state<M: Module>(m: *mut c_void) -> *const c_char {
        let instance = inner::<M>(m);
        if instance.broken {
            return std::ptr::null();
        }
        let state = catch_unwind(AssertUnwindSafe(|| instance.module.state())).ok().flatten();
        give(instance, state)
    }

    /// # Safety
    /// `m` came from `new::<M>`; `state` is a NUL-terminated text.
    pub unsafe fn set_state<M: Module>(m: *mut c_void, state: *const c_char) {
        let instance = inner::<M>(m);
        if state.is_null() || instance.broken {
            return;
        }
        let state = CStr::from_ptr(state).to_string_lossy();
        if catch_unwind(AssertUnwindSafe(|| instance.module.set_state(&state))).is_err() {
            instance.broken = true;
        }
    }

    /// # Safety
    /// `m` came from `new::<M>`.
    pub unsafe fn menu<M: Module>(m: *mut c_void) -> *const c_char {
        let instance = inner::<M>(m);
        if instance.broken {
            return std::ptr::null();
        }
        let menu = catch_unwind(AssertUnwindSafe(|| instance.module.menu())).unwrap_or_default();
        let json = (!menu.is_empty()).then(|| menu_json(&menu));
        give(instance, json)
    }

    /// # Safety
    /// `m` came from `new::<M>`.
    pub unsafe fn menu_choose<M: Module>(m: *mut c_void, id: u32, value: f32) {
        let instance = inner::<M>(m);
        if !instance.broken && catch_unwind(AssertUnwindSafe(|| instance.module.choose(id, value))).is_err() {
            instance.broken = true;
        }
    }

    /// # Safety
    /// `m` came from `new::<M>`.
    pub unsafe fn reset<M: Module>(m: *mut c_void) {
        let instance = inner::<M>(m);
        if !instance.broken && catch_unwind(AssertUnwindSafe(|| instance.module.reset())).is_err() {
            instance.broken = true;
        }
    }

    /// # Safety
    /// `m` came from `new::<M>`; `inputs` has `count` bytes.
    pub unsafe fn connected<M: Module>(m: *mut c_void, inputs: *const u8, count: u32) {
        let instance = inner::<M>(m);
        let flags: &[u8] = if count == 0 { &[] } else { std::slice::from_raw_parts(inputs, count as usize) };
        // (one flag per input of the spec, whatever the host sent)
        let fed: Vec<bool> = (0..instance.inputs).map(|i| flags.get(i).is_some_and(|&f| f != 0)).collect();
        if !instance.broken && catch_unwind(AssertUnwindSafe(|| instance.module.connected(&fed))).is_err() {
            instance.broken = true;
        }
    }

    /// # Safety
    /// `m` came from `new::<M>`; `message` has `size` bytes.
    pub unsafe fn midi<M: Module>(m: *mut c_void, message: *const u8, size: u32) {
        let instance = inner::<M>(m);
        if message.is_null() || size == 0 || size > 3 {
            return;
        }
        let bytes = std::slice::from_raw_parts(message, size as usize);
        if !instance.broken && catch_unwind(AssertUnwindSafe(|| instance.module.midi(bytes))).is_err() {
            instance.broken = true;
        }
    }

    pub fn panel_json<M: Module>() -> CString {
        let json = catch_unwind(|| M::panel().map(|p| p.to_json())).ok().flatten().unwrap_or_default();
        CString::new(json.replace('\0', "")).unwrap_or_default()
    }

    pub fn art<M: Module>() -> &'static [u8] {
        catch_unwind(M::art).ok().flatten().unwrap_or(&[])
    }

    /// # Safety
    /// `m` came from `new::<M>`; `out` has room for `count`.
    pub unsafe fn lights<M: Module>(m: *mut c_void, out: *mut f32, count: u32) -> u32 {
        let instance = inner::<M>(m);
        if instance.broken {
            return 0;
        }
        let mut room = vec![0.0f32; count as usize];
        let n = catch_unwind(AssertUnwindSafe(|| instance.module.lights(&mut room))).unwrap_or(0);
        if count > 0 && !out.is_null() {
            std::ptr::copy_nonoverlapping(room.as_ptr(), out, (count as usize).min(n));
        }
        if count == 0 {
            // (asked how many: what it says for no room)
            return catch_unwind(AssertUnwindSafe(|| instance.module.lights(&mut []))).unwrap_or(0) as u32;
        }
        n as u32
    }

    /// # Safety
    /// `m` came from `new::<M>`; asked on the window's thread, one call at a time.
    pub unsafe fn draw<M: Module>(m: *mut c_void, count: *mut u32) -> *const f32 {
        let instance = shared::<M>(m);
        if !count.is_null() {
            *count = 0;
        }
        let Some(screen) = &instance.screen else { return std::ptr::null() };
        let mut d = Drawing::new();
        if catch_unwind(AssertUnwindSafe(|| screen.draw(&mut d))).is_err() || d.is_empty() {
            return std::ptr::null();
        }
        let Ok(mut drawn) = instance.drawn.lock() else { return std::ptr::null() };
        *drawn = d.numbers().to_vec();
        if !count.is_null() {
            *count = drawn.len() as u32;
        }
        drawn.as_ptr()
    }

    /// # Safety
    /// `m` came from `new::<M>`; asked on the window's thread.
    #[allow(clippy::too_many_arguments)]
    pub unsafe fn pointer<M: Module>(m: *mut c_void, kind: u32, x: f32, y: f32, dx: f32, dy: f32, button: u32, mods: u32) -> u32 {
        let instance = shared::<M>(m);
        let (Some(screen), Some(event)) = (&instance.screen, Pointer::of(kind, x, y, dx, dy, button, mods)) else { return 0 };
        u32::from(catch_unwind(AssertUnwindSafe(|| screen.pointer(event))).unwrap_or(false))
    }

    /// Whether instance `m`'s screen turns its knobs (`Screen::knob`): the
    /// plugin then reads them back after a gesture on it.
    ///
    /// # Safety
    /// `m` came from `new::<M>`.
    pub unsafe fn screen_knobs<M: Module>(m: *mut c_void) -> u32 {
        u32::from(shared::<M>(m).screen_knobs)
    }

    /// # Safety
    /// `m` came from `new::<M>`; `inputs` and `outputs` have as many values
    /// as the spec's inputs and outputs.
    pub unsafe fn tick<M: Module>(m: *mut c_void, inputs: *const f32, outputs: *mut f32) {
        let instance = inner::<M>(m);
        let ins: &[f32] = if instance.inputs == 0 { &[] } else { std::slice::from_raw_parts(inputs, instance.inputs) };
        let outs: &mut [f32] =
            if instance.outputs == 0 { &mut [] } else { std::slice::from_raw_parts_mut(outputs, instance.outputs) };
        if instance.broken || catch_unwind(AssertUnwindSafe(|| instance.module.tick(ins, outs))).is_err() {
            instance.broken = true;
            outs.fill(0.0);
        }
    }
}

/// Exports `$module` (a [`Module`]) as the library's module: the ABI's
/// functions, `oroboro_module_abi` to `oroboro_module_tick`. Once per
/// library. `export_module!(YourType, panel)` exports its own panel too:
/// its layout, its picture, its lights, what its screen draws and the
/// pointer there.
///
/// With this crate's `built-in` feature the functions aren't exported by
/// name: `oroboro_built_in()` gives them as a table ([`Exports`]), so that
/// several modules can be built into one program (Oroboro Modular builds
/// its own modules into itself so). The module's source is the same.
#[macro_export]
macro_rules! export_module {
    ($module:ty, panel) => {
        $crate::__export_base!($module);

        $crate::__abi_fns! {
            /// # Safety
            /// The ABI's.
            pub unsafe extern "C" fn oroboro_module_panel(_m: *mut ::std::ffi::c_void) -> *const ::std::ffi::c_char {
                static PANEL: ::std::sync::OnceLock<::std::ffi::CString> = ::std::sync::OnceLock::new();
                let panel = PANEL.get_or_init($crate::abi::panel_json::<$module>);
                if panel.as_bytes().is_empty() {
                    ::std::ptr::null()
                } else {
                    panel.as_ptr()
                }
            }

            /// # Safety
            /// The ABI's: `size` is written.
            pub unsafe extern "C" fn oroboro_module_art(size: *mut u32) -> *const u8 {
                let art = $crate::abi::art::<$module>();
                if !size.is_null() {
                    *size = art.len() as u32;
                }
                if art.is_empty() {
                    ::std::ptr::null()
                } else {
                    art.as_ptr()
                }
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`, room for `count`.
            pub unsafe extern "C" fn oroboro_module_lights(m: *mut ::std::ffi::c_void, out: *mut f32, count: u32) -> u32 {
                $crate::abi::lights::<$module>(m, out, count)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`.
            pub unsafe extern "C" fn oroboro_module_draw(m: *mut ::std::ffi::c_void, count: *mut u32) -> *const f32 {
                $crate::abi::draw::<$module>(m, count)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`.
            #[allow(clippy::too_many_arguments)]
            pub unsafe extern "C" fn oroboro_module_pointer(
                m: *mut ::std::ffi::c_void,
                kind: u32,
                x: f32,
                y: f32,
                dx: f32,
                dy: f32,
                button: u32,
                mods: u32,
            ) -> u32 {
                $crate::abi::pointer::<$module>(m, kind, x, y, dx, dy, button, mods)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`.
            pub unsafe extern "C" fn oroboro_module_screen_knobs(m: *mut ::std::ffi::c_void) -> u32 {
                $crate::abi::screen_knobs::<$module>(m)
            }
        }

        $crate::__built_in!(panel);
    };
    ($module:ty) => {
        $crate::__export_base!($module);
        $crate::__built_in!();
    };
}

/// The ABI's functions every module has (`export_module!`'s).
#[doc(hidden)]
#[macro_export]
macro_rules! __export_base {
    ($module:ty) => {
        $crate::__abi_fns! {
            pub extern "C" fn oroboro_module_abi() -> u32 {
                $crate::ABI
            }

            pub extern "C" fn oroboro_module_spec() -> *const ::std::ffi::c_char {
                static SPEC: ::std::sync::OnceLock<::std::ffi::CString> = ::std::sync::OnceLock::new();
                SPEC.get_or_init($crate::abi::spec_json::<$module>).as_ptr()
            }

            pub extern "C" fn oroboro_module_new(sample_rate: f32) -> *mut ::std::ffi::c_void {
                $crate::abi::new::<$module>(sample_rate)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`, not used again.
            pub unsafe extern "C" fn oroboro_module_free(m: *mut ::std::ffi::c_void) {
                $crate::abi::free::<$module>(m)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`.
            pub unsafe extern "C" fn oroboro_module_set_param(m: *mut ::std::ffi::c_void, index: u32, value: f32) {
                $crate::abi::set_param::<$module>(m, index, value)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`.
            pub unsafe extern "C" fn oroboro_module_reset(m: *mut ::std::ffi::c_void) {
                $crate::abi::reset::<$module>(m)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`, as many inputs and
            /// outputs as its spec has.
            pub unsafe extern "C" fn oroboro_module_tick(m: *mut ::std::ffi::c_void, inputs: *const f32, outputs: *mut f32) {
                $crate::abi::tick::<$module>(m, inputs, outputs)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`, `count` bytes.
            pub unsafe extern "C" fn oroboro_module_connected(m: *mut ::std::ffi::c_void, inputs: *const u8, count: u32) {
                $crate::abi::connected::<$module>(m, inputs, count)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`, `size` bytes.
            pub unsafe extern "C" fn oroboro_module_midi(m: *mut ::std::ffi::c_void, message: *const u8, size: u32) {
                $crate::abi::midi::<$module>(m, message, size)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`.
            pub unsafe extern "C" fn oroboro_module_get_param(m: *mut ::std::ffi::c_void, index: u32) -> f32 {
                $crate::abi::get_param::<$module>(m, index)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`.
            pub unsafe extern "C" fn oroboro_module_get_state(m: *mut ::std::ffi::c_void) -> *const ::std::ffi::c_char {
                $crate::abi::get_state::<$module>(m)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`, a NUL-terminated text.
            pub unsafe extern "C" fn oroboro_module_set_state(m: *mut ::std::ffi::c_void, state: *const ::std::ffi::c_char) {
                $crate::abi::set_state::<$module>(m, state)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`.
            pub unsafe extern "C" fn oroboro_module_menu(m: *mut ::std::ffi::c_void) -> *const ::std::ffi::c_char {
                $crate::abi::menu::<$module>(m)
            }

            /// # Safety
            /// The ABI's: a module from `oroboro_module_new`.
            pub unsafe extern "C" fn oroboro_module_menu_choose(m: *mut ::std::ffi::c_void, id: u32, value: f32) {
                $crate::abi::menu_choose::<$module>(m, id, value)
            }
        }
    };
}

/// The ABI's functions, exported by name from a library of its own.
#[cfg(not(feature = "built-in"))]
#[doc(hidden)]
#[macro_export]
macro_rules! __abi_fns {
    ($($item:item)*) => { $( #[no_mangle] $item )* };
}

/// The ABI's functions, built into a program with others: not by name
/// (`oroboro_built_in` gives them).
#[cfg(feature = "built-in")]
#[doc(hidden)]
#[macro_export]
macro_rules! __abi_fns {
    ($($item:item)*) => { $( $item )* };
}

/// Nothing more in a library of its own.
#[cfg(not(feature = "built-in"))]
#[doc(hidden)]
#[macro_export]
macro_rules! __built_in {
    ($($panel:tt)*) => {};
}

/// A built-in module's table of its ABI functions.
#[cfg(feature = "built-in")]
#[doc(hidden)]
#[macro_export]
macro_rules! __built_in {
    () => {
        /// This module's ABI functions, built in (`oroboro-module`'s `built-in` feature).
        pub fn oroboro_built_in() -> $crate::Exports {
            $crate::__exports!(None, None, None, None, None, None)
        }
    };
    (panel) => {
        /// This module's ABI functions, built in (`oroboro-module`'s `built-in` feature).
        pub fn oroboro_built_in() -> $crate::Exports {
            $crate::__exports!(
                Some(oroboro_module_panel),
                Some(oroboro_module_art),
                Some(oroboro_module_lights),
                Some(oroboro_module_draw),
                Some(oroboro_module_pointer),
                Some(oroboro_module_screen_knobs)
            )
        }
    };
}

#[cfg(feature = "built-in")]
#[doc(hidden)]
#[macro_export]
macro_rules! __exports {
    ($panel:expr, $art:expr, $lights:expr, $draw:expr, $pointer:expr, $screen_knobs:expr) => {
        $crate::Exports {
            abi: oroboro_module_abi,
            spec: oroboro_module_spec,
            new: oroboro_module_new,
            free: oroboro_module_free,
            set_param: oroboro_module_set_param,
            reset: oroboro_module_reset,
            tick: oroboro_module_tick,
            connected: oroboro_module_connected,
            midi: oroboro_module_midi,
            get_param: oroboro_module_get_param,
            get_state: oroboro_module_get_state,
            set_state: oroboro_module_set_state,
            menu: oroboro_module_menu,
            menu_choose: oroboro_module_menu_choose,
            panel: $panel,
            art: $art,
            lights: $lights,
            draw: $draw,
            pointer: $pointer,
            screen_knobs: $screen_knobs,
        }
    };
}

/// A module's ABI functions as a table: for a module built into a program
/// with others instead of a library of its own (`export_module!` with the
/// `built-in` feature makes `oroboro_built_in()`, which gives it). The
/// panel's functions are there for a module exported with its panel.
#[derive(Clone, Copy, Debug)]
pub struct Exports {
    pub abi: unsafe extern "C" fn() -> u32,
    pub spec: unsafe extern "C" fn() -> *const c_char,
    pub new: unsafe extern "C" fn(f32) -> *mut c_void,
    pub free: unsafe extern "C" fn(*mut c_void),
    pub set_param: unsafe extern "C" fn(*mut c_void, u32, f32),
    pub reset: unsafe extern "C" fn(*mut c_void),
    pub tick: unsafe extern "C" fn(*mut c_void, *const f32, *mut f32),
    pub connected: unsafe extern "C" fn(*mut c_void, *const u8, u32),
    pub midi: unsafe extern "C" fn(*mut c_void, *const u8, u32),
    pub get_param: unsafe extern "C" fn(*mut c_void, u32) -> f32,
    pub get_state: unsafe extern "C" fn(*mut c_void) -> *const c_char,
    pub set_state: unsafe extern "C" fn(*mut c_void, *const c_char),
    pub menu: unsafe extern "C" fn(*mut c_void) -> *const c_char,
    pub menu_choose: unsafe extern "C" fn(*mut c_void, u32, f32),
    pub panel: Option<unsafe extern "C" fn(*mut c_void) -> *const c_char>,
    pub art: Option<unsafe extern "C" fn(*mut u32) -> *const u8>,
    pub lights: Option<unsafe extern "C" fn(*mut c_void, *mut f32, u32) -> u32>,
    pub draw: Option<unsafe extern "C" fn(*mut c_void, *mut u32) -> *const f32>,
    #[allow(clippy::type_complexity)]
    pub pointer: Option<unsafe extern "C" fn(*mut c_void, u32, f32, f32, f32, f32, u32, u32) -> u32>,
    /// Its screen turns its knobs (`oroboro_module_screen_knobs`).
    pub screen_knobs: Option<unsafe extern "C" fn(*mut c_void) -> u32>,
}

#[cfg(test)]
mod tests {
    use super::*;

    struct Doubler {
        gain: f32,
        invert: bool,
    }

    impl Module for Doubler {
        fn spec() -> Spec {
            Spec::new("native/Doubler").category("level").input("In").output("Out").param(Param::new("Gain", 0.0, 4.0, 2.0))
        }
        fn new(_sample_rate: f32) -> Self {
            Doubler { gain: 2.0, invert: false }
        }
        fn set_param(&mut self, index: usize, value: f32) {
            if index == 0 {
                self.gain = value;
            }
            if value < 0.0 {
                panic!("a negative gain");
            }
        }
        fn tick(&mut self, inputs: &[f32], outputs: &mut [f32]) {
            outputs[0] = inputs[0] * if self.invert { -self.gain } else { self.gain };
        }
        fn state(&self) -> Option<String> {
            Some(format!("{{\"invert\": {}}}", self.invert))
        }
        fn set_state(&mut self, state: &str) {
            self.invert = state.contains("true");
        }
        fn menu(&self) -> Vec<MenuEntry> {
            vec![
                MenuEntry::label("Doubler"),
                MenuEntry::check(1, "Invert", self.invert),
                MenuEntry::Separator,
                MenuEntry::choice("Gain", &["Unity", "Double"], usize::from(self.gain >= 2.0), 10),
                MenuEntry::slider(2, &format!("Gain: {}", self.gain), self.gain / 4.0),
            ]
        }
        fn choose(&mut self, id: u32, value: f32) {
            match id {
                1 => self.invert = !self.invert,
                2 => self.gain = 4.0 * value,
                10 => self.gain = 1.0,
                11 => self.gain = 2.0,
                _ => {}
            }
        }
        fn param(&self, index: usize) -> Option<f32> {
            (index == 0).then_some(self.gain)
        }
        // (a note's velocity is its gain: 127 is 4)
        fn midi(&mut self, message: &[u8]) {
            if let [status, _key, velocity] = message {
                if status & 0xf0 == 0x90 && *velocity > 0 {
                    self.gain = *velocity as f32 / 127.0 * 4.0;
                }
            }
        }
    }

    #[test]
    fn specs_are_json_the_plugin_reads() {
        let json = Spec::new("native/Say \"hi\"").category("fx").input("V/Oct").output("Out")
            .param(Param::new("Freq", 20.0, 20000.0, 440.0).unit("Hz").exp())
            .source_hash("00ff00ff00ff00ff")
            .to_json();
        assert_eq!(
            json,
            "{\"module\": \"native/Say \\\"hi\\\"\", \"category\": \"fx\", \"inputs\": [{\"name\": \"V/Oct\"}], \
             \"outputs\": [\"Out\"], \"params\": [{\"name\": \"Freq\", \"min\": 20, \"max\": 20000, \"default\": 440, \
             \"unit\": \"Hz\", \"scaling\": \"exp\"}], \"source_hash\": \"00ff00ff00ff00ff\"}"
        );
        // who makes it, when it says
        let json = Spec::new("native/X").vendor(" Signal \"Works\" ").to_json();
        assert!(json.ends_with("\"source_hash\": null, \"vendor\": \"Signal \\\"Works\\\"\"}"), "{json}");
        // and its name says so: native/<Vendor>/<Name>, from a name without a maker
        let json = Spec::new("native/X").vendor(" Signal Works ").to_json();
        assert!(json.starts_with("{\"module\": \"native/Signal Works/X\", "), "{json}");
        assert_eq!(Spec::new("faust/Echo").vendor("Acme").full_name(), "native/Acme/Echo");
        assert_eq!(Spec::new("native/Acme/Echo").vendor("Acme").full_name(), "native/Acme/Echo");
        assert_eq!(Spec::new("native/Echo").full_name(), "native/Echo", "(refused: who makes it?)");
        // what a jack carries, where its maker says
        let json = Spec::new("native/X").input("In").input_as("Kick", Signal::Logic).output_as("Env", Signal::Control).output("Out").to_json();
        assert!(
            json.contains("\"inputs\": [{\"name\": \"In\"}, {\"name\": \"Kick\", \"signal\": \"logic\"}], \"outputs\": [{\"name\": \"Env\", \"signal\": \"control\"}, \"Out\"]"),
            "{json}"
        );
        // one that takes MIDI says so
        let json = Spec::new("native/X").vendor("Acme").midi().to_json();
        assert!(json.ends_with("\"vendor\": \"Acme\", \"midi\": true}"), "{json}");
        assert!(!Spec::new("native/X").to_json().contains("midi"));
        // a selector names its positions
        let json = Spec::new("native/X").param(Param::choice("Wave", &["Sine", "Saw"], 1)).to_json();
        assert!(
            json.contains("{\"name\": \"Wave\", \"min\": 0, \"max\": 1, \"default\": 1, \"unit\": \"\", \"scaling\": \"enum\", \"labels\": [\"Sine\", \"Saw\"]}"),
            "{json}"
        );
    }

    /// The ABI's functions over a module: it plays, takes its knob, and a
    /// panic silences that instance instead of reaching the host.
    #[test]
    fn the_abi_runs_a_module_and_catches_its_panics() {
        unsafe {
            let m = abi::new::<Doubler>(48_000.0);
            let mut out = [0.0f32];
            abi::tick::<Doubler>(m, [0.5f32].as_ptr(), out.as_mut_ptr());
            assert_eq!(out, [1.0]);
            abi::set_param::<Doubler>(m, 0, 3.0);
            abi::tick::<Doubler>(m, [0.5f32].as_ptr(), out.as_mut_ptr());
            assert_eq!(out, [1.5]);
            // MIDI: a note at full velocity, and messages the ABI doesn't have
            abi::midi::<Doubler>(m, [0x90u8, 60, 127].as_ptr(), 3);
            abi::midi::<Doubler>(m, std::ptr::null(), 3);
            abi::midi::<Doubler>(m, [0x90u8, 60, 1, 0].as_ptr(), 4);
            abi::tick::<Doubler>(m, [0.5f32].as_ptr(), out.as_mut_ptr());
            assert_eq!(out, [2.0]);
            let hook = std::panic::take_hook();
            std::panic::set_hook(Box::new(|_| {}));
            abi::set_param::<Doubler>(m, 0, -1.0);
            std::panic::set_hook(hook);
            abi::tick::<Doubler>(m, [0.5f32].as_ptr(), out.as_mut_ptr());
            assert_eq!(out, [0.0], "silent once it panicked");
            abi::free::<Doubler>(m);
        }
        assert!(abi::spec_json::<Doubler>().to_str().unwrap().contains("native/Doubler"));
    }

    /// A module's state goes out as text and comes back; its menu is JSON
    /// the plugin reads, and a choice changes the state and a knob.
    #[test]
    fn the_abi_gives_a_modules_state_and_menu() {
        let text = |p: *const c_char| unsafe { (!p.is_null()).then(|| CStr::from_ptr(p).to_str().unwrap().to_string()) };
        unsafe {
            let m = abi::new::<Doubler>(48_000.0);
            assert_eq!(text(abi::get_state::<Doubler>(m)).as_deref(), Some("{\"invert\": false}"));
            assert_eq!(
                text(abi::menu::<Doubler>(m)).unwrap(),
                "[{\"kind\": \"label\", \"text\": \"Doubler\"}, \
                 {\"kind\": \"item\", \"id\": 1, \"text\": \"Invert\", \"right\": \"\", \"checked\": false, \"disabled\": false}, \
                 {\"kind\": \"separator\"}, \
                 {\"kind\": \"item\", \"id\": 0, \"text\": \"Gain\", \"right\": \"Double\", \"items\": [\
                 {\"kind\": \"item\", \"id\": 10, \"text\": \"Unity\", \"right\": \"\", \"checked\": false, \"disabled\": false}, \
                 {\"kind\": \"item\", \"id\": 11, \"text\": \"Double\", \"right\": \"\", \"checked\": true, \"disabled\": false}]}, \
                 {\"kind\": \"slider\", \"id\": 2, \"text\": \"Gain: 2\", \"value\": 0.5}]"
            );
            abi::menu_choose::<Doubler>(m, 1, 0.0);
            assert_eq!(text(abi::get_state::<Doubler>(m)).as_deref(), Some("{\"invert\": true}"));
            abi::menu_choose::<Doubler>(m, 10, 0.0);
            assert_eq!(abi::get_param::<Doubler>(m, 0), 1.0);
            abi::menu_choose::<Doubler>(m, 2, 0.75);
            assert_eq!(abi::get_param::<Doubler>(m, 0), 3.0);
            // another instance takes the state
            let other = abi::new::<Doubler>(48_000.0);
            let state = CString::new("{\"invert\": true}").unwrap();
            abi::set_state::<Doubler>(other, state.as_ptr());
            let mut out = [0.0f32];
            abi::tick::<Doubler>(other, [0.5f32].as_ptr(), out.as_mut_ptr());
            assert_eq!(out, [-1.0]);
            abi::free::<Doubler>(other);
            abi::free::<Doubler>(m);
        }
    }

    /// A level whose screen turns its knob: a drag up turns it up, the
    /// module plays what the screen has, and the knob is read of the screen.
    struct Level {
        knob: Arc<Knob>,
    }

    struct Knob(std::sync::atomic::AtomicU32);

    impl Knob {
        fn get(&self) -> f32 {
            f32::from_bits(self.0.load(std::sync::atomic::Ordering::Relaxed))
        }
        fn set(&self, v: f32) {
            self.0.store(v.to_bits(), std::sync::atomic::Ordering::Relaxed)
        }
    }

    impl Screen for Knob {
        fn draw(&self, _d: &mut Drawing) {}
        fn pointer(&self, event: Pointer) -> bool {
            if let Pointer::Move { dy, .. } = event {
                self.set((self.get() - dy / 100.0).clamp(0.0, 1.0));
            }
            true
        }
        fn knob(&self, index: usize) -> Option<f32> {
            (index == 0).then(|| self.get())
        }
    }

    impl Module for Level {
        fn spec() -> Spec {
            Spec::new("native/Level").input("In").output("Out").param(Param::new("Gain", 0.0, 1.0, 0.5)).param(Param::new("Unknown", 0.0, 1.0, 0.0))
        }
        fn new(_sample_rate: f32) -> Self {
            Level { knob: Arc::new(Knob(0.5f32.to_bits().into())) }
        }
        fn set_param(&mut self, index: usize, value: f32) {
            if index == 0 {
                self.knob.set(value);
            }
        }
        fn tick(&mut self, inputs: &[f32], outputs: &mut [f32]) {
            outputs[0] = inputs[0] * self.knob.get();
        }
        fn screen(&self) -> Option<Arc<dyn Screen>> {
            Some(self.knob.clone() as Arc<dyn Screen>)
        }
    }

    #[test]
    fn a_screen_that_turns_a_knob_has_it() {
        unsafe {
            let m = abi::new::<Level>(48_000.0);
            assert_eq!(abi::screen_knobs::<Level>(m), 1);
            abi::set_param::<Level>(m, 0, 0.25);
            assert_eq!(abi::get_param::<Level>(m, 0), 0.25);
            // dragged 50 up on the screen: half way further, played at once
            assert_eq!(abi::pointer::<Level>(m, 3, 10.0, 10.0, 0.0, -50.0, 0, 0), 1);
            assert_eq!(abi::get_param::<Level>(m, 0), 0.75);
            let mut out = [0.0f32];
            abi::tick::<Level>(m, [2.0f32].as_ptr(), out.as_mut_ptr());
            assert_eq!(out, [1.5]);
            assert!(abi::get_param::<Level>(m, 1).is_nan(), "a knob the screen doesn't know");
            abi::free::<Level>(m);
            // a module whose screen turns none is asked itself
            let d = abi::new::<Doubler>(48_000.0);
            assert_eq!((abi::screen_knobs::<Doubler>(d), abi::get_param::<Doubler>(d, 0)), (0, 2.0));
            abi::free::<Doubler>(d);
        }
    }
}

/// Built in (the `built-in` feature): the same functions, as a table.
#[cfg(all(test, feature = "built-in"))]
mod built_in {
    use crate::{Module, Param, Spec};

    pub struct Halver;

    impl Module for Halver {
        fn spec() -> Spec {
            Spec::new("native/Halver").vendor("Acme").category("level").input("In").output("Out")
                .param(Param::new("Gain", 0.0, 1.0, 0.5))
        }
        fn new(_sample_rate: f32) -> Self {
            Halver
        }
        fn set_param(&mut self, _index: usize, _value: f32) {}
        fn tick(&mut self, inputs: &[f32], outputs: &mut [f32]) {
            outputs[0] = inputs[0] / 2.0;
        }
    }

    crate::export_module!(Halver);

    #[test]
    fn a_built_in_module_gives_its_functions_as_a_table() {
        let exports = oroboro_built_in();
        unsafe {
            assert_eq!((exports.abi)(), crate::ABI);
            let spec = std::ffi::CStr::from_ptr((exports.spec)()).to_string_lossy().into_owned();
            assert!(spec.starts_with("{\"module\": \"native/Acme/Halver\""), "{spec}");
            let m = (exports.new)(48_000.0);
            let mut out = [0.0f32];
            (exports.tick)(m, [3.0f32].as_ptr(), out.as_mut_ptr());
            assert_eq!(out, [1.5]);
            (exports.free)(m);
        }
        assert!(exports.panel.is_none() && exports.draw.is_none(), "no panel of its own");
    }
}
