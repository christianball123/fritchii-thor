#!/usr/bin/env bash
# Package a built Fritchii (eden + eden-cli) into Fritchii-Thor-aarch64.AppImage.
# Usage: package-appimage.sh /path/to/Fritchii [/path/to/memvault]
set -euo pipefail

FRITCHII="$(cd "${1:?usage: $0 /path/to/Fritchii [memvault]}" && pwd)"
MEMVAULT="$(cd "${2:-$(dirname "$0")/../../..}" && pwd)"
THOR="$MEMVAULT/packaging/thor"
BUILD_DIR="${BUILD_DIR:-$FRITCHII/build/release}"
OUT_NAME="${OUT_NAME:-Fritchii-Thor-aarch64.AppImage}"
ARCH="$(uname -m)"
OUT_DIR="${OUT_DIR:-$FRITCHII}"

# Fritchii v6.1 Linux GUI is bin/lichii (CMake OUTPUT_NAME). Older trees used eden/yuzu.
GUI_BIN=""
for cand in lichii eden yuzu fritchii; do
  if [ -x "$BUILD_DIR/bin/$cand" ]; then GUI_BIN=$cand; break; fi
done
test -n "$GUI_BIN" && test -x "$BUILD_DIR/bin/$GUI_BIN"

# fritchii-v6.1 has no yuzu_cmd / eden-cli target; CLI is optional (--cli in AppRun).
CLI_BIN=""
for cand in eden-cli yuzu-cmd; do
  if [ -x "$BUILD_DIR/bin/$cand" ]; then CLI_BIN=$cand; break; fi
done
strip "$BUILD_DIR/bin/$GUI_BIN" ${CLI_BIN:+"$BUILD_DIR/bin/$CLI_BIN"} || true

tools="${RUNNER_TEMP:-/tmp}/linuxdeploy-thor"
mkdir -p "$tools"
need() {
  local name="$1" url="$2"
  if [ ! -x "$tools/$name" ]; then
    curl -fsSL -o "$tools/$name" "$url"
    chmod +x "$tools/$name"
  fi
}
need "linuxdeploy-$ARCH.AppImage" \
  "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage"
need "linuxdeploy-plugin-qt-$ARCH.AppImage" \
  "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$ARCH.AppImage"
need "appimagetool-$ARCH.AppImage" \
  "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-$ARCH.AppImage"

WORKDIR="${WORKDIR:-$FRITCHII/AppDir-thor}"
rm -rf "$WORKDIR"
mkdir -p "$WORKDIR/stage-bin" "$WORKDIR/AppDir"

cp -a "$BUILD_DIR/bin/$GUI_BIN" "$WORKDIR/stage-bin/eden"
chmod +x "$WORKDIR/stage-bin/eden"
if [ -n "$CLI_BIN" ]; then
  cp -a "$BUILD_DIR/bin/$CLI_BIN" "$WORKDIR/stage-bin/eden-cli"
  chmod +x "$WORKDIR/stage-bin/eden-cli"
fi

DESKTOP="$THOR/dev.eden_emu.eden.desktop"
ICON=""
for cand in "$THOR/dev.eden_emu.eden.png" "$THOR/dev.eden_emu.eden.svg" \
            "$FRITCHII/dist/dev.eden_emu.eden.png" "$FRITCHII/dist/eden.png"; do
  if [ -f "$cand" ]; then ICON="$cand"; break; fi
done
test -n "$ICON"

APPRUN="$THOR/AppRun"
chmod +x "$APPRUN"

export APPIMAGE_EXTRACT_AND_RUN=1
if [ -n "${QT_ROOT_DIR:-}" ]; then
  export QMAKE="$QT_ROOT_DIR/bin/qmake"
fi
export EXTRA_QT_MODULES=waylandcompositor
export EXTRA_PLATFORM_PLUGINS='libqwayland-egl.so;libqwayland-generic.so'
export ARCH

extra=(--custom-apprun "$APPRUN")
for lib in 'libvulkan.so*' 'libudev.so*' 'libsystemd.so*' 'libdbus-1.so*' \
           'libwayland-cursor.so*' 'libwayland-egl.so*'; do
  extra+=(--exclude-library "$lib")
done

# Populate AppDir without wrapping; we inject Thor hooks next.
deploy=(
  "$tools/linuxdeploy-$ARCH.AppImage" --appdir "$WORKDIR/AppDir"
  --executable "$WORKDIR/stage-bin/eden"
)
if [ -x "$WORKDIR/stage-bin/eden-cli" ]; then
  deploy+=(--executable "$WORKDIR/stage-bin/eden-cli")
fi
deploy+=(
  --desktop-file "$DESKTOP"
  --icon-file "$ICON"
  --plugin qt
  "${extra[@]}"
)
"${deploy[@]}"

APPDIR="$WORKDIR/AppDir"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/duo-modules" "$APPDIR/bin" "$APPDIR/share"
cp -a "$THOR/00-custom-odin.hook" "$THOR/20-duo-companion.hook" "$APPDIR/usr/bin/"
cp -a "$THOR/eden-maxclock.sh" "$THOR/eden-maxclock-setup.sh" "$APPDIR/usr/bin/"
cp -a "$THOR/duo-companion.py" "$THOR/nx2_switches.txt" "$APPDIR/usr/bin/"
cp -a "$THOR/AppRun.lib" "$APPDIR/usr/bin/"
DUO_SRC="${DUO_MODULES_DIR:-$THOR/duo-modules}"
if [ -d "$DUO_SRC" ] && compgen -G "$DUO_SRC/"'*.so' >/dev/null; then
  cp -a "$DUO_SRC/"*.so "$APPDIR/usr/share/duo-modules/"
elif [ -d /tmp/fritchii-thor/duo-modules-cache ] && \
     compgen -G /tmp/fritchii-thor/duo-modules-cache/'*.so' >/dev/null; then
  cp -a /tmp/fritchii-thor/duo-modules-cache/*.so "$APPDIR/usr/share/duo-modules/"
else
  echo "warning: no duo-modules *.so found; companions that need readers will fail" >&2
fi
chmod +x "$APPDIR/usr/bin/"*.hook "$APPDIR/usr/bin/"eden-maxclock*.sh \
  "$APPDIR/usr/bin/duo-companion.py" "$APPDIR/usr/bin/AppRun.lib"
# Compatibility links: menus/hooks historically used $APPDIR/bin and $APPDIR/share.
ln -sfn ../usr/bin/duo-companion.py "$APPDIR/bin/duo-companion.py"
ln -sfn ../usr/share/duo-modules "$APPDIR/share/duo-modules"

# Keep TryExec/StartupWMClass as eden for Steam ROM Manager.
if [ -f "$APPDIR/usr/share/applications/"*.desktop ]; then
  sed -i 's/^Exec=.*/Exec=eden %f/; s/^TryExec=.*/TryExec=eden/; s/^StartupWMClass=.*/StartupWMClass=eden/' \
    "$APPDIR/usr/share/applications/"*.desktop || true
fi

export VERSION="${LDAI_VERSION:-thor-${GITHUB_SHA:-local}}"
APPIMAGE_EXTRACT_AND_RUN=1 "$tools/appimagetool-$ARCH.AppImage" "$APPDIR" "$OUT_DIR/$OUT_NAME"
test -s "$OUT_DIR/$OUT_NAME"
ls -la "$OUT_DIR/$OUT_NAME"
echo "Built $OUT_DIR/$OUT_NAME"
