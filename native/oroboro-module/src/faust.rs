//! Faust-generated Rust as a compiled module.
//!
//! `faust -lang rust -single -cn Dsp module.dsp` writes a `Dsp` type that
//! implements [`FaustDsp`]; include it in a module that has this one's
//! names in scope, and `export_faust!` makes it the library's module, with
//! the interface the plugin gives the same source when it runs it as a
//! script: its `declare name` (else the file's name) as `faust/<Name>`, its
//! `declare category`, `input_names` and `output_names` (and what its jacks
//! carry, `input_signals` and `output_signals`), and its controls
//! as knobs in Faust's order (sliders and number entries, buttons and
//! check boxes; bargraphs left out). `oromod faust build` writes all of
//! this for a `.dsp`:
//!
//! ```ignore
//! #[allow(non_snake_case, non_camel_case_types, non_upper_case_globals, unused_parens,
//!         unused_mut, unused_variables, unused_assignments, dead_code, clippy::all)]
//! mod dsp {
//!     use oroboro_module::faust::{FaustDsp, FaustFloat, Meta, ParamIndex, UI, F32};
//!     include!("dsp.rs");
//! }
//! oroboro_module::export_faust!(dsp::Dsp, "echo", "3b1d…");
//! ```

use std::collections::HashMap;

use crate::{Module, Param, Scaling, Signal, Spec};

/// The sample type of `faust -single`.
pub type FaustFloat = f32;
pub type F32 = f32;

/// A control's number in the generated code.
#[derive(Copy, Clone, Debug)]
pub struct ParamIndex(pub i32);

/// Receives a DSP's `declare` metadata.
pub trait Meta {
    fn declare(&mut self, key: &str, value: &str);
}

/// Receives a DSP's controls, as Faust lays them out.
pub trait UI<T> {
    fn open_tab_box(&mut self, label: &str);
    fn open_horizontal_box(&mut self, label: &str);
    fn open_vertical_box(&mut self, label: &str);
    fn close_box(&mut self);
    fn add_button(&mut self, label: &str, param: ParamIndex);
    fn add_check_button(&mut self, label: &str, param: ParamIndex);
    fn add_vertical_slider(&mut self, label: &str, param: ParamIndex, init: T, min: T, max: T, step: T);
    fn add_horizontal_slider(&mut self, label: &str, param: ParamIndex, init: T, min: T, max: T, step: T);
    fn add_num_entry(&mut self, label: &str, param: ParamIndex, init: T, min: T, max: T, step: T);
    fn add_horizontal_bargraph(&mut self, label: &str, param: ParamIndex, min: T, max: T);
    fn add_vertical_bargraph(&mut self, label: &str, param: ParamIndex, min: T, max: T);
    fn declare(&mut self, param: Option<ParamIndex>, key: &str, value: &str);
}

/// What every generated DSP implements (Faust 2.88's Rust backend).
pub trait FaustDsp {
    type T;
    fn new() -> Self
    where
        Self: Sized;
    fn metadata(&self, m: &mut dyn Meta);
    fn get_sample_rate(&self) -> i32;
    fn get_num_inputs(&self) -> i32;
    fn get_num_outputs(&self) -> i32;
    fn class_init(sample_rate: i32)
    where
        Self: Sized;
    fn instance_reset_params(&mut self);
    fn instance_clear(&mut self);
    fn instance_constants(&mut self, sample_rate: i32);
    fn instance_init(&mut self, sample_rate: i32);
    fn init(&mut self, sample_rate: i32);
    fn build_user_interface(&self, ui_interface: &mut dyn UI<Self::T>);
    fn build_user_interface_static(ui_interface: &mut dyn UI<Self::T>)
    where
        Self: Sized;
    fn get_param(&self, param: ParamIndex) -> Option<Self::T>;
    fn set_param(&mut self, param: ParamIndex, value: Self::T);
    fn compute(&mut self, count: i32, inputs: &[&[Self::T]], outputs: &mut [&mut [Self::T]]);
}

/// A source compiled with Faust: its DSP, its file's name (the module's
/// name when it declares none) and its `source_hash`.
pub trait Source: 'static {
    type Dsp: FaustDsp<T = f32> + Send + 'static;
    const STEM: &'static str;
    const SOURCE_HASH: &'static str;
}

#[derive(Default)]
struct Metadata(HashMap<String, String>);

impl Meta for Metadata {
    fn declare(&mut self, key: &str, value: &str) {
        self.0.insert(key.to_string(), value.to_string());
    }
}

/// A control as the plugin takes it.
struct Control {
    index: i32,
    param: Param,
}

#[derive(Default)]
struct Controls {
    found: Vec<Control>,
    declared: HashMap<i32, HashMap<String, String>>,
}

