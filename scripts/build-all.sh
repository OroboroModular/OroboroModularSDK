#!/usr/bin/env bash
# Builds the module in this folder (a Rust crate, or a Faust source) for
# Windows and Linux, each for x86-64 and ARM64, into one module file, in
# Docker (docker/Dockerfile; docs/native-modules.md, "Every platform at
# once").
#
#   cd my-module && ../OroboroModularSDK/scripts/build-all.sh [out folder] [--rebuild]
#
# The first time it makes the build image (oromod-build:<oromod's version>),
# which takes a while; --rebuild makes it again. The folder the module and
# the SDK are both in is mounted, so a path from the module to the SDK's
# crate holds in the container.
set -euo pipefail

sdk=$(cd "$(dirname "$0")/.." && pwd)
project=$(pwd)
out=build
rebuild=0
for arg in "$@"; do
    case $arg in
        --rebuild) rebuild=1 ;;
        *) out=$arg ;;
    esac
done
sources=$(find . -maxdepth 1 -name '*.dsp' -type f | wc -l)
[ -f Cargo.toml ] || [ "$sources" = 1 ] || { echo "No Cargo.toml here and not one .dsp: run it in a module's folder." >&2; exit 1; }
command -v docker > /dev/null || { echo "Docker isn't installed (docker.com)." >&2; exit 1; }

# oromod's version: its source's (the SDK's repository), else the oromod
# the published SDK comes with
if [ -f "$sdk/oromod/Cargo.toml" ]; then
    version=$(sed -n 's/^version *= *"\(.*\)"/\1/p' "$sdk/oromod/Cargo.toml" | head -n 1)
else
    version=$("$sdk/bin/oromod" --version | awk '{print $2}')
fi
image="oromod-build:$version"
if [ "$rebuild" = 1 ] || [ -z "$(docker image ls -q "$image")" ]; then
    echo "Making the build image $image (the first time takes a while)"
    docker build --platform linux/amd64 -f "$sdk/docker/Dockerfile" --build-arg "OROMOD_VERSION=$version" -t "$image" "$sdk"
fi

# the folder both are in
root=$project
while [ "$root" != / ] && [ "${sdk#"$root"/}" = "$sdk" ] && [ "$sdk" != "$root" ]; do
    root=$(dirname "$root")
done
rel=${project#"$root"}
rel=${rel#/}

# on Linux the files it writes are yours, not root's
user=()
if [ "$(uname -s)" = Linux ]; then
    user=(--user "$(id -u):$(id -g)" -e CARGO_HOME=/tmp/cargo)
fi
# (Git Bash on Windows: Docker takes the Windows path, and no path rewriting)
host=$root
command -v cygpath > /dev/null && host=$(cygpath -w "$root")
MSYS_NO_PATHCONV=1 docker run --rm --platform linux/amd64 ${user[@]+"${user[@]}"} \
    -v "$host:/src" -w "/src/$rel" "$image" build-all "$out"
