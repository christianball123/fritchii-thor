#!/bin/bash
# Diagnose why "Set up the game" / GAME=1 fails on Thor / Armada.
set -euo pipefail
cd "$(dirname "$0")/../.."
HOST=$(uname -m); HOST=${HOST/arm64/aarch64}
ARCH=${ARCH:-$HOST}
B=$PWD/build/linux/$ARCH/za
DATA=${LARECOMP_DATA_ROOT:-$HOME/.local/share/zarecomp}

echo "=== ZA Recomp Armada doctor ==="
echo "host=$(uname -m)  pagesize=$(getconf PAGESIZE 2>/dev/null || echo ?)"
echo "cwd=$PWD"
echo

check() { # label path [optional test]
    local label=$1 path=$2
    if [ -e "$path" ]; then echo "  OK  $label: $path"
    else echo "  MISSING  $label: $path"; return 1; fi
}

echo "-- binary + gamebuild tools --"
fail=0
check "lasrecomp" "$B/lasrecomp" || fail=1
check "lasgen wrapper" "$B/lasgen/lasgen" || fail=1
check "zig" "$B/zig/zig" || fail=1
check "gamelib-src/cpu.h" "$B/gamelib-src/cpu.h" || fail=1
check "tools/venv" "tools/venv/bin/python" || fail=1
check "tools/gen.py" "tools/gen.py" || fail=1
echo

echo "-- game dump (need ONE of these trees with exefs/main) --"
found=0
for root in "$PWD/extracted" "$DATA/extracted"; do
    if [ -f "$root/exefs/main" ]; then
        echo "  OK  exefs/main: $root/exefs/main ($(du -h "$root/exefs/main" | cut -f1))"
        if [ -d "$root/romfs" ]; then echo "  OK  romfs/: $root/romfs"
        elif [ -f "$root/romfs.bin" ]; then echo "  OK  romfs.bin: $root/romfs.bin"
        else echo "  MISSING  romfs/ or romfs.bin under $root"; fail=1; fi
        # ZA main NSO is large; a tiny file is almost certainly wrong
        sz=$(stat -c%s "$root/exefs/main" 2>/dev/null || stat -f%z "$root/exefs/main")
        if [ "${sz:-0}" -lt 10000000 ]; then
            echo "  WARN  exefs/main is only $sz bytes (expect tens of MB for Z-A) — wrong/incomplete dump?"
        fi
        # sdk NSO often required for gen
        if [ -f "$root/exefs/sdk" ]; then echo "  OK  exefs/sdk: $root/exefs/sdk"
        else echo "  WARN  no exefs/sdk (generator usually needs it)"; fi
        found=1
    else
        echo "  — no exefs/main in $root"
    fi
done
if [ "$found" = 0 ]; then
    echo "  MISSING  no usable dump. Put exefs/ + romfs (or romfs.bin) in:"
    echo "           $PWD/extracted/   OR   $DATA/extracted/"
    fail=1
fi
echo

echo "-- recent logs --"
for log in "$DATA/logs/play.log" "$DATA/lasrecomp.log" "$DATA/extracted/.gamebuild/"*.log \
           "$PWD/extracted/.gamebuild/"*.log "$PWD/logs/play.log"; do
    for f in $log; do
        [ -f "$f" ] || continue
        echo "  --- $f (last 30 lines) ---"
        tail -n 30 "$f" || true
        echo
    done
done

if [ "$fail" = 1 ]; then
    echo "Doctor found problems."
    echo "  1) tools/za/build-armada.sh          # runtime"
    echo "  2) tools/za/prepare-gamebuild.sh     # lasgen + zig next to the binary"
    echo "  3) Place dump at extracted/exefs/main (+ romfs)"
    echo "  4) ./build/linux/$ARCH/za/lasrecomp  # from repo root, then Set up the game"
    echo "  Or: GAME=1 tools/za/build-armada.sh  # compile from extracted/ on the CLI"
    exit 1
fi
echo "Doctor: layout looks ready. If Compile still fails, paste the log section above."
