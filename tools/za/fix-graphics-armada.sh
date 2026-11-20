#!/bin/bash
# Soften video settings for AYN Thor / Armada (Adreno + Mesa Turnip).
#
# Rebuild required after git pull:
#   tools/za/build-armada.sh
#
# Target: stop city background flicker from incoherent sparse-gmem VB fetches.
#   XFB mirror ON, draw-sized xm_lookup for skinning, CPU-copy other VB/IB after
#   dma-buf READ sync. Do NOT bind sparse/piece dma-buf as VB by default.
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
  echo "fix-graphics-armada: cleared $DATA/cache"
fi

echo "fix-graphics-armada: updated $INI"
echo
echo "Rebuild: tools/za/build-armada.sh && $B/lasrecomp"
echo
echo "Good log lines (stable city backgrounds):"
echo "  [vk] transform feedback mirror on (Armada/Turnip; … draw-sized + CPU VB copy …)"
echo "  [vk] Turnip: draw-sized vertex looks …"
echo "  [vk] Turnip: vertices/indices CPU-copied to ring after dma-buf READ sync …"
echo
echo "Bad (background flicker / green strips):"
echo "  binding sparse dma-buf gmem as VB/IB"
echo "  full-pool vertex binds"
echo "  → unset LARECOMP_TURNIP_GMEM_VB / LARECOMP_XM_FULL_POOL"
