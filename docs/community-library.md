# The Community Library

The [Community Library](https://www.oroboro.space/library) is where users
find and share modules and patches for Oroboro Modular. Its modules show
up in the plugin's Add module (the Community group), and a patch that
uses one of your modules brings it along when it's downloaded.

You publish modules on the website (Community Library › Publish Module)
or with `oromod publish`.

## What you need

- An oroboro.space account with an **artist name** (shown as the author;
  your email never is) and an Oroboro Modular license.
- For compiled modules, an approved **developer account** (below).
- To publish only what you made or have the right to publish
  ([terms](https://www.oroboro.space/library/terms)).
- A licence: CC BY 4.0 (the default), CC BY-SA 4.0, CC0, GPL 3.0, MIT, or
  Free to use (anyone may play it, nobody may share, sell or change it).
  Modules built from GPL code must be GPL, and a GPL compiled module must
  link to its source. Once someone has downloaded your module, you can
  only change its licence to a more open one.

## What gets checked

| | You upload | Checks |
|---|---|---|
| Faust modules | the `.dsp` | the rules in [faust-modules.md](faust-modules.md), then a compile with Faust 2.88.0 in a sandbox |
| Compiled modules | the `.oromodule` | every build is a real library for its platform, then the library **signs** it |

Publish a module before any patch that uses it.

## Compiled modules

```bash
oromod build crunch-box
oromod publish "crunch-box/Crunch Box.oromodule" --summary "A drive with a crunch"
```

The library signs each build, so users can play your module without
turning on Developer mode. Publishing again adds a new version.

A build only runs on the platform it was built for (Windows, Linux,
macOS). Build on each platform into the same module file and publish
once it has them all:

```bash
oromod build crunch-box --out dist   # on Windows, then on Linux into the same dist/
oromod check "dist/Crunch Box.oromodule"
oromod publish "dist/Crunch Box.oromodule"
```

A Faust source built with `oromod faust build` is a compiled module like
any other. Publishing the `.dsp` instead works on every platform, for
users who have Faust installed.

### Developer accounts and vendor names

The library can't look inside a build, so compiled modules come only from
approved developers. To become one, apply on your oroboro.space account
page under Developer, with your **vendor name** and what you make.
Faust modules don't need this.

Your vendor name is part of every compiled module's name,
`native/<Vendor>/<Name>`, so two vendors can each have a "Delay". Set it
in your build (Rust: `Spec::new(…).vendor("Oroboro")`; Faust:
`declare vendor "Oroboro";`; a Rack plugin: its `brand`). The library only
accepts modules under your own vendor name and signs it into every
build, and Add module lists them under Third Party Modules › your vendor
name. Once your account is approved, the vendor name is fixed; write to
Oroboro if it really has to change.

## With oromod

```bash
oromod login
oromod publish Wobble.dsp --summary "A wobbly low-pass" --tags filter,lofi
oromod publish Wobble.dsp --version-of https://www.oroboro.space/library/faust/wobble --changelog "Smoother"
oromod publish "dist/Crunch Box.oromodule"
```

After publishing, add a description, tags and a picture on the module's
page at oroboro.space.
