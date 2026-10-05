# Getting started

## What you need

- **Oroboro Modular**, licensed. Without a license it plays patches but
  can't edit them, so you can't add your module to one.
- **Faust 2.88** for Faust modules ([faust.grame.fr](https://faust.grame.fr)).
  `oromod` and the plugin find it through `FAUST`, your `PATH`, or
  `~/tools/faust-*`.
- **Rust 1.80+** ([rustup.rs](https://rustup.rs)), only to build compiled
  modules from Rust or Faust.
- **g++ or clang++**, only to build modules from VCV Rack sources.

Put the SDK's `bin` folder on your `PATH` and keep `oromod` there: it
finds the rest of the SDK next to itself.

## The modules folder

The plugin's Settings (the gear) show the modules folder, and **Show
folder** opens it. By default it's `Oroboro/Oroboro Modular/modules` in
your user data folder (`%APPDATA%` on Windows, `~/Library/Application
Support` on macOS, `~/.local/share` on Linux). `oromod` uses the same
folder.

- Faust sources (`Name.dsp`) go at the top.
- Compiled modules (`Name.oromodule`, or a library with its `.sig`) go in
  `native/<Vendor>/`.

`oromod install FILE` puts each in the right place. The plugin checks
the folder about once a second, so you never need to restart it.

## Creating a Faust DSP module

```bash
oromod new faust Wobble --vendor "Your Name"   # a resonant low-pass with V/Oct
oromod check Wobble.dsp                        # compiles it, shows its jacks and knobs
oromod install Wobble.dsp
```

In the plugin, open **Add module**: Wobble is in the Faust group, under
Filter. Edit and save `Wobble.dsp` while the plugin plays and you'll hear
the change within a second. See [faust-modules.md](faust-modules.md).

## Creating a native/compiled module

```bash
oromod new native "Crunch Box" --vendor "Your Name"   # a Rust crate in crunch-box/
oromod build crunch-box --install
oromod check "crunch-box/Crunch Box.oromodule"
```

Turn on **Developer mode** in the plugin's Settings to load your own
unsigned builds. The module appears under Third Party Modules > Your
Name, as `native/Your Name/Crunch Box`. See
[native-modules.md](native-modules.md).

A Faust module can be compiled too: `oromod faust build Wobble.dsp
--install` makes `native/Your Name/Wobble`, which plays without Faust and
runs several times faster than the Faust DSP script.

For complete modules to learn from, see
[OroboroModularModules](https://github.com/OroboroModular/OroboroModularModules).

## Publishing

```bash
oromod login
oromod publish Wobble.dsp --summary "A wobbly low-pass"
oromod publish "crunch-box/Crunch Box.oromodule"
```

See [community-library.md](community-library.md) for what you need.
