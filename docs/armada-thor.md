# ZA Recomp on AYN Thor (Armada OS)

This fork adds a **native linux-aarch64** path so [ZA Recomp](https://github.com/Hexveil404/PokeZ-ARecomp) can run on **AYN Thor** under **[Armada OS](https://armadaos.dev/)** (Fedora bootc, Mesa **Turnip** / Adreno), without Wine, Proton, or FEX.

Official releases are **Windows / Linux x86-64 only**. Upstream closed an arm64 Linux request as not planned; the Makefile already knew about aarch64 (from LAS Recomp). This fork wires ZA’s build scripts for native aarch64 and applies **Adreno/Turnip** Vulkan workarounds ported from the unofficial [Android arm64 port](https://github.com/fleckaben/PokeZ-ARecomp/tree/android) (vendor id `0x5143`), without Android-only APIs (no AHardwareBuffer, no guest VA rebase).

> **You need your own copy of the game.** Nothing here includes game files, keys, dumps, or a prebuilt `lasgame.so`.

## What works (expectations)

| Item | Notes |
|------|--------|
| Target | AYN Thor (Snapdragon 8 Gen 2, Adreno) on Armada OS; other Armada aarch64 handhelds may work |
| Build | Native on-device (or any aarch64 Linux with glibc 2.35+, clang, Vulkan 1.3) |
| GPU | Mesa Turnip; Adreno defaults: quad-control for eyes, XFB mirror off, D24 depth, dynamic-state refresh after pipeline bind, soft-fail missing Vulkan features |
| Pages | 4K (Thor) or 16K; **64K page kernels are not supported** |
| Performance | Untested on Thor hardware in this fork. Expect handheld settings (lower internal resolution, 30 fps). Odin 3 Android builds reported ~30 fps in the city |

Do **not** run the x86_64 AppImage under FEX for this title if you can avoid it: the game code is heavy SIMD and will be much slower than a native aarch64 build.

## Build on the Thor (recommended)

1. In Desktop Mode, install build tools (package names vary with Armada/Fedora updates), roughly:

   ```bash
   sudo dnf install git clang lld cmake ninja-build pkgconf-pkg-config python3 \
       libX11-devel libXext-devel libXrandr-devel libXcursor-devel libXi-devel \
       libXScrnSaver-devel libXfixes-devel libXtst-devel libxkbcommon-devel \
       wayland-devel wayland-protocols-devel libdecor-devel mesa-libEGL-devel \
       libdrm-devel mesa-libgbm-devel pulseaudio-libs-devel alsa-lib-devel \
       pipewire-devel dbus-devel systemd-devel ibus-devel vulkan-loader-devel \
       patchelf llvm binutils zip
   ```

   If `pkg-config` checks fail, set `PKGCHECK=0` only after the matching `-devel` packages are installed, or after `build/linux/aarch64/deps` already exists.

2. Clone this tree and build the start screen (no game files needed yet):

   ```bash
   git clone <this-repo-url> PokeZ-Armada
   cd PokeZ-Armada
   chmod +x tools/za/build-armada.sh
   tools/za/build-armada.sh
   ```

3. Prepare **your** dump on the device or copy an already-extracted tree:

   - Extracted `exefs/` + `romfs` as on PC, **or**
   - Base + update (`.xci` / `.nsp` / `.nup`) with your own `prod.keys` for the importer

4. Set up and compile the game **once** (10–60+ minutes, several GB RAM):

   ```bash
   # if you already have extracted/exefs/main:
   GAME=1 tools/za/build-armada.sh
   ./build/linux/aarch64/za/lasrecomp --play
   ```

   Or build a redistributable folder/AppImage for yourself:

   ```bash
   DIST=1 ZAIMPORT=1 tools/za/build-armada.sh
   # -> build/dist/ZA Recomp-<ver>-linux-aarch64.AppImage and .zip
   ```

   Then run the AppImage / `zarecomp` wrapper, choose your game folder, **Set up the game**, then **Play**.

Data, saves, settings and logs: `~/.local/share/zarecomp` (same as desktop Linux).

## Suggested first-run settings (F1)

- Resolution: **75%** or lower / Dynamic
- Frame rate: **30** until you know the Thor can hold 60
- Upscaler: FSR 1 if needed
- Leave optional AO / high shadows off at first
- If textures pop or lights stretch: confirm log shows Adreno/Turnip; those paths force dynamic state after pipeline binds

Useful env overrides (optional):

```bash
export LARECOMP_SHADER_QUAD_CONTROL=1
export LARECOMP_VK_XFB_MIRROR=0
export LARECOMP_VK_D24=1
```

## Thor dual screen / Game Mode

- Prefer **Game Mode** or a single fullscreen window on the top panel first.
- Bottom-screen companions (Duo / Fritchii) are unrelated to this port; ZA Recomp is a normal SDL3 Vulkan app.
- If the AppImage fails with FUSE: `APPIMAGE_EXTRACT_AND_RUN=1 ./ZA\ Recomp-*-linux-aarch64.AppImage` or use the `.zip` folder.

## What changed vs upstream (summary)

- `tools/za/build-linux.sh` / `dist-linux.sh`: native **aarch64** (and existing x86_64)
- `tools/za/build-armada.sh`: Thor/Armada wrapper
- Vulkan (vendor `0x5143`): quad-control default, XFB mirror auto-off, D24 without blit-DST, pipeline-bind state invalidate, drop missing device features

## Reporting issues

Include: Armada build id, `uname -r`, `getconf PAGESIZE`, `vulkaninfo --summary` (deviceName / driverName), game version, and `~/.local/share/zarecomp/logs/play.log` (or `lasrecomp.log`).

## Credits

- [Hexveil404/PokeZ-ARecomp](https://github.com/Hexveil404/PokeZ-ARecomp) and [peiikai/LASRecomp](https://github.com/peiikai/LASRecomp)
- Adreno workarounds adapted from [fleckaben’s Android port](https://github.com/fleckaben/PokeZ-ARecomp) / [MrSw4gGuy/PokeZ-Android](https://github.com/MrSw4gGuy/PokeZ-Android)
