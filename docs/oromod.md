# oromod

The SDK's command-line tool, in the SDK's `bin` folder. It checks and
plays modules with the plugin's own engine, so what it reports is what
the plugin will do.

Every command accepts `--modules DIR` to use a different modules folder.
By default it uses the plugin's, as set in the plugin's Settings.

## `oromod modules [--json | --markdown]`

Lists every module the plugin has, built-in and yours, by category.
`--json` gives each module's full interface; `--markdown` a readable
reference.

## `oromod check FILE…`

Shows what the plugin and the Community Library make of each file, and
exits with an error if anything is wrong:

- **`.dsp`**: the Community Library's rules, and (with Faust installed)
  whether it compiles and what jacks and knobs it has.
- **`.dll`, `.so`, `.dylib`**: loads it like the plugin does, plays a
  second of sound through it, and reports the cost per sample, plus its
  state, menu and panel if it has them.
- **`.oromodule`**: its interface, version and description, which
  platforms it has builds for, whether the Community Library would accept
  it, and a check of this platform's build.
- **`.orobundle`**: each module in it, with its platforms.

## `oromod new faust NAME [--dir DIR] [--vendor VENDOR]`

Writes `NAME.dsp`, a starter module with a V/Oct input and a few knobs.
`--vendor` is who makes it (default `Me`).

## `oromod new native NAME [--dir DIR] [--vendor VENDOR]`

Writes a Rust crate for the compiled module `native/VENDOR/NAME`: a
`Cargo.toml` using `oroboro-module`, a `src/lib.rs` with a soft-clipping
drive and a test, and a `.gitignore`.

## `oromod build [DIR] [--out DIR] [--install]`

Builds a Rust module crate (`cargo build --release`) into
`<Name>.oromodule`, with the version, description, licence and homepage
from `Cargo.toml`. If the file already exists, its builds for other
platforms are kept, so building on each platform gives one file for all
of them. `--install` also copies it into the modules folder.

## `oromod faust build SOURCE.dsp [--out DIR] [--install]`

Compiles a Faust source with Faust's Rust backend into `<Name>.oromodule`,
named `native/<Vendor>/<Name>` by the source's `declare vendor` (or
`declare author`). Needs the `faust` compiler on `PATH` or in `FAUST`.

## `oromod rack build [DIR]`, `oromod rack list [DIR]`

Builds a VCV Rack 2 plugin into one module file per Rack module
([rack-modules.md](rack-modules.md)). `DIR` is the plugin's folder or a
folder with an `oroboro-rack.json`. Options: `--out DIR` (default
`build/`), `--only A,B`, `--jobs N`, `--clean`, `--install`. `list` shows
the modules and what each will be called. Needs `g++` or `clang++`.

`oromod rack ui [DIR]` writes starter panels in the plugin's style
(`oroboro/<Slug>.json`) for modules that don't have one.

## `oromod pack FILE… [-o OUT.oromodule] [--version V] [--description D] [--license L] [--url U]`

Makes or extends a module file from libraries built some other way (a C
module, a macOS universal binary). Give this platform's library first,
since oromod loads it to read the interface.

## `oromod bundle FILE… -o OUT.orobundle [--name N] [--vendor V] [--version X]`

Packs several module files into one bundle, to install or pass on as a
set.

## `oromod install FILE…`

Copies Faust sources into the modules folder, and module files, bundles
and libraries (with their `.sig`) into `native/<Vendor>/`.

## `oromod login`, `oromod logout`

Signs in to oroboro.space with a code you confirm in the browser. No
password is entered here.

## `oromod publish FILE…`

Publishes Faust sources, module files, libraries and bundles to the
Community Library ([community-library.md](community-library.md)).
Compiled modules need an approved developer account; the library signs
each build with your vendor name. Publishing a module you've published
before adds a new version.

Options:

- `--licence`: `CC-BY-4.0` (the default for Faust), `CC-BY-SA-4.0`,
  `CC0-1.0`, `GPL-3.0-or-later`, `GPL-3.0-only`, `MIT` or `Free-to-use`
  (anyone may play it, nobody may share, sell or change it). For compiled
  modules oromod asks if you don't say, and refuses to guess when it
  can't ask. Modules built from GPL code must use the GPL.
- `--source URL`: where the source is. Required for GPL compiled modules.
- `--summary`, `--tags a,b`, `--unlisted`, `--version-of ID-OR-PAGE`,
  `--changelog`.

## Environment

| Variable | |
|---|---|
| `OROBORO_DATA_DIR` | The plugin's data folder (settings, default modules folder). |
| `OROBORO_SDK` | The SDK folder, if oromod isn't in its `bin`. |
| `OROMOD_CONFIG_DIR` | Where oromod keeps its sign-in. |
| `CXX`, `CC`, `AR` | Compilers and archiver for `rack build`. |
| `FAUST` | The `faust` compiler. |
