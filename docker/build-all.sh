#!/usr/bin/env bash
# In the module build image (docker/Dockerfile), in a module's folder (a
# Rust crate, or a Faust source: one .dsp and no Cargo.toml): builds it for
# Linux and Windows, each for x86-64 and ARM64, into one module file
# (build/<Name>.oromodule, or the folder given).
#
# Linux x86-64 is this container's own platform: `oromod build` (`oromod
# faust build`) makes the module file from it (it reads the module's
# interface from the library it loads). The other three are cross-built
# with cargo (a Faust source's from the crate `oromod faust build` wrote)
# and added with `oromod pack`; a build already in the file for a platform
# (one made on Windows, say) is replaced. The container's builds go to
# target/oromod-build, apart from those of the computer the folder is on.
set -euo pipefail

out=${1:-build}
export CARGO_TARGET_DIR=${CARGO_TARGET_DIR:-$(pwd)/target/oromod-build}
mkdir -p "$out"
out=$(cd "$out" && pwd)

if [ -f Cargo.toml ]; then
    echo "== linux-x64"
    oromod build . --out "$out"
    # the crate, and its library's name as cargo names its file
    manifest=$(pwd)/Cargo.toml
    lib=$(cargo metadata --no-deps --format-version 1 | python3 -c '
import json, sys
package = json.load(sys.stdin)["packages"][0]
print(next(t["name"] for t in package["targets"] if "cdylib" in t["kind"]).replace("-", "_"))')
else
    shopt -s nullglob
    sources=(*.dsp)
    [ "${#sources[@]}" = 1 ] || { echo "No Cargo.toml and not one .dsp in $(pwd): run it in a module's folder." >&2; exit 1; }
    echo "== linux-x64 (Faust)"
    oromod faust build "${sources[0]}" --out "$out"
    # the crate oromod faust build wrote, as it names it
    safe=$(basename "${sources[0]}" .dsp | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9]/_/g')
    manifest=${OROBORO_SDK:-/sdk}/target/oromod-faust/$safe/Cargo.toml
    lib=oroboro_faust_$safe
fi
file=$(ls -t "$out"/*.oromodule | head -n 1)

builds=()
for build in linux-arm64:aarch64-unknown-linux-gnu windows-x64:x86_64-pc-windows-gnu \
             windows-arm64:aarch64-pc-windows-gnullvm; do
    name=${build%%:*}; triple=${build#*:}
    echo "== $name"
    cargo build --release --target "$triple" --manifest-path "$manifest"
    case $name in
        windows-*) builds+=("$CARGO_TARGET_DIR/$triple/release/$lib.dll") ;;
        *) builds+=("$CARGO_TARGET_DIR/$triple/release/lib$lib.so") ;;
    esac
done
oromod pack "$file" "${builds[@]}" --out "$file"
echo
oromod check "$file"
