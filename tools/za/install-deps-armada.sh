#!/bin/bash
# Layer the Fedora packages ZA Recomp needs to *build* on AYN Thor / Armada OS (bootc / rpm-ostree).
# After this finishes, reboot once, then run: tools/za/build-armada.sh
#
# Armada ships its own Mesa (*.armada RPMs: Turnip). Fedora's mesa-libEGL-devel / mesa-libgbm-devel
# conflict with those, so they are intentionally omitted. SDL3 is built with KMSDRM/EGL off
# (Wayland + X11 + Vulkan only) — enough for ZA Recomp.
set -euo pipefail

PKGS=(
    # toolchain
    clang lld cmake ninja-build pkgconf-pkg-config python3
    git curl
    # packaging / strip (dist-linux / AppImage helpers)
    patchelf llvm binutils zip
    # Vulkan loader headers (runtime loader + Turnip are already on the image)
    vulkan-loader-devel
    # SDL3 backend headers — Wayland / X11 / audio / udev (NOT mesa-*-devel)
    libX11-devel libXext-devel libXrandr-devel libXcursor-devel libXi-devel
    libXScrnSaver-devel libXfixes-devel libXtst-devel libxkbcommon-devel
    wayland-devel wayland-protocols-devel libdecor-devel
    libdrm-devel
    pulseaudio-libs-devel alsa-lib-devel
    pipewire-devel dbus-devel systemd-devel ibus-devel
)

if ! command -v rpm-ostree >/dev/null; then
    echo "install-deps-armada: rpm-ostree not found."
    echo "  On mutable Fedora: sudo dnf install ${PKGS[*]}"
    exit 1
fi

echo "install-deps-armada: layering ${#PKGS[@]} packages (needs network + a reboot afterwards)"
echo "  ${PKGS[*]}"
echo
echo "Note: mesa-libEGL-devel / mesa-libgbm-devel are skipped — they conflict with Armada's Mesa."
echo

sudo rpm-ostree install --allow-inactive "${PKGS[@]}"
echo
echo "Layer queued. Reboot to apply, then build:"
echo "  sudo systemctl reboot"
echo "  cd $(cd "$(dirname "$0")/../.." && pwd)"
echo "  tools/za/build-armada.sh"
