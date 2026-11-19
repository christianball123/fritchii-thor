#!/bin/bash
# Put the game-code generator + compiler next to lasrecomp so the start screen's
# "Set up the game" / "Compile now" works from a source tree (not only DIST=1 AppImages).
#
# Creates under build/linux/<arch>/za/:
#   lasgen/lasgen   -> wrapper around tools/venv + tools/gen.py
#   zig/            -> symlink to build/zig (fetched if missing)
#   gamelib-src/    -> headers the compiler includes
#
# Usage (from repo root, after tools/za/build-linux.sh):
#   tools/za/prepare-gamebuild.sh
#   tools/za/prepare-gamebuild.sh aarch64
set -euo pipefail
cd "$(dirname "$0")/../.."
HOST=$(uname -m); HOST=${HOST/arm64/aarch64}
ARCH=${1:-${ARCH:-$HOST}}
B=$PWD/build/linux/$ARCH/za
[ -x "$B/lasrecomp" ] || {
    echo "prepare-gamebuild: missing $B/lasrecomp — run tools/za/build-armada.sh (or build-linux.sh) first"
    exit 1
}

echo "prepare-gamebuild: tools for $B"

# 1. Python generator (capstone / unicorn / lz4)
if [ ! -f tools/venv/.ok ]; then
    python3 -m venv tools/venv
    # aarch64 often has no unicorn wheel: allow a long source build
    tools/venv/bin/pip install -q --upgrade pip setuptools wheel
    tools/venv/bin/pip install -q capstone==5.0.9 unicorn==2.1.4 lz4==4.4.5
    touch tools/venv/.ok
fi
[ -x tools/venv/bin/python ] || { echo "prepare-gamebuild: tools/venv/bin/python missing"; exit 1; }
[ -f tools/gen.py ] || { echo "prepare-gamebuild: tools/gen.py missing"; exit 1; }

# 2. zig (start-screen compiler; also used when not in "dev mode")
[ -x build/zig/zig ] || ZIG_TARGET=$ARCH-linux tools/fetch_zig.sh

# 3. Layout next to lasrecomp (runtime/gamebuild.c find_tools / rt_resource)
mkdir -p "$B/lasgen" "$B/gamelib-src"
ROOT=$PWD
cat > "$B/lasgen/lasgen" <<EOF
#!/bin/bash
# Thin wrapper: same CLI as the PyInstaller lasgen helper (tools/gen.py).
exec "$ROOT/tools/venv/bin/python" "$ROOT/tools/gen.py" "\$@"
EOF
chmod +x "$B/lasgen/lasgen"

rm -rf "$B/zig"
ln -sfn "$ROOT/build/zig" "$B/zig"

cp -f runtime/cpu.h runtime/simd.h runtime/simd_fp.h runtime/gamelib.h runtime/modlib.h \
      tools/gamelib_entry.c tools/modlib_entry.c runtime/hle/hle_symbols.txt \
      "$B/gamelib-src/"

# Bundled substitute fonts (runtime/hle/pl.c): next to the exe so Play works from any cwd /
# Game Mode shortcut — without these it FATAL("no font… import the Switch fonts").
cp -f third_party/fonts/DejaVu/DejaVuSans.ttf \
      third_party/fonts/DejaVu/DejaVuSans-Bold.ttf \
      third_party/fonts/BIZUDPGothic/BIZUDPGothic-Regular.ttf \
      third_party/fonts/Baloo2/Baloo2-SemiBold.ttf \
      "$B/" 2>/dev/null || true
# keep license notes next to them (optional)
cp -f third_party/fonts/DejaVu/LICENSE "$B/DejaVu-LICENSE.txt" 2>/dev/null || true
cp -f third_party/fonts/BIZUDPGothic/OFL.txt "$B/BIZUDPGothic-OFL.txt" 2>/dev/null || true
cp -f third_party/fonts/Baloo2/OFL.txt "$B/Baloo2-OFL.txt" 2>/dev/null || true
cp -f tools/za/za-default.ini "$B/za-default.ini" 2>/dev/null || true

# Smoke-check the same paths the runtime resolves from the exe directory
ok=1
[ -x "$B/lasgen/lasgen" ] || ok=0
[ -x "$B/zig/zig" ] || ok=0
[ -f "$B/gamelib-src/cpu.h" ] || ok=0
[ -f "$B/DejaVuSans.ttf" ] || ok=0
[ "$ok" = 1 ] || { echo "prepare-gamebuild: layout incomplete under $B"; exit 1; }

echo "prepare-gamebuild: OK"
echo
echo "Game files must look like this (either works):"
echo "  A) Dev mode — from the repo root, with your dump already extracted:"
echo "       extracted/exefs/main"
echo "       extracted/romfs/   (or romfs.bin)"
echo "       ./build/linux/$ARCH/za/lasrecomp"
echo "  B) Normal — put the same extracted/ tree in:"
echo "       ~/.local/share/zarecomp/extracted/"
echo "     then run the binary (any cwd) and click Set up the game / Compile now."
echo
echo "Fonts: substitute fonts (DejaVu / BIZ UDPGothic) are next to lasrecomp — enough to play."
echo "  Optional original Switch look: Settings → Files → Import fonts"
echo "  (nintendo_udsg-r_std_003.bfttf + nintendo_ext_003.bfttf from your firmware)."
echo
echo "Check dump:"
echo "  tools/za/doctor-armada.sh"
