# syntax=docker/dockerfile:1

# The module build image: oromod, Faust, and the toolchains that build a
# module (a Rust crate, or a Faust source) for Windows and Linux, each for
# x86-64 and ARM64. The toolchain is ci/Dockerfile's (Debian 12's glibc for
# Linux, GCC's MinGW for Windows x86-64, llvm-mingw for Windows ARM64), and
# Faust the plugin's (2.88.0, built from its source): keep them alike.
#
# scripts/build-all.ps1 (Windows) or scripts/build-all.sh (Linux, macOS),
# run in a module's folder, make this image the first time and build the
# module in it (docs/native-modules.md, "Every platform at once"). By hand:
#
#   docker build --platform linux/amd64 -f docker/Dockerfile --build-arg OROMOD_VERSION=<version> -t oromod-build .
#   docker run --rm --platform linux/amd64 -v <a folder>:/src -w /src/<the module, in it> oromod-build build-all

# ---- Faust, for a module written in Faust -----------------------------------------
FROM debian:bookworm AS faust
ARG FAUST=2.88.0
ARG FAUST_SHA256=e4e175cf236924b5b7d4784cbb8c50cc01e211159e169655dd9e6d8f92b871d9
RUN apt-get update \
 && apt-get install -y --no-install-recommends ca-certificates curl cmake make g++ pkg-config \
 && rm -rf /var/lib/apt/lists/* \
 && curl -fsSL -o /tmp/faust.tar.gz "https://github.com/grame-cncm/faust/releases/download/$FAUST/faust-$FAUST.tar.gz" \
 && echo "$FAUST_SHA256  /tmp/faust.tar.gz" | sha256sum -c - \
 && mkdir /tmp/faust && tar -xzf /tmp/faust.tar.gz -C /tmp/faust --strip-components=1 && rm /tmp/faust.tar.gz \
 && make -C /tmp/faust -j"$(nproc)" compiler \
 && make -C /tmp/faust install PREFIX=/opt/faust \
 && rm -rf /tmp/faust \
 && /opt/faust/bin/faust --version | grep -q "Version $FAUST"

# ---- Toolchain -------------------------------------------------------------------
FROM rust:1.95-bookworm AS toolchain
ARG LLVM_MINGW=20260922
ARG LLVM_MINGW_SHA256=bb7bb7654b33d5aa8712acb837c963b2e0c56352560c76105270a3268c665c21
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      gcc-aarch64-linux-gnu g++-aarch64-linux-gnu libc6-dev-arm64-cross \
      gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64 binutils-mingw-w64-x86-64 \
      xz-utils unzip \
 && rm -rf /var/lib/apt/lists/* \
 && rustup target add aarch64-unknown-linux-gnu x86_64-pc-windows-gnu aarch64-pc-windows-gnullvm \
 && curl -fsSL -o /tmp/llvm-mingw.tar.xz \
      "https://github.com/mstorsjo/llvm-mingw/releases/download/$LLVM_MINGW/llvm-mingw-$LLVM_MINGW-ucrt-ubuntu-22.04-x86_64.tar.xz" \
 && echo "$LLVM_MINGW_SHA256  /tmp/llvm-mingw.tar.xz" | sha256sum -c - \
 && mkdir /opt/llvm-mingw \
 && tar -xJf /tmp/llvm-mingw.tar.xz -C /opt/llvm-mingw --strip-components=1 \
 && rm /tmp/llvm-mingw.tar.xz
# The C++ runtimes linked in, not beside: without their import libraries
# the linker takes the static ones, for any crate that brings C++ in.
RUN find /usr/lib/gcc/x86_64-w64-mingw32 /opt/llvm-mingw/aarch64-w64-mingw32/lib \
      \( -name 'libstdc++.dll.a' -o -name 'libc++.dll.a' -o -name 'libunwind.dll.a' \) -print -delete
ENV PATH=$PATH:/opt/llvm-mingw/bin \
    CARGO_TARGET_AARCH64_UNKNOWN_LINUX_GNU_LINKER=aarch64-linux-gnu-gcc \
    CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER=x86_64-w64-mingw32-gcc \
    CARGO_TARGET_AARCH64_PC_WINDOWS_GNULLVM_LINKER=aarch64-w64-mingw32-clang \
    CARGO_TARGET_AARCH64_PC_WINDOWS_GNULLVM_RUSTFLAGS="-C target-feature=+crt-static" \
    CC_aarch64_unknown_linux_gnu=aarch64-linux-gnu-gcc \
    CXX_aarch64_unknown_linux_gnu=aarch64-linux-gnu-g++ \
    CC_x86_64_pc_windows_gnu=x86_64-w64-mingw32-gcc \
    CXX_x86_64_pc_windows_gnu=x86_64-w64-mingw32-g++ \
    CC_aarch64_pc_windows_gnullvm=aarch64-w64-mingw32-clang \
    CXX_aarch64_pc_windows_gnullvm=aarch64-w64-mingw32-clang++

# ---- oromod, and what builds a module ------------------------------------------
# oromod is built from its source where the SDK has it (its own repository);
# a published SDK has none, and takes the Linux oromod of its version from
# the SDK's releases (OROMOD_VERSION, which the scripts pass).
FROM toolchain AS build
COPY --from=faust /opt/faust /opt/faust
ENV PATH=$PATH:/opt/faust/bin \
    OROBORO_SDK=/sdk
ARG OROMOD_VERSION
ARG RELEASES=https://github.com/OroboroModular/OroboroModularSDK/releases/download
WORKDIR /sdk
COPY . .
RUN --mount=type=cache,target=/usr/local/cargo/registry \
    set -eu; \
    if [ -f oromod/Cargo.toml ]; then \
        cargo build --release --locked -p oromod; \
        cp target/release/oromod /usr/local/bin/oromod; \
        rm -rf target; \
    else \
        test -n "${OROMOD_VERSION:-}" || { echo "Build with --build-arg OROMOD_VERSION=<version>" >&2; exit 1; }; \
        curl -fsSL -o /tmp/sdk.zip "$RELEASES/v$OROMOD_VERSION/OroboroModularSDK-$OROMOD_VERSION-linux-x64.zip"; \
        unzip -q -j /tmp/sdk.zip "*/bin/oromod" -d /usr/local/bin; \
        chmod +x /usr/local/bin/oromod; \
        rm /tmp/sdk.zip; \
    fi; \
    oromod --version
# (with Unix line ends, whatever the checkout it came from has; and the
# folder oromod faust build writes its crate in open to any user, as the
# scripts run it as yours on Linux)
RUN sed 's/\r$//' docker/build-all.sh > /usr/local/bin/build-all \
 && chmod +x /usr/local/bin/build-all \
 && mkdir -p /sdk/target && chmod 777 /sdk/target
WORKDIR /src
CMD ["build-all"]
