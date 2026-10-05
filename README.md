# Oroboro Modular SDK

Build your own modules for **Oroboro Modular**, Oroboro's modular
synthesizer plugin, and share them in the
[Community Library](https://www.oroboro.space/library).

You can make two kinds of module:

- **Faust modules**: a single `.dsp` file in [Faust](https://faust.grame.fr).
  The plugin compiles it on the fly, so users need Faust 2.88 installed.
- **Compiled modules**: a native library, written in Rust or C, built
  from a Faust file, or compiled from a **VCV Rack 2 plugin**. They're the
  fastest, and they can have their own panels and screens.

## Examples

[OroboroModularModules](https://github.com/OroboroModular/OroboroModularModules) has Oroboro's own modules,
built with this SDK: a spectrum analyser with its own panel and screen, a
through-zero oscillator, delays and a reverb in Rust, and a ping-pong
delay in Faust. They're the best place to see how a module is put
together.

## What's inside

```
bin/oromod           the command-line tool
docs/                the guides
native/              the module ABI (C header), the Rust crate, an example
rack/                Rack 2's plugin interface, for building Rack modules
examples/faust/      two Faust examples
```

## Quick start

Put the `bin` folder on your `PATH`. Then:

```bash
oromod new faust Wobble --vendor "Your Name"   # a starter Faust module
oromod check Wobble.dsp                        # see what the plugin will make of it
oromod install Wobble.dsp                      # it's now in the plugin's Add module
oromod new native "Crunch Box"                 # or start a compiled module in Rust
```

Your own compiled builds load when **Developer mode** is on in the
plugin's Settings. To publish compiled modules, apply for a developer
account on your oroboro.space account page; the library then signs your
builds so they load for everyone.

## Bringing over VCV Rack 2 modules

`oromod rack build` compiles a Rack 2 plugin's source, unchanged, into one
Oroboro module per Rack module. The same folder still builds for Rack.

```bash
cd MyPlugin                       # where plugin.json and the Makefile are
oromod rack build . --install     # one .oromodule per module, ready to try
oromod publish build/*.oromodule  # share them in the Community Library
```

You need `g++` or `clang++` (on Windows, MinGW-w64). Your panel, knobs,
jacks, lights, menus, saved state and custom-drawn widgets all come over.
Expanders and 16-channel polyphonic cables don't. Most Rack plugins are
GPL, so their builds must be published under the GPL with a link to the
source. The full guide: [docs/rack-modules.md](docs/rack-modules.md).

## Guides

1. [Getting started](docs/getting-started.md)
2. [Kinds of modules](docs/modules.md) and [signals, voices and timing](docs/signals.md)
3. [Faust modules](docs/faust-modules.md)
4. [Compiled modules](docs/native-modules.md)
5. [Modules from VCV Rack sources](docs/rack-modules.md)
6. [Publishing to the Community Library](docs/community-library.md)
7. [oromod reference](docs/oromod.md)

## Versions

This SDK works with Oroboro Modular 0.1.0 and later. The module ABI is
version 1; future versions only add to it, so modules you build today
keep working. See [CHANGELOG.md](CHANGELOG.md) for changes.

## Licence

The SDK's source files are MIT-licensed: see [LICENSE](LICENSE). The
`oromod` program in `bin/` has a licence of its own, `bin/LICENSE.txt`: it's
free to use, including for modules you sell. What you make with the SDK is
yours.

VCV Rack is a trademark of VCV. This project isn't affiliated with or
endorsed by VCV.
