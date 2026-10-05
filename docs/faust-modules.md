# Faust DSP modules

A Faust module is a single `.dsp` file written in
[Faust DSP](https://faust.grame.fr). Drop it in the modules folder and the
plugin compiles it with Faust's interpreter (Faust 2.88 must be
installed), recompiling every time you save. `oromod faust build` turns
it into a compiled module that plays without Faust.

Examples in `examples/faust/`:

- `TapeEcho.dsp`: a tape echo that uses every kind of control
- `SimpleVoice.dsp`: a complete voice that plays from the keyboard

[OroboroModularModules](https://github.com/OroboroModular/OroboroModularModules/tree/main/PingPongDelay) has a
Faust delay that's built into a compiled module.

`oromod new faust NAME` starts a new one.

## Declarations

```faust
declare name "MoogLP";          // "faust/MoogLP" in patches (defaults to the file name)
declare category "filter";      // where Add module lists it
declare input_names "In, V/Oct";
declare output_names "Out";
declare vendor "Your Name";     // needed to build it as a compiled module

import("stdfaust.lib");
freq = hslider("[1]Freq[unit:Hz][scale:log]", 1000, 20, 20000, 1);
res  = hslider("[2]Res", 0.5, 0, 1, 0.01);
process(x, voct) = x : ve.moog_vcf(res, min(freq * 2 ^ voct, 18000));
```

- **Category**: `osc`, `filter`, `env`, `lfo`, `level`, `mixer`, `fx`,
  `shaper`, `logic`, `seq`, `delay`, `io`. Anything else lands in Custom.
- **Jacks** are `process`'s inputs and outputs, named by `input_names` and
  `output_names`.
- **Jack colours** come from the names (Gate, Clock, Trig are logic;
  V/Oct, CV, Env are control; the rest audio). If a name doesn't make it
  clear, set it yourself with `declare input_signals "audio, logic"` (and
  `output_signals`), one word per jack.
- **Keyboard inputs**: `V/Oct`, `Gate` and `Vel` read the key when nothing
  is plugged in ([signals.md](signals.md)).

## Knobs

`hslider`, `vslider` and `nentry` become knobs; `checkbox` and `button`
become on/off buttons. An `nentry` with whole steps and up to 16 values
becomes a selector.

- Values are in real units, and `[unit:Hz]` is shown next to the value.
- `[scale:log]` (with a minimum above 0) makes a knob exponential.
- **Number your knobs.** Faust sorts controls by label, so put `[1]`,
  `[2]` … at the start of each (the number isn't shown). Patches refer to
  knobs by that order, so give new knobs the next number and never
  reorder them.

The module runs one sample per call ([signals.md](signals.md)), so
formulas that depend on knobs run every sample: keep them cheap. `ma.SR`
is the engine's sample rate.

## Script or compiled

- **As a script**: put the `.dsp` in the modules folder (`oromod install`).
  The plugin checks the folder about once a second and recompiles what
  changed. If your edit doesn't compile, the last good version keeps
  playing and Add module's Faust group shows the error. The interpreter is
  several times slower than compiled code, which is fine for trying
  things out.
- **Compiled**: `oromod faust build Name.dsp --install` builds it with
  Faust's Rust backend into `Name.oromodule`, named
  `native/<Vendor>/<Name>`. It's a separate module from the
  `faust/<Name>` script. Until the Community Library signs it, it needs
  Developer mode.

## Community Library rules

The library compiles every Faust source with Faust 2.88.0 in a sandbox,
and refuses:

- imports other than Faust's own libraries (keep everything in one file)
- `ffunction`, `fconstant`, `fvariable`, `soundfile` and `component`
- a name that's already taken, or that a built-in module uses (MoogLP,
  Comb, Notch)

`oromod check Name.dsp` applies the same rules and shows the jacks and
knobs the plugin will show.
