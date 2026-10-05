# Compiled modules

A compiled module is a native library that exports the **module ABI**, a
small set of C functions. The plugin loads it from `native/<Vendor>/` in
the modules folder and creates one instance per voice. It runs at native
speed and can do anything code can.

Because native code runs inside your DAW, the plugin only loads a
library that the Community Library has **signed** (it signs every build
published there). Your own unsigned builds load when **Developer mode**
is on in the plugin's Settings. Only turn that on for modules you built
yourself.

## In Rust

```bash
oromod new native "Crunch Box" --vendor "Your Name"   # creates crunch-box/
```

Or by hand, a `cdylib` crate that depends on `oroboro-module`, from the
SDK on GitHub (or by `path` from your SDK folder):

```toml
[lib]
crate-type = ["cdylib", "rlib"]

[dependencies]
oroboro-module = { git = "https://github.com/OroboroModular/OroboroModularSDK", tag = "v0.1.0" }
```

```rust
use oroboro_module::{export_module, Module, Param, Spec};

pub struct Fold { fold: f32 }

impl Module for Fold {
    fn spec() -> Spec {
        Spec::new("native/Fold")
            .vendor("Acme")
            .category("shaper")
            .input("In")
            .output("Out")
            .param(Param::new("Fold", 1.0, 10.0, 1.0))
    }
    fn new(_sample_rate: f32) -> Self { Fold { fold: 1.0 } }
    fn set_param(&mut self, index: usize, value: f32) { if index == 0 { self.fold = value } }
    fn reset(&mut self) {}
    fn tick(&mut self, inputs: &[f32], outputs: &mut [f32]) {
        outputs[0] = 5.0 * (inputs[0] * self.fold / 5.0).sin();
    }
}

export_module!(Fold);
```

`native/examples/fold` is a complete example with a test, and
[OroboroModularModules](https://github.com/OroboroModular/OroboroModularModules) has more examples: oscillators, delays and a
reverb in Rust.

| Method | What it does |
|---|---|
| `spec()` | The interface: name, vendor (together `native/Acme/Fold`), jacks and knobs. Add new knobs at the end. |
| `new(sample_rate)` | Creates an instance. Allocate here, not in `tick`. |
| `set_param(index, value)` | A knob changed. Runs on the audio thread: don't block or allocate. |
| `reset()` | Clear internal state. |
| `tick(inputs, outputs)` | One sample: read every input, write every output. |
| `connected(inputs)` | Optional. Which inputs have a cable, for modules that behave differently without one (an internal clock when nothing is at Clock). |
| `midi(message)` | Optional, with `Spec::midi()`. One MIDI message, just before the sample it belongs to. In the voice area each instance gets its own voice's notes plus every channel's other messages; in the FX area, everything. |
| `state()`, `set_state(text)` | Optional. Anything the module saves beyond its knobs, as text. Stored with the patch. |
| `menu()`, `choose(id, value)` | Optional. Your own options in the module's right-click menu. |

A few more things the spec can say:

- `Param::choice("Wave", &["Sine", "Saw", "Square"], 0)` is a selector
  shown as buttons (up to eight).
- `Param::new(..).steps()` is a knob that moves in whole steps.
- `.input_as("Kick", Signal::Logic)` sets a jack's colour when its name
  doesn't make it clear.

Build and try it:

```bash
oromod build                             # cargo build --release, into "Crunch Box.oromodule"
oromod check "Crunch Box.oromodule"      # interface, a second of sound, cost per sample
oromod install "Crunch Box.oromodule"    # into native/<Vendor>/
```

`oromod check` also reports how long a sample takes. A few tenths of a
microsecond is cheap: a 16-voice patch calls each instance 48,000 times a
second.

### Menus and state

Options that aren't knobs (a mode, a range, a quality setting) go in the
module's menu, and whatever they set belongs in its state:

```rust
use oroboro_module::MenuEntry;

impl Module for Fold {
    // …
    fn state(&self) -> Option<String> {
        Some(format!("{{\"soft\": {}, \"stages\": {}}}", self.soft, self.stages))
    }
    fn set_state(&mut self, state: &str) {
        // read what you understand: a patch may hold an older version's state
    }
    fn menu(&self) -> Vec<MenuEntry> {
        vec![
            MenuEntry::check(1, "Soft clipping", self.soft),
            MenuEntry::choice("Stages", &["2", "4", "8"], self.stages, 10),   // ids 10, 11, 12
        ]
    }
    fn choose(&mut self, id: u32, value: f32) {
        match id {
            1 => self.soft = !self.soft,
            10..=12 => self.stages = (id - 10) as usize,
            _ => {}
        }
    }
}
```

The plugin shows the menu from a separate, non-playing instance. After a
choice it saves that instance's `state()` into the patch (so it can be
undone) and recreates the playing instances with it. So `state()` must
cover everything `choose` can change, and the module restarts when an
option changes.

### Panels, screens and lights

A module can bring its own panel, with a **screen** that shows something
live (a scope, a spectrum, a grid of steps) and reacts to the mouse.
`Panel::stock` lays it out in the plugin's own style; `Panel::new` uses
your own SVG artwork (`Module::art`), drawn in the theme's colours.
Export it with `export_module!(YourType, panel)`:

```rust
use std::sync::Arc;
use oroboro_module::panel::MEDIUM;
use oroboro_module::{export_module, Align, Color, Drawing, Module, Panel, Pointer, Screen};

impl Module for Scope {
    // …
    fn panel() -> Option<Panel> {
        Some(Panel::stock(255.0, 250.0)
            .screen(8.0, 22.0, 239.0, 132.0)                      // left, top, width, height
            .knob(0, 32.0, 178.0, MEDIUM).label("Time")
            .selector(1, 90.0, 176.0, 40.0).label("Mode")
            .input(0, 26.0, 230.0).label("In")
            .output(0, 229.0, 230.0).label("Out")
            .light(0, 128.0, 228.0, 6.0, &["#6dd68e"]))
    }
    fn lights(&self, out: &mut [f32]) -> usize {
        if let Some(first) = out.first_mut() { *first = self.level; }
        1
    }
    fn screen(&self) -> Option<Arc<dyn Screen>> {
        Some(self.screen.clone())                // created in `new`, kept by the module
    }
}

export_module!(Scope, panel);
```

The screen is drawn on the UI thread **while the audio thread is running
the module**, so it must never touch the module directly. Keep a shared
handle (an `Arc`), write what to show into it with atomics or a lock that
`tick` never waits on, and read that in `draw`:

```rust
impl Screen for ScopeScreen {
    fn draw(&self, d: &mut Drawing) {
        d.clip(10.0, 26.0, 245.0, 172.0);
        d.line(&self.points(), 1.2, Color::rgb(0x6d, 0xd6, 0x8e));
        d.text(14.0, 34.0, "1 kHz", 7.0, Color::rgb(0xe6, 0xea, 0xec), Align::Left);
    }
    fn pointer(&self, event: Pointer) -> bool {
        // return true for a press the screen handles; otherwise it drags the module
        matches!(event, Pointer::Press { x, y, .. } if self.on_screen(x, y))
    }
}
```

`Drawing` fills shapes, draws lines and text, all in panel pixels, and
adapts its colours to the theme. `lights` is called on the audio thread,
between samples.

The Spectrum Analyser (`native/Oroboro/Spectrum`, built into the plugin)
uses all of this. Its source is in
[OroboroModularModules](https://github.com/OroboroModular/OroboroModularModules/tree/main/SpectrumAnalyser).

## From a Faust source

```bash
oromod faust build Wobble.dsp --install
```

compiles the source with Faust's Rust backend into `Wobble.oromodule`,
named `native/<Vendor>/Wobble` by its `declare vendor`. See
[faust-modules.md](faust-modules.md).

## In C or C++

`native/include/oroboro_module.h` is the ABI:

```c
uint32_t    oroboro_module_abi(void);                         // 1
const char *oroboro_module_spec(void);                        // JSON: the interface
void       *oroboro_module_new(float sample_rate);
void        oroboro_module_free(void *m);
void        oroboro_module_set_param(void *m, uint32_t index, float value);
void        oroboro_module_reset(void *m);
void        oroboro_module_tick(void *m, const float *inputs, float *outputs);
// optional
void        oroboro_module_connected(void *m, const uint8_t *inputs, uint32_t count);
void        oroboro_module_midi(void *m, const uint8_t *message, uint32_t size);
const char *oroboro_module_get_state(void *m);
void        oroboro_module_set_state(void *m, const char *state);
float       oroboro_module_get_param(void *m, uint32_t index);
const char *oroboro_module_menu(void *m);
void        oroboro_module_menu_choose(void *m, uint32_t id, float value);
const char    *oroboro_module_panel(void *m);
const uint8_t *oroboro_module_art(uint32_t *size);
uint32_t       oroboro_module_lights(void *m, float *out, uint32_t count);
const float   *oroboro_module_draw(void *m, uint32_t *count);
const uint8_t *oroboro_module_picture(uint32_t index, uint32_t *size);
uint32_t       oroboro_module_pointer(void *m, uint32_t kind, float x, float y,
                                      float dx, float dy, uint32_t button, uint32_t mods);
uint32_t       oroboro_module_live(void);
```

Leave out the optional functions you don't need; the plugin looks them up
by name. The header documents each one, including the JSON for menus and
panels and the format of drawing lists. Export with C linkage from a
64-bit shared library, and never let an exception cross the boundary.

The interface JSON:

```json
{"module": "native/Acme/Fold", "category": "shaper",
 "inputs": [{"name": "In"}], "outputs": ["Out"],
 "params": [{"name": "Fold", "min": 1, "max": 10, "default": 1, "unit": "", "scaling": "lin"}],
 "source_hash": null}
```

`scaling` is `lin`, `exp` (minimum above 0) or `enum`. An `enum` knob
that counts from 0 can name its positions with `"labels"`. The limits are
64 inputs, 64 outputs and 256 knobs.

## The module file

Every build produces a `.oromodule` file: the interface, the version and
description, and one build per platform. Building into the same file on
each platform gives one file that plays everywhere:

```bash
oromod build crunch-box --out dist   # on Windows
oromod build crunch-box --out dist   # then on Linux, into the same dist/
oromod check "dist/Crunch Box.oromodule"
```

| Platform | Library |
|---|---|
| Windows (x64, arm64) | `name.dll` |
| Linux (x64, arm64) | `libname.so` |
| macOS | `libname.dylib`, a universal binary |

For macOS, build both architectures, join them with `lipo -create`, and
add the result with `oromod pack`. `oromod pack` also wraps libraries
built some other way, such as a C module.