impl Controls {
    fn add(&mut self, label: &str, index: ParamIndex, range: [f32; 4], kind: Kind) {
        let declared = self.declared.get(&index.0).cloned().unwrap_or_default();
        let [init, min, max, step] = range;
        let scale = declared.get("scale").map(String::as_str).unwrap_or("");
        let scaling = match kind {
            Kind::Button => Scaling::Enum,
            Kind::Entry if step >= 1.0 && max - min <= 16.0 => Scaling::Enum,
            _ if matches!(scale, "log" | "exp") && min > 0.0 => Scaling::Exp,
            _ => Scaling::Lin,
        };
        let unit = declared.get("unit").cloned().unwrap_or_default();
        let param = Param { name: label.to_string(), min, max, default: init, unit, scaling, labels: Vec::new() };
        self.found.push(Control { index: index.0, param });
    }
}

#[derive(Clone, Copy)]
enum Kind {
    Slider,
    Entry,
    Button,
}

impl UI<f32> for Controls {
    fn open_tab_box(&mut self, _: &str) {}
    fn open_horizontal_box(&mut self, _: &str) {}
    fn open_vertical_box(&mut self, _: &str) {}
    fn close_box(&mut self) {}
    fn add_button(&mut self, label: &str, param: ParamIndex) {
        self.add(label, param, [0.0, 0.0, 1.0, 1.0], Kind::Button);
    }
    fn add_check_button(&mut self, label: &str, param: ParamIndex) {
        self.add(label, param, [0.0, 0.0, 1.0, 1.0], Kind::Button);
    }
    fn add_vertical_slider(&mut self, label: &str, param: ParamIndex, init: f32, min: f32, max: f32, step: f32) {
        self.add(label, param, [init, min, max, step], Kind::Slider);
    }
    fn add_horizontal_slider(&mut self, label: &str, param: ParamIndex, init: f32, min: f32, max: f32, step: f32) {
        self.add(label, param, [init, min, max, step], Kind::Slider);
    }
    fn add_num_entry(&mut self, label: &str, param: ParamIndex, init: f32, min: f32, max: f32, step: f32) {
        self.add(label, param, [init, min, max, step], Kind::Entry);
    }
    fn add_horizontal_bargraph(&mut self, _: &str, _: ParamIndex, _: f32, _: f32) {}
    fn add_vertical_bargraph(&mut self, _: &str, _: ParamIndex, _: f32, _: f32) {}
    fn declare(&mut self, param: Option<ParamIndex>, key: &str, value: &str) {
        if let Some(param) = param {
            self.declared.entry(param.0).or_default().insert(key.to_string(), value.to_string());
        }
    }
}

/// Port names: those listed ("In, FM"), else `base`, `base 2` …
/// What `n` jacks carry as `declare input_signals "audio, , logic"` says it:
/// a word a jack (audio, control or cv, logic or gate or trigger, any
/// case); an empty one, or none, leaves the jack to its name. (`oromod
/// faust build` refuses a word it doesn't know, or more words than jacks.)
fn signals(listed: Option<&String>, n: usize) -> Vec<Option<Signal>> {
    let given: Vec<&str> = listed.map(|l| l.split(',').map(str::trim).collect()).unwrap_or_default();
    (0..n)
        .map(|i| match given.get(i).map(|w| w.to_ascii_lowercase()).as_deref() {
            Some("audio") => Some(Signal::Audio),
            Some("control" | "cv") => Some(Signal::Control),
            Some("logic" | "gate" | "trigger") => Some(Signal::Logic),
            _ => None,
        })
        .collect()
}

fn names(listed: Option<&String>, n: usize, base: &str) -> Vec<String> {
    let given: Vec<&str> = listed.map(|l| l.split(',').map(str::trim).collect()).unwrap_or_default();
    (0..n)
        .map(|i| match given.get(i) {
            Some(name) if !name.is_empty() => name.to_string(),
            _ if i == 0 => base.to_string(),
            _ => format!("{base} {}", i + 1),
        })
        .collect()
}

/// A DSP made on the heap. Faust's DSPs keep their delay lines in the
/// struct, often more than a thread's stack holds, so `D::new()` (a value
/// on the stack) can't be used. All zeros is what `new` sets a DSP's
/// fields to (numbers, and arrays of them); `init` sets the rest.
fn boxed<D: FaustDsp>() -> Box<D> {
    let layout = std::alloc::Layout::new::<D>();
    if layout.size() == 0 {
        // (not `Box::new(D::new())`: a build without optimisation keeps room
        // on the stack for that value even where it's never made, and a
        // DSP's delay lines overflow the stack)
        // SAFETY: a value without size needs no memory, and has no fields to set
        return unsafe { Box::from_raw(std::ptr::NonNull::<D>::dangling().as_ptr()) };
    }
    // SAFETY: a Faust DSP is plain numbers: zeroed is the state `new` makes
    unsafe {
        let ptr = std::alloc::alloc_zeroed(layout).cast::<D>();
        if ptr.is_null() {
            std::alloc::handle_alloc_error(layout);
        }
        Box::from_raw(ptr)
    }
}

