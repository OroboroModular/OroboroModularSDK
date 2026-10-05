# oroboro-module

Compiled modules for [Oroboro Modular](https://www.oroboro.space), in Rust.

A compiled module is a native library (a `cdylib`) that exports Oroboro
Modular's module ABI, version 1 (its C header is
[`native/include/oroboro_module.h`](../include/oroboro_module.h)). This crate does
the exporting: implement `Module` for a type, describe it with a `Spec`,
and `export_module!(YourType);`.

```toml
[lib]
crate-type = ["cdylib"]

[dependencies]
oroboro-module = { git = "https://github.com/OroboroModular/OroboroModularSDK", tag = "v0.1.0" }
```

```rust
use oroboro_module::{export_module, Module, Param, Spec};

pub struct Fold { amount: f32 }

impl Module for Fold {
    fn spec() -> Spec {
        Spec::new("native/Fold").vendor("Acme").category("shaper").input("In").output("Out")
            .param(Param::new("Amount", 1.0, 8.0, 2.0))
    }
    fn new(_sample_rate: f32) -> Self { Fold { amount: 2.0 } }
    fn set_param(&mut self, index: usize, value: f32) { if index == 0 { self.amount = value } }
    fn tick(&mut self, inputs: &[f32], outputs: &mut [f32]) {
        outputs[0] = 5.0 * (inputs[0] * self.amount / 5.0).sin();
    }
}

export_module!(Fold);
```

Signals are volts, as Oroboro Modular's own modules: audio about ±5 V,
gates 0/10 V, pitch 1 V an octave with 0 V = C4. The plugin runs one
instance per voice and calls `tick` once a sample. A module can keep a
state with the patch, add items to its menu, take MIDI, say what its jacks
carry, and have a panel of its own with a screen and lights: the crate's
documentation (`cargo doc --open`) has all of it.

The plugin loads a compiled module from `native/<Vendor>/` in its modules
folder when the Community Library signed it, or, with Developer mode on in
its Settings, when it's a build of your own. The Oroboro Modular SDK's
tool, `oromod`, packs builds into a module file, checks it and publishes
it to the Community Library.

Oroboro's own modules, made with this crate, are examples of all of it:
[OroboroModularModules](https://github.com/OroboroModular/OroboroModularModules).

## Licence

MIT: see [LICENSE](LICENSE). What you make with the crate is yours.
