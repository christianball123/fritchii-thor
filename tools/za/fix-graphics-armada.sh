#!/bin/bash
# Soften video settings + document the Adreno/Turnip guest-window fix for AYN Thor / Armada.
#
# Rebuild required after git pull (dma-buf window is compiled into lasrecomp):
#   tools/za/build-armada.sh
#
# Default: top 1536 MB of Z-A's guest window shared with Turnip (not the full 3.2 GB —
# that OOM'd Thor and crashed boot #271). Override:
#   LARECOMP_TURNIP_WINDOW_MB=1024 ./build/linux/aarch64/za/lasrecomp
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
patch_key gpu_xfb_mirror on

# Stale pipelines from a previous bad run can stick; safe to clear
if [ -d "$DATA/cache" ]; then
  rm -rf "$DATA/cache"
  echo "fix-graphics-armada: cleared $DATA/cache (shader / pipeline cache)"
fi

echo "fix-graphics-armada: updated $INI"
echo
echo "Thor best-known so far: dma-buf window + XFB mirror ON"
echo "  (mirror OFF collapsed the character to a solid colour block)"
echo
echo "Rebuild:"
echo "  tools/za/build-armada.sh"
echo "  $B/lasrecomp"
echo
echo "Good log lines:"
echo "  [vk] guest heap imported …"
echo "  [vk] transform feedback mirror on (Adreno/Turnip Linux …)"
echo
echo "No rebuild — force mirror on with current binary:"
echo "  LARECOMP_VK_XFB_MIRROR=1 $B/lasrecomp"
echo
echo "If allocate fails: LARECOMP_TURNIP_WINDOW_MB=1536"
echo "If still broken, send $DATA/lasrecomp.log (gmem / turnip / xfb / mirror)"
