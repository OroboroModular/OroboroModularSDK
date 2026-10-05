# Modules from VCV Rack sources

`oromod rack build` compiles a **VCV Rack 2 plugin's** source, unchanged,
into one compiled module per Rack module
([native-modules.md](native-modules.md)). The same folder keeps building
for Rack.

```bash
cd MyPlugin                          # where plugin.json and the Makefile are
oromod rack list .                   # its modules, and what each will be called here
oromod rack build . --install        # build/<Name>.oromodule for each, installed to try
                                     #   (turn on Developer mode in the plugin's Settings)
oromod check build/MyModule.oromodule
oromod login                         # once
oromod publish build/*.oromodule     # the library signs each build
```

Useful options: `--only X,Y,Z` builds just those modules, `--jobs 4`
limits parallel compiles, `--clean` rebuilds everything.

- **You need** `g++` or `clang++` on your `PATH` (or `CXX` set). On
  Windows that's MinGW-w64, for example WinLibs, not Visual Studio.
- **Other platforms**: run `oromod rack build .` on each one into the
  same `build/` folder. Each module file keeps the other platforms'
  builds, so after the last one it has them all.
- **New versions**: raise `version` in `plugin.json`, build and publish
  again.
- **Code that must differ here**: `OROBORO_MODULAR` is defined during the
  build, so you can wrap those lines in `#ifdef OROBORO_MODULAR`. Most
  modules need nothing.
- **Publishing** compiled modules needs an approved developer account.
  Apply on your oroboro.space account page
  ([community-library.md](community-library.md)).

## How it works

The SDK has its own `rack.hpp` (`rack/include`), written to Rack 2's
plugin interface, and a bridge (`rack/src`) that exposes each Rack module
through Oroboro Modular's module ABI. None of it is Rack's code.

`oromod rack build` reads your `Makefile` (sources, include paths,
defines, libraries and platform conditionals) and `plugin.json`, compiles
each source once the way Rack does, and links a separate library for each
module. Then it test-plays every module in its own process, leaving out
any that crash or hang, and packs each one with its panel.

Module names come from `plugin.json`, with the plugin's `brand` as the
vendor. Characters a file name can't contain are replaced, and names over
40 characters are shortened at a word.

## The project file

An `oroboro-rack.json` changes how the plugin is built here. Put it in
your plugin's folder, or in a folder of its own with `source` pointing at
the plugin:

```json
{
  "source": "../../SomeRackPlugin",
  "out": "build",
  "skip": ["Scope"],
  "names": { "gsx": "Grains X" },
  "categories": { "Ratio": "mixer" },
  "flags": ["-DSOME_OPTION"],
  "vendor": "Some Rack Plugin",
  "hide": { "Vac": ["FM", "Cutoff CV"], "*": ["Clock in"] },
  "signals": { "Vac": { "Kick": "logic", "out 2": "cv" } }
}
```

- `names`, `categories`, `skip`: by module slug. `only` builds a subset;
  `prefix` goes before every name.
- `vendor`: who makes the modules, if not the plugin's `brand`.
- `hide`: inputs not to show. Every knob here accepts a cable with its
  own attenuator, so an FM or CV input that only moves a knob is often
  redundant. Hidden inputs keep working in patches that already use them.
- `signals`: a jack's colour (`audio`, `cv` or `logic`) when its name
  doesn't make it clear. Name a jack, or use its position (`"in 2"`,
  `"out 1"`). `"*"` applies to every module.

## What comes over

| In Rack | In Oroboro Modular |
|---|---|
| `configParam`, `configSwitch`, `configButton` | Knobs with their ranges and defaults; switches as selectors; buttons as on/off |
| `configInput`, `configOutput` | Jacks, with names shortened to the part before a bracket, colon or dash |
| A `V/Oct` input | Follows the keyboard when nothing is plugged in |
| `dataToJson`, `dataFromJson` | The module's state, saved with the patch |
| `appendContextMenu` | The module's right-click menu: items, checkmarks, submenus, sliders |
| Your panel SVG and widgets | The panel, with knobs, jacks and lights where your `ModuleWidget` puts them |
| A widget's own `draw` or `drawLayer` | Drawn live from the playing module |
| `onButton`, `onDragMove`, `onHover`, scroll | Mouse input on your widgets, by Rack's rules |
| `osdialog` file dialogs | The system's dialogs (Windows; Linux and macOS not yet) |
| `isConnected()` | True when a cable (or the keyboard, for `V/Oct`) feeds the input |
| `midi::Input` | The plugin's MIDI. In the voice area each instance gets its own voice's notes. |
| `rack::dsp`, `simd::float_4`, `random`, `string`, `math`, `json_t` | All there |

