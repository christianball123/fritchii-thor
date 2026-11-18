#!/usr/bin/env bash
# Apply Eden Duo (Linux) + Thor Qt onto a Frizti/Fritchii checkout at fritchii-v6.1.
#
# Usage:
#   ./patches/duo/scripts/apply.sh /path/to/Fritchii
#
# Expects the Fritchii tree to be checked out at tag fritchii-v6.1 (e0f673a).

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TARGET="${1:-}"

if [[ -z "$TARGET" || ! -d "$TARGET" ]]; then
  echo "usage: $0 /path/to/Fritchii" >&2
  exit 2
fi

TARGET="$(cd "$TARGET" && pwd)"
cd "$TARGET"

if [[ ! -f src/core/core.h || ! -f src/yuzu/main_window.cpp ]]; then
  echo "error: $TARGET does not look like a Fritchii source tree" >&2
  exit 1
fi

echo "==> Copying Duo-only tree (mods, dsmod, dsm, Thor Qt, …)"
cp -a "$ROOT/tree/." .

echo "==> Applying Fritchii-v6.1 hook patches"
for p in "$ROOT/hooks"/000*.patch; do
  echo "    $(basename "$p")"
  git apply --whitespace=nowarn "$p"
done

if [[ -d "$ROOT/thor-qt/src/yuzu/duo" ]]; then
  mkdir -p src/yuzu/duo
  cp -a "$ROOT/thor-qt/src/yuzu/duo/." src/yuzu/duo/
fi

echo "==> Done."
echo
echo "Still useful before an aarch64 Thor AppImage ships companions end-to-end:"
echo "  - Remaining ports in $ROOT/CONFLICTS.md (texture-cache DSMod readback,"
echo "    kernel application-process epoch, registered_cache, mod_manager, …)."
echo "  - Build linux-aarch64 (GitHub Actions ubuntu-24.04-arm)."
echo "  - Package with packaging/thor/ hooks + share/duo-modules/."
