#!/bin/bash
# Soften video settings for AYN Thor / Armada (Adreno + Mesa Turnip).
#
# Rebuild required after git pull:
#   tools/za/build-armada.sh
#
# Target: restore the 8066 path that showed head + visible (rainbow) body:
#   XFB mirror ON, full-pool vertex binds, sparse dma-buf gmem as VB/IB.
#   Later piece/always-inside/CPU-copy experiments went head-only or empty.
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
echo "Good log lines (8066 path):"
echo "  [vk] transform feedback mirror on (Armada/Turnip; … 8066 full-pool+gmem VB path …)"
echo "  [vk] Turnip: full-pool vertex binds (8066 path …)"
echo "  [vk] Turnip: binding sparse dma-buf gmem as VB/IB (8066 path …)"
echo "  creator: head + visible body (may be rainbow-tinted); cached + large host"
echo
echo "Bad:"
echo "  always-inside / per-piece VB / CPU copy / 0MB host with empty preview"