Voltages are the same as Rack's: about ±5 V audio, 10 V gates, 1 V per
octave with 0 V at C4.

### Menus and state

The menu is read from the playing module and choices run on it, just as
in Rack. The new state is saved with the patch, so a choice can be
undone. Changes the module makes to itself while playing (a recorded
buffer, a learned sequence) are only saved when a menu choice or a click
on the panel triggers them.

Your panel's constructor runs for the menu, so it must work without a
window: `APP->window`, fonts and images exist but draw nothing. If it
crashes, the module is built without its menu and the build tells you.

### The panel

- **Your SVG** is drawn by the plugin at full sharpness at any zoom, with
  the colours adapted to the theme: the background becomes the theme's
  module colour, boxes on it become wells, lettering stays readable, and
  hues stay what they are. Text that's still text in the SVG is set in
  the plugin's font.
- **Controls** are the plugin's own knobs, faders, switches and jacks,
  placed where your widgets are. Your own component classes work like the
  classes they derive from. Rack's component artwork isn't used.
- **Lights** sit where their widgets are, in their colours.
- **Anything without a place on the panel** doesn't show, as in Rack.
- A button with no parameter behind it (a LOAD button made with
  `createWidget`) is yours: it's drawn, and it gets the clicks.

What your widgets draw themselves is recorded each frame from the playing
module and drawn over the panel. There are no fonts, so text measurement
is approximate: align text with `nvgTextAlign` rather than by measuring
it. SVGs your widgets draw with `window::svgDraw` are packed with the
module.

## A panel in Oroboro Modular's style

Instead of your Rack panel, you can give a module a panel in the
plugin's own style: one file per module, `oroboro/<Slug>.json`, next to
your plugin (or the project file). `oromod rack ui .` writes a starter
for every module that has none.

```json
{
  "width": 255,
  "items": [
    {"knob": "Rate", "x": 30, "y": 44, "size": "big"},
    {"knob": "Depth", "x": 74, "y": 44},
    {"button": "Shape", "x": 122, "y": 44, "width": 44},
    {"fader": "Level", "x": 170, "y": 60, "height": 60},
    {"input": "Sync", "x": 20, "y": 110},
    {"output": "Audio", "x": 235, "y": 110, "label": "Out"},
    {"light": 0, "x": 60, "y": 28, "color": "#90c73e"},
    {"text": "LFO", "x": 200, "y": 28, "size": 10},
    {"line": [8, 92, 247, 92]},
    {"screen": [106, 14, 305, 154], "x": 127, "y": 160, "scale": 0.8}
  ]
}
```

- `x` and `y` are the centre of each item, in pixels from the top left.
  The top 17 pixels are the module's title.
- Knobs, buttons, faders and jacks are named as `oromod check` prints
  them (or by number). Each gets its name as a label unless you set
  `label`.
- `screen` takes a part of the Rack panel whose widgets draw themselves
  (x, y, width, height in Rack panel pixels) and shows it live, with mouse
  input, at `x`, `y` and `scale`.
- Anything you leave out doesn't show, so place every jack a patch needs.

`{ "layout": "auto" }` lets the plugin lay the module out by itself
instead. Mistakes in the file are reported at build time.

## Not supported

- Keyboard input on widgets
- PNG images in what widgets draw (SVGs work)
- Reading an SVG's shapes (`svg->handle->shapes`): the plugin draws SVGs
  itself, so the shapes are empty
- Sixteen channels on one cable: each voice has its own instance
- Expanders: a patch has no "next to"
- Files from your plugin's `res/` folder other than panel SVGs (samples,
  presets) aren't packed with the module
- Network access, MIDI out and audio devices
- The rack around a module: no neighbours, cables, clipboard or undo

## The licence

A module built from a Rack plugin carries that plugin's licence. Most
Rack plugins are **GPL-3.0-or-later**: build them for yourself freely,
but publishing a build means publishing it under the GPL with its source
available. `oromod rack build` prints the plugin's licence when it
starts, and `oromod publish` asks for `--source`, a link to the plugin's
source and the SDK's `rack/` folder at the version you built. Only
publish someone else's modules with their permission.
