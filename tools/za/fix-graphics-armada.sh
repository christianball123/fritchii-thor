#!/bin/bash
# Soften video settings + document the Adreno character-creator fix for AYN Thor / Armada.
#
# Without rebuilding (try first — env overrides the 256 MB heap window):
#   LARECOMP_VK_GMEM_WINDOW=0 ./build/linux/aarch64/za/lasrecomp
#
# After git pull + rebuild of lasrecomp, Adreno defaults to a whole-heap import.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DATA="${XDG_DATA_HOME:-$HOME/.local/share}/zarecomp"
INI="$DATA/larecomp.ini"
mkdir -p "$DATA"

ARCH="$(uname -m)"
case "$ARCH" in aarch64|arm64) ARCH=aarch64 ;; x86_64|amd64) ARCH=x86_64 ;; esac
B="$ROOT/build/linux/$ARCH/za"
if [ -d "$B" ] && [ -f "$ROOT/tools/za/za-default-armada.ini" ]; then
  cp -f "$ROOT/tools/za/za-default-armada.ini" "$B/za-default.ini"
  echo "fix-graphics-armada: wrote $B/za-default.ini"
fi

# Patch / create player settings (keep unknown keys; override the heavy desktop preset)
touch "$INI"
patch_key() {
  local k="$1" v="$2"
  if grep -q "^${k}[[:space:]]*=" "$INI" 2>/dev/null; then
    # portable sed in-place
    sed -i "s|^${k}[[:space:]]*=.*|${k} = ${v}|" "$INI"
  else
    printf '%s = %s\n' "$k" "$v" >>"$INI"
  fi
}
patch_key operation_mode handheld
patch_key upscaler bilinear
patch_key antialias off
patch_key resolution 1x
patch_key fps60 0
patch_key fps60_lock 0
patch_key aniso16 0
patch_key show_fps 1
patch_key gpu_xfb_mirror auto

# Stale pipelines from the broken window can stick; safe to clear
if [ -d "$DATA/cache" ]; then
  rm -rf "$DATA/cache"
  echo "fix-graphics-armada: cleared $DATA/cache (shader / pipeline cache)"
fi

echo "fix-graphics-armada: updated $INI"
echo
echo "Try without rebuilding first:"
echo "  LARECOMP_VK_GMEM_WINDOW=0 $B/lasrecomp"
echo
echo "Then rebuild so the Adreno whole-heap default is compiled in:"
echo "  tools/za/build-armada.sh"
echo "  # (or at least rebuild lasrecomp after git pull)"
echo
echo "If the character is still solid blocks, grab the top of:"
echo "  $DATA/lasrecomp.log"
echo "  (look for lines: [vk] using …, gmem window, host import, xfb mirror, depth24)"
