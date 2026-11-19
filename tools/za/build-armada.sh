#!/bin/bash
# Build ZA Recomp on AYN Thor / Armada OS (native linux-aarch64).
#
#   tools/za/build-armada.sh           # start screen + runtime -> build/linux/aarch64/za/lasrecomp
#   GAME=1 tools/za/build-armada.sh    # also compile your own extracted game (needs extracted/exefs/main)
#   DIST=1 tools/za/build-armada.sh    # AppImage + zip (+ deb unless DEB=0)
#
# See docs/armada-thor.md. No game files are downloaded or shipped.
set -euo pipefail
cd "$(dirname "$0")/../.."
HOST=$(uname -m)
HOST=${HOST/arm64/aarch64}
[ "$HOST" = aarch64 ] || {
    echo "build-armada: this script is for native aarch64 (Thor / Armada). Host is $HOST."
    echo "  On a PC: tools/linux/appimage.sh aarch64 (podman) or use the Windows/Linux x86_64 build."
    exit 1
}
export ARCH=aarch64
PAGESIZE=$(getconf PAGESIZE 2>/dev/null || echo 0)
if [ "$PAGESIZE" != 4096 ] && [ "$PAGESIZE" != 16384 ]; then
    echo "build-armada: warning: page size $PAGESIZE (only 4K and 16K are supported; 64K kernels will not work)"
fi

# Friendly hint before the full pkg-config scan fails on a fresh Armada image
if [ "${PKGCHECK:-1}" != 0 ] && ! pkg-config --exists x11 2>/dev/null; then
    echo "build-armada: host build headers not found (typical on a fresh Armada image)."
    echo "  Layer them, reboot once, then re-run this script:"
    echo "    tools/za/install-deps-armada.sh && sudo systemctl reboot"
    echo
fi

if [ "${DIST:-0}" = 1 ]; then
    # Armada is Fedora-based: .deb is optional. ZIP is the reliable install path.
    DEB=${DEB:-0} ZAIMPORT=${ZAIMPORT:-1} ZIP=${ZIP:-1} tools/za/dist-linux.sh
    echo "build-armada: packaged under build/dist/"
    exit 0
fi

GAME=${GAME:-0} tools/za/build-linux.sh
echo
echo "Run from the repo root:"
echo "  ./build/linux/aarch64/za/lasrecomp"
echo "Data dir: ~/.local/share/zarecomp"
echo "With your own dump set up, compile the game once: GAME=1 tools/za/build-armada.sh"
echo "Or use the start screen's Set up the game (needs zaimport from DIST=1, or extracted/ already)."
