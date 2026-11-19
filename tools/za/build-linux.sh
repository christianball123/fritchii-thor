#!/bin/bash
# ZA Recomp on Linux, built natively (no container): the start screen, and with GAME=1 the game code from the
# player's own extracted/ (exefs/ + romfs.bin, as the start screen sets them up).
#
#   tools/za/build-linux.sh                 # this machine's arch -> build/linux/<arch>/za/lasrecomp
#   ARCH=aarch64 tools/za/build-linux.sh    # same, requires a native aarch64 host (AYN Thor / Armada OS)
#   GAME=1 tools/za/build-linux.sh          # + generated/ from extracted/exefs, then lasgame.so
#
# x86_64 needs AVX2 for the game code. aarch64 uses the Makefile's --simd asm path (like Apple Silicon).
# SDL3 3.4, an LGPL FFmpeg, shaderc and the Vulkan / VMA headers come from pinned sources
# (tools/linux/build_deps.sh). Portable AppImages: tools/linux/appimage.sh (podman) or tools/za/dist-linux.sh.
set -euo pipefail
cd "$(dirname "$0")/../.."
HOSTARCH=$(uname -m)
HOSTARCH=${HOSTARCH/arm64/aarch64}
ARCH=${ARCH:-$HOSTARCH}
case $ARCH in x86_64|aarch64) ;; *) echo "build-linux: ARCH must be x86_64 or aarch64 (got $ARCH)"; exit 1 ;; esac
L=$PWD/build/linux/$ARCH
P=$L/deps
B=$L/za
GENOPT="-O1 -DGUEST_VOLATILE"  # (the Windows release's game code flags, tools/za/dist.sh: part of the library id)
JOBS=${JOBS:-$(nproc)}

[ "$HOSTARCH" = "$ARCH" ] || {
    echo "build-linux: native build only (host=$HOSTARCH, ARCH=$ARCH)."
    echo "  On x86_64 for an aarch64 AppImage: tools/linux/appimage.sh aarch64 (needs podman)."
    echo "  On AYN Thor / Armada OS: run this script with no ARCH= (defaults to aarch64)."
    exit 1
}
if [ "$ARCH" = x86_64 ]; then
    grep -qw avx2 /proc/cpuinfo || echo "build-linux: warning: this CPU has no AVX2 - the game code needs it (x86-64-v3)"
fi

# ---- what the system must have: tools, and SDL3's backend headers (pkg-config module -> Debian / Ubuntu package)
# Fedora / Armada: the same libraries under different package names; PKGCHECK=0 skips the apt hint.
missing=()
for t in clang:clang ld.lld:lld cmake:cmake ninja:ninja-build pkg-config:pkg-config python3:python3 \
         curl:curl git:git; do
    command -v "${t%%:*}" >/dev/null || missing+=("${t#*:}")
done
# nasm is only used by some x86_64 deps paths; keep it required on x86_64
if [ "$ARCH" = x86_64 ]; then
    command -v nasm >/dev/null || missing+=(nasm)
fi
python3 -c 'import venv, ensurepip' 2>/dev/null || missing+=(python3-venv)
for m in x11:libx11-dev xext:libxext-dev xrandr:libxrandr-dev xcursor:libxcursor-dev xi:libxi-dev \
         xscrnsaver:libxss-dev xfixes:libxfixes-dev xtst:libxtst-dev xkbcommon:libxkbcommon-dev \
         wayland-client:libwayland-dev wayland-protocols:wayland-protocols libdecor-0:libdecor-0-dev egl:libegl-dev \
         libdrm:libdrm-dev gbm:libgbm-dev libpulse:libpulse-dev alsa:libasound2-dev libpipewire-0.3:libpipewire-0.3-dev \
         dbus-1:libdbus-1-dev libudev:libudev-dev ibus-1.0:libibus-1.0-dev; do
    pkg-config --exists "${m%%:*}" || missing+=("${m#*:}")
