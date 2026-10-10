# Changelog

What changed in each version of the Oroboro Modular SDK.

## [Unreleased]

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
