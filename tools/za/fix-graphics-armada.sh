#!/bin/bash
# Soften video settings + document the Adreno/Turnip guest-window fix for AYN Thor / Armada.
#
# Rebuild required after git pull (dma-buf window is compiled into lasrecomp):
#   tools/za/build-armada.sh
#
# Proven Thor path (do not change casually):
#   top 2048 MB dma-buf + uncached + XFB mirror ON + direct gmem VBs
# Whole-window / mirror-off / force-copy each regressed the character creator.
# Override window size only if allocate fails:
#   LARECOMP_TURNIP_WINDOW_MB=1600 ./build/linux/aarch64/za/lasrecomp
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

touch "$INI"
patch_key() {
  local k="$1" v="$2"
  if grep -q "^${k}[[:space:]]*=" "$INI" 2>/dev/null; then
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
patch_key gpu_xfb_mirror on

if [ -d "$DATA/cache" ]; then
  rm -rf "$DATA/cache"
  echo "fix-graphics-armada: cleared $DATA/cache (shader / pipeline cache)"
fi

echo "fix-graphics-armada: updated $INI"
echo
echo "Thor: top 2048 MB dma-buf + uncached + XFB mirror ON + direct gmem VBs"
echo
echo "Rebuild:"
echo "  tools/za/build-armada.sh"
echo "  $B/lasrecomp"
echo
echo "Good log lines:"
echo "  [vk] turnip window: sharing top 2048 MB …"
echo "  [vk] turnip window: memory type … (uncached)"
echo "  [vk] transform feedback mirror on (Adreno/Turnip Linux …)"
echo "  vertices … cached … host   ← host must be non-zero in the creator"
echo
echo "If still broken, send $DATA/lasrecomp.log"
