#!/bin/bash
# Soften video settings + document the Adreno/Turnip guest-window fix for AYN Thor / Armada.
#
# Rebuild required after git pull (dma-buf window is compiled into lasrecomp):
#   tools/za/build-armada.sh
#
# Default: whole Z-A guest phys window shared with Turnip (Android AHB layout), set up at
# gwin_pick. If allocate OOMs, the binary retries the top 2048 MB. Override:
#   LARECOMP_TURNIP_WINDOW_MB=2048 ./build/linux/aarch64/za/lasrecomp
#   LARECOMP_TURNIP_WINDOW=0 …   # disable share (debug)
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
# Linux Turnip needs the XFB mirror (Android AHB can leave it off). Whole-window + mirror off = invisible character.
patch_key gpu_xfb_mirror on

# Stale pipelines from a previous bad run can stick; safe to clear
if [ -d "$DATA/cache" ]; then
  rm -rf "$DATA/cache"
  echo "fix-graphics-armada: cleared $DATA/cache (shader / pipeline cache)"
fi

echo "fix-graphics-armada: updated $INI"
echo
echo "Thor: full dma-buf window + cached pieces + XFB mirror ON + direct gmem VBs"
echo
echo "Rebuild (required for character fix):"
echo "  tools/za/build-armada.sh"
echo "  $B/lasrecomp"
echo
echo "Good log lines:"
echo "  [vk] gmem window: whole guest window (Adreno/Turnip dma-buf)"
echo "  [vk] turnip window: memory type … (cached)"
echo "  [vk] turnip window: sharing whole … MB …"
echo "  [vk] transform feedback mirror on (Adreno/Turnip Linux …)"
echo
echo "If allocate falls back: … retrying top 2048 MB …"
echo "If still broken, send $DATA/lasrecomp.log (gmem / turnip / cached / mirror)"
