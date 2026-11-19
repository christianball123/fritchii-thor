#!/bin/bash
# Put ZA Recomp's substitute fonts next to lasrecomp so Play works without Switch fonts.
# Fixes the dialog: "No font for the game's text / Import the Switch fonts…"
#
# No rebuild needed. Run from the repo root (or pass the lasrecomp path):
#   tools/za/fix-fonts-armada.sh
#   tools/za/fix-fonts-armada.sh /path/to/lasrecomp
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

ARCH="$(uname -m)"
case "$ARCH" in
  aarch64|arm64) ARCH=aarch64 ;;
  x86_64|amd64) ARCH=x86_64 ;;
esac

BIN="${1:-}"
if [ -z "$BIN" ]; then
  for c in \
      "build/linux/$ARCH/za/lasrecomp" \
      "build/linux/aarch64/za/lasrecomp" \
      "build/linux/x86_64/za/lasrecomp"; do
    [ -x "$c" ] && BIN="$c" && break
  done
fi
if [ -z "${BIN}" ] || [ ! -x "$BIN" ]; then
  echo "fix-fonts-armada: lasrecomp not found."
  echo "  Pass its path:  tools/za/fix-fonts-armada.sh /path/to/lasrecomp"
  exit 1
fi
BIN="$(cd "$(dirname "$BIN")" && pwd)/$(basename "$BIN")"
DIR="$(dirname "$BIN")"

SRC_DV="$ROOT/third_party/fonts/DejaVu/DejaVuSans.ttf"
SRC_DB="$ROOT/third_party/fonts/DejaVu/DejaVuSans-Bold.ttf"
SRC_BZ="$ROOT/third_party/fonts/BIZUDPGothic/BIZUDPGothic-Regular.ttf"
SRC_BA="$ROOT/third_party/fonts/Baloo2/Baloo2-SemiBold.ttf"
for f in "$SRC_DV" "$SRC_BZ"; do
  [ -f "$f" ] || { echo "fix-fonts-armada: missing $f (incomplete checkout)"; exit 1; }
done

cp -f "$SRC_DV" "$DIR/DejaVuSans.ttf"
[ -f "$SRC_DB" ] && cp -f "$SRC_DB" "$DIR/DejaVuSans-Bold.ttf"
cp -f "$SRC_BZ" "$DIR/BIZUDPGothic-Regular.ttf"
[ -f "$SRC_BA" ] && cp -f "$SRC_BA" "$DIR/Baloo2-SemiBold.ttf"

# Also seed the data folder (pl.c tries ./DejaVuSans.ttf after a rebuild that includes that fallback)
DATA="${XDG_DATA_HOME:-$HOME/.local/share}/zarecomp"
mkdir -p "$DATA"
cp -f "$SRC_DV" "$DATA/DejaVuSans.ttf"
cp -f "$SRC_BZ" "$DATA/BIZUDPGothic-Regular.ttf"

echo "fix-fonts-armada: OK"
echo "  binary: $BIN"
echo "  fonts:  $DIR/DejaVuSans.ttf"
echo "          $DIR/BIZUDPGothic-Regular.ttf"
echo "  data:   $DATA/DejaVuSans.ttf"
echo
echo "Run the game again (same binary). Switch fonts are optional."
echo
echo "If it still fails (old binary that only looks beside itself — you already fixed that),"
echo "or you want a one-shot override without copying:"
echo "  LARECOMP_FONT=$SRC_DV $BIN"
