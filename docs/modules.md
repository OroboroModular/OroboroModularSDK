# Kinds of modules

Oroboro Modular has its own modules built in. With this SDK you can add
two kinds of your own:

| Kind | What it is | Example |
|---|---|---|
| `faust/<Name>` | A Faust `.dsp` file in your modules folder. The plugin compiles it while it runs. | `faust/TapeEcho` |
| `native/<Vendor>/<Name>` | A compiled library built against the module ABI: Rust, C, a Faust file built with `oromod faust build`, or a VCV Rack plugin built with `oromod rack build`. | `native/Oroboro/Fold` |

Patches refer to modules by these names, so don't rename a module once
patches use it. `oromod modules` lists everything the plugin has,
including yours.

## Which one to make

- **Faust** is the quickest way to new DSP: oscillators, filters,
  effects, envelopes, all in one file. Users need Faust installed to play
  the source, but `oromod faust build` turns it into a compiled module
  that doesn't. See [faust-modules.md](faust-modules.md).
- **Compiled** is for things Faust can't express well (sequencers, state
  machines, lookups), for speed, for a custom panel or screen, and for
  bringing VCV Rack modules over. See [native-modules.md](native-modules.md)
  and [rack-modules.md](rack-modules.md).