done
if [ ${#missing[@]} -gt 0 ] && [ "${PKGCHECK:-1}" != 0 ]; then
    echo "build-linux: missing system packages. On Debian/Ubuntu/Mint:"
    echo "    sudo apt install ${missing[*]}"
    echo "On Fedora / Armada OS, install the matching -devel packages (SDL3 backends: X11, Wayland, audio, udev),"
    echo "or set PKGCHECK=0 if deps/ is already built."
    exit 1
fi

# ---- SDL3, FFmpeg (LGPL), shaderc, Vulkan / VMA headers: pinned, hash-checked, built once
tools/linux/build_deps.sh $ARCH
# the Vulkan loader is always the system's (the GPU drivers' own); only its versioned name ships without libvulkan-dev
if [ ! -e "$P/lib/libvulkan.so" ] && ! pkg-config --exists vulkan; then
    if [ "$ARCH" = aarch64 ]; then
        sys=$(ldconfig -p | awk '/libvulkan\.so\.1 .*aarch64/ {print $NF; exit}')
        [ -n "$sys" ] || sys=$(ldconfig -p | awk '/libvulkan\.so\.1/ {print $NF; exit}')
    else
        sys=$(ldconfig -p | awk '/libvulkan\.so\.1 .*x86-64/ {print $NF; exit}')
    fi
    [ -n "$sys" ] || { echo "build-linux: no Vulkan loader (libvulkan1 / vulkan-loader)"; exit 1; }
    ln -sf "$sys" "$P/lib/libvulkan.so"
fi

# ---- the start screen (no game code inside: that is built from the player's files)
MAKE_ARCH=
[ "$ARCH" = aarch64 ] && MAKE_ARCH="ARCH=aarch64"
# shellcheck disable=SC2086
make -j"$JOBS" $MAKE_ARCH B="$B" DEPS="$P" SHADERC=-lshaderc_shared GENOPT="$GENOPT" "$B/lasrecomp"
if nm "$B/lasrecomp" | grep -q ' f_70'; then echo "build-linux: game code linked into the app"; exit 1; fi
echo "build-linux: $B/lasrecomp"

[ "${GAME:-0}" = 1 ] || exit 0

# ---- the game code, from the player's own extracted/exefs (never shipped: generated/ and lasgame.so stay here)
[ -f extracted/exefs/main ] || { echo "build-linux: GAME=1 needs extracted/exefs/main (your own game files)"; exit 1; }
if [ ! -f tools/venv/.ok ]; then
    python3 -m venv tools/venv
    tools/venv/bin/pip install -q capstone==5.0.9 unicorn==2.1.4 lz4==4.4.5
    touch tools/venv/.ok
fi
# Native aarch64 uses generated/ with --simd asm; only a cross build uses generated-aarch64/ (Makefile).
[ -f generated/main/main_build_id.txt ] || make $MAKE_ARCH gen
# Giant functions at -O0 first, as the start screen's build does (runtime/gamebuild.c cold_giant: a file of one
# function with over 1000 labels, unless it is in HOT_GIANTS, read from there): clang -O1 takes up to an hour on some.
# make then finds those objects up to date. JOBS lower on machines with little RAM (each clang can take ~1 GB).
# shellcheck disable=SC2086
FLAGS=$(make -s --eval='pv: ; @echo $(GENFLAGS)' $MAKE_ARCH GENOPT="$GENOPT" pv)
HOT=$(sed -n '/HOT_GIANTS\[\] = {/,/};/p' runtime/gamebuild.c | grep -o 'f_[0-9a-f]*' | sort -u)
giants=()
for f in generated/main/*.c generated/sdk/*.c; do
    # (definitions start with gen.py's FN_ATTR, older ones with "void")
    name=$(awk '/^(__attribute__.*\) )?void f_.*\(Cpu \*c\) \{/ {n++; match($0, /f_[0-9a-f]+/); s=substr($0, RSTART, RLENGTH)}
        /^L_/ {l++} END {if (n == 1 && l > 1000) print s}' "$f")
    [ -n "$name" ] && ! grep -qx "$name" <<<"$HOT" && giants+=("$f")
done
echo "build-linux: ${#giants[@]} giant functions at -O0"
printf '%s\n' "${giants[@]}" | xargs -P "$JOBS" -I{} sh -c 'o="$1/obj/${2#generated/}"; o="${o%.c}.o"; [ "$o" -nt "$2" ] ||
    { mkdir -p "$(dirname "$o")" && clang $3 -O0 -c "$2" -o "$o"; }' _ "$B" {} "$FLAGS"
# shellcheck disable=SC2086
make -j"$JOBS" $MAKE_ARCH B="$B" DEPS="$P" SHADERC=-lshaderc_shared GENOPT="$GENOPT" "$B/lasgame.so"
echo "build-linux: $B/lasgame.so"
