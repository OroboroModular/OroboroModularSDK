# Changelog

What changed in each version of the Oroboro Modular SDK.

## [Unreleased]

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
