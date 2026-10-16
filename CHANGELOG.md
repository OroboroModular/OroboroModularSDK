# Changelog

What changed in each version of the Oroboro Modular SDK.

## [Unreleased]

## [0.1.8] - 2026-10-16

- `oromod` reads, checks and renders patches with the engine of Oroboro
  Modular 1.4.0: it knows the Expr module's new functions (`asin`,
  `acos`, `atan`, `sinh`, `cosh`, `log10`, `rand(a, b)`) and `noise`.
- A module file can carry the module's documentation, a page of
  Markdown that Oroboro Modular shows from the **?** beside the module:
  `oromod build` takes `docs.md` from the crate's folder, `oromod faust
  build` `<Name>.md` beside the source, `oromod rack build`
  `oroboro/<Slug>.md`, and `oromod pack` a file given with `--docs`.
  `oromod check` says it's there (docs/native-modules.md, "Its
  documentation").
- A build added to a module file keeps what the file says of the module
  (its description, licence, web page, documentation) unless the new
  build says otherwise: a `--version` alone dropped the rest.
- `oromod check` passes a patch with the compiled modules built into
  Oroboro Modular (Doppler, Tape, EQ …, `native/Oroboro/…`): it says
  they play in the plugin and not in its renders, where it called them
  modules it skips.
- `oromod check` notes a VCO or Wavetable (in either area, or a part of a
  composite) with a cable into its FM whose values stop before its
  hidden FM knob (knob 3): Oroboro Modular reads that as a patch from
  before the knob was hidden, at 0, and mutes the cable. Stored as 1
  (`[tune, fine, kbt, 1, …]`), the cable's amount sets how deep the FM
  goes; stored as anything else, the note says how far it turns the
  cables down.

## [0.1.7] - 2026-10-15

- `oromod` reads, checks and renders patches with the engine of Oroboro
  Modular 1.3.0: it knows the Utilities modules (Math, Compare, Counter,
  Flip, Pulse), the Expr module's lines and its four outputs, and an LFO
  locked to a stopped clock runs on as the plugin's does.
- A module can send MIDI out of Oroboro Modular (1.3.0 and later):
  `Spec::midi_out()` and `Module::midi_out(send)` in Rust, and a Rack
  module's `midi::Output` (on the output's channel). `oromod check` says
  what a module sends (docs/native-modules.md, docs/rack-modules.md).

## [0.1.6] - 2026-10-11

- `oromod` reads, checks and renders patches with the engine of Oroboro
  Modular 1.2.0: it knows the new Note Out and CC Out modules, the In
  module's Source and the Out module's Out 2, 3 and 4, and a patch from
  before whose VCO, Wavetable, Comb or Notch used its FM knob or input
  reads as the plugin reads it now.

## [0.1.5] - 2026-10-10

- A screen can turn its module's knobs: answer `Screen::knob` for them
  (kept in atomics the module reads), and what's dragged on the screen
  is heard at once and kept in the patch when it's let go
  (docs/native-modules.md, "A screen that turns knobs"). Oroboro
  Modular before 1.1.0 plays such a module but doesn't keep what its
  screen turns.

## [0.1.4] - 2026-10-10

- `scripts/build-all.ps1` (Windows) and `scripts/build-all.sh` (Linux,
  macOS) build a Rust or Faust module in Docker for Windows and Linux,
  each for x86-64 and ARM64, into one module file: run either in the
  module's folder (docs/native-modules.md, "Every platform at once").

## [0.1.3] - 2026-10-05

- The Rack bridge (`rack/include`, `rack/src`) is GPL-3.0-or-later with
  an exception: modules built with it may still be released under any
  licence, closed-source and commercial included
  (`rack/LICENSE-EXCEPTION.md`). The rest of the SDK stays MIT.

## [0.1.2] - 2026-10-05

- `oromod` checks and plays patches with the engine of Oroboro Modular
  1.0.1.

## [0.1.1] - 2026-10-05

- `oromod` has a licence of its own, `bin/LICENSE.txt`: free to use, for
  modules you sell too. The SDK's source files stay MIT.
- Each archive lists the licences of the third-party code in `oromod`, in
  `bin/THIRD_PARTY_NOTICES.txt`.

## [0.1.0] - 2026-10-05

The first release.

- **`oromod`**, ready to run on Windows and Linux (x64 and ARM64): create,
  check, build, install and publish modules.
- **Faust modules**: a `.dsp` file the plugin compiles as it runs, or a
  compiled module built with `oromod faust build`.
- **Compiled modules** in Rust (the `oroboro-module` crate) or C
  (`oroboro_module.h`, module ABI version 1), with saved state, menus,
  MIDI input, jack types, and custom panels with screens and lights.
- **VCV Rack 2 modules**: `oromod rack build` compiles a Rack plugin's
  source, unchanged, into Oroboro modules, with its panel, menu, saved
  state and custom-drawn widgets.
- **Publishing** to the Community Library, which signs every compiled
  build.
