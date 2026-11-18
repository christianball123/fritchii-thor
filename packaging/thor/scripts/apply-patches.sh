#!/usr/bin/env bash
# Apply memvault Duo + Thor patch kit onto a Fritchii checkout at fritchii-v6.1.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
FRITCHII="${1:-}"

if [ -z "$FRITCHII" ] || [ ! -d "$FRITCHII" ]; then
  echo "usage: $0 /path/to/Fritchii" >&2
  exit 2
fi

exec "$ROOT/patches/duo/scripts/apply.sh" "$FRITCHII"