fn controls<D: FaustDsp<T = f32>>() -> Vec<Control> {
    let mut controls = Controls::default();
    D::build_user_interface_static(&mut controls);
    controls.found
}

/// The interface the plugin gives a source compiled as `D`.
pub fn spec_of<D: FaustDsp<T = f32>>(stem: &str, source_hash: Option<&str>) -> Spec {
    let dsp = boxed::<D>();
    let mut meta = Metadata::default();
    dsp.metadata(&mut meta);
    let meta = meta.0;
    let name = meta.get("name").map(|n| n.trim()).filter(|n| !n.is_empty()).unwrap_or(stem);
    let mut spec = Spec::new(&format!("faust/{name}"));
    spec.category = meta.get("category").cloned().unwrap_or_else(|| "custom".into());
    spec.inputs = names(meta.get("input_names"), dsp.get_num_inputs().max(0) as usize, "In");
    spec.outputs = names(meta.get("output_names"), dsp.get_num_outputs().max(0) as usize, "Out");
    spec.input_signals = signals(meta.get("input_signals"), spec.inputs.len());
    spec.output_signals = signals(meta.get("output_signals"), spec.outputs.len());
    spec.params = controls::<D>().into_iter().map(|c| c.param).collect();
    spec.source_hash = source_hash.map(str::to_string);
    // who makes it: its `declare vendor`, else its `declare author`
    let said = |key: &str| meta.get(key).map(|v| v.trim()).filter(|v| !v.is_empty());
    spec.vendor = said("vendor").or_else(|| said("author")).unwrap_or_default().to_string();
    spec
}

/// The most ports a module may have (the plugin's limit).
const MAX_PORTS: usize = 64;

/// A Faust DSP as a [`Module`]: one sample per `compute` call.
pub struct FaustModule<S: Source> {
    dsp: Box<S::Dsp>,
    /// Knob `i` is the DSP's control `controls[i]`.
    controls: Vec<i32>,
    inputs: usize,
    outputs: usize,
    buffer: Vec<f32>,
}

impl<S: Source> Module for FaustModule<S> {
    fn spec() -> Spec {
        spec_of::<S::Dsp>(S::STEM, Some(S::SOURCE_HASH))
    }

    fn new(sample_rate: f32) -> Self {
        let mut dsp = boxed::<S::Dsp>();
        dsp.init(sample_rate.round() as i32);
        let (inputs, outputs) = (dsp.get_num_inputs().max(0) as usize, dsp.get_num_outputs().max(0) as usize);
        let controls = controls::<S::Dsp>().into_iter().map(|c| c.index).collect();
        FaustModule { dsp, controls, inputs: inputs.min(MAX_PORTS), outputs: outputs.min(MAX_PORTS), buffer: vec![0.0; inputs + outputs] }
    }

    fn set_param(&mut self, index: usize, value: f32) {
        if let Some(&control) = self.controls.get(index) {
            self.dsp.set_param(ParamIndex(control), value);
        }
    }

    fn reset(&mut self) {
        self.dsp.instance_clear();
    }

    fn tick(&mut self, inputs: &[f32], outputs: &mut [f32]) {
        let (ins, outs) = self.buffer.split_at_mut(self.inputs);
        ins.copy_from_slice(&inputs[..self.inputs]);
        let in_refs: [&[f32]; MAX_PORTS] =
            std::array::from_fn(|i| if i < ins.len() { std::slice::from_ref(&ins[i]) } else { &[] });
        let mut cells = outs.chunks_mut(1);
        let mut out_refs: [&mut [f32]; MAX_PORTS] = std::array::from_fn(|_| cells.next().unwrap_or_default());
        self.dsp.compute(1, &in_refs[..self.inputs], &mut out_refs[..self.outputs]);
        outputs[..self.outputs].copy_from_slice(&self.buffer[self.inputs..self.inputs + self.outputs]);
    }
}

/// Exports the Faust DSP `$dsp` (compiled from `$stem`.dsp, whose
/// `source_hash` is `$hash`) as the library's module.
#[macro_export]
macro_rules! export_faust {
    ($dsp:ty, $stem:expr, $hash:expr) => {
        pub struct OroboroFaustSource;

        impl $crate::faust::Source for OroboroFaustSource {
            type Dsp = $dsp;
            const STEM: &'static str = $stem;
            const SOURCE_HASH: &'static str = $hash;
        }

        $crate::export_module!($crate::faust::FaustModule<OroboroFaustSource>);
    };
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_source_says_what_its_jacks_carry() {
        let listed = Some("Trigger, , cv, wobble".to_string());
        assert_eq!(
            signals(listed.as_ref(), 5),
            [Some(Signal::Logic), None, Some(Signal::Control), None, None],
            "an empty or unknown word, or none, leaves a jack to its name"
        );
        assert_eq!(signals(None, 2), [None, None]);
    }
}
