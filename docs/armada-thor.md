# ZA Recomp on AYN Thor (Armada OS)

This fork adds a **native linux-aarch64** path so [ZA Recomp](https://github.com/Hexveil404/PokeZ-ARecomp) can run on **AYN Thor** under **[Armada OS](https://armadaos.dev/)** (Fedora bootc, Mesa **Turnip** / Adreno), without Wine, Proton, or FEX.

Official releases are **Windows / Linux x86-64 only**. Upstream closed an arm64 Linux request as not planned; the Makefile already knew about aarch64 (from LAS Recomp). This fork wires ZA’s build scripts for native aarch64 and adds **Mesa Turnip / Adreno** Vulkan paths for Armada (vendor id `0x5143`): dma-buf guest window, XFB mirror, GPU-copied vertex streams — no Android APIs.

> **You need your own copy of the game.** Nothing here includes game files, keys, dumps, or a prebuilt `lasgame.so`.

## What works (expectations)

| Item | Notes |
|------|--------|
| Target | AYN Thor (Snapdragon 8 Gen 2, Adreno) on Armada OS; other Armada aarch64 handhelds may work |
| Build | Native on-device (or any aarch64 Linux with glibc 2.35+, clang, Vulkan 1.3) |
| GPU | Mesa Turnip; Adreno defaults: quad-control for eyes, **XFB mirror off** (per-piece dma-buf XFB+VB), D24 depth, dynamic-state refresh after pipeline bind, soft-fail missing Vulkan features |
| Pages | 4K (Thor) or 16K; **64K page kernels are not supported** |
| Performance | Handheld settings first (lower internal resolution, 30 fps) |

Do **not** run the x86_64 AppImage under FEX for this title if you can avoid it: the game code is heavy SIMD and will be much slower than a native aarch64 build.

## Build on the Thor (recommended)

Armada’s root filesystem is **immutable** (Fedora bootc). Install build deps with **`rpm-ostree`**, then **reboot once**. Plain `dnf install` / `apt install` will not work on the host.

1. In Desktop Mode, from this tree:

   ```bash
   chmod +x tools/za/install-deps-armada.sh tools/za/build-armada.sh
   tools/za/install-deps-armada.sh
   sudo systemctl reboot
   ```

   That layers clang, SDL3 backend `-devel` headers, Vulkan loader headers, etc. After reboot, `pkg-config --exists x11 wayland-client` should succeed.

   **Do not** layer Fedora’s `mesa-libEGL-devel` / `mesa-libgbm-devel` — they conflict with Armada’s own Mesa (`*.armada` Turnip packages). The build turns SDL’s KMSDRM/EGL off and uses Wayland/X11 + Vulkan instead.

2. Build the runtime **and** the on-device game compiler tools:

   ```bash
   cd ~/PokeZ-Armada   # or wherever you cloned
   git pull
   tools/za/build-armada.sh
   ```

   That produces `build/linux/aarch64/za/lasrecomp` **plus** `lasgen/`, `zig/`, and `gamelib-src/` next to it. Without those, the start screen cannot compile the game.

3. Prepare **your** dump — extracted Switch layout (recommended on Armada; avoids needing .NET `zaimport`):

   ```text
   extracted/
     exefs/main      # required (tens of MB)
     exefs/sdk       # usually required
     romfs/          # or a single romfs.bin
   ```

   Put that tree either:

   - in the **repo root** as `~/PokeZ-Armada/extracted/`, then always start the app **from the repo root**, or
   - in `~/.local/share/zarecomp/extracted/` (works from any cwd)

   A Ryujinx/Yuzu/Sudachi dump of title `0100F43008C44000` with update **1.0.0 / 2.0.0 / 2.0.2** is the usual source. `.xci`/`.nsp` alone are not enough until extracted.

4. Compile the game **once** (10–60+ minutes, several GB RAM; keep the Thor plugged in):

   ```bash
   # from repo root — start screen:
   ./build/linux/aarch64/za/lasrecomp
   # → Set up the game / Compile now

   # or CLI (needs extracted/exefs/main in the repo):
   GAME=1 tools/za/build-armada.sh
   ./build/linux/aarch64/za/lasrecomp --play
   ```

   If setup fails, run `tools/za/doctor-armada.sh` and check `~/.local/share/zarecomp/logs/` (or `extracted/.gamebuild/`).

   Optional AppImage/zip for yourself: `DIST=1 tools/za/build-armada.sh`.

Data, saves, settings and logs: `~/.local/share/zarecomp` (same as desktop Linux).

## Graphics (Turnip guest window)

Turnip on Armada **does not** expose `VK_EXT_external_memory_host`. This fork maps the **top ~1600 MB** of Z-A’s guest window as **128 MB dma-buf pieces** (mmap + per-piece `VkBuffer`s + one sparse buffer for shader storage). Character skinning matches the **Android AHB path**: **XFB mirror off**, XFB writes into the **same per-piece buffer** the draw binds as a vertex buffer.

Thor evidence:

| Config | Result |
|--------|--------|
| Bind 1600 MB sparse dma-buf as VB | Head OK, body = static noise |
| Mirror **on** + piece VB | Head OK, body wrong/missing (7447) — sparse XFB/copy ≠ piece VB fetch |
| Unpaired `DMA_BUF` END\|WRITE / END\|RW before submit | Hid or clobbered body |
| Mirror **off** + XFB through per-piece buffer (same as VB) | Target (Android AHB-like) |

### Fix (needs rebuild)

```bash
git pull
tools/za/build-armada.sh
tools/za/fix-graphics-armada.sh   # forces gpu_xfb_mirror=off in larecomp.ini
./build/linux/aarch64/za/lasrecomp
```

Good log lines:

```
[vk] turnip window: sharing top 1600 MB …
[vk] turnip window: … (per-piece VB buffers)
[vk] transform feedback mirror off (Armada/Turnip: per-piece dma-buf VBs …)
[vk] Turnip: binding per-piece dma-buf buffers as VB/IB …
[vk] Turnip: XFB writes through per-piece dma-buf (same VkBuffer as VB; AHB-like)
```

If you still see `gpu_xfb_mirror=on` / `transform feedback mirror on (F1/larecomp.ini…)`, re-run `fix-graphics-armada.sh` and restart — old ini forced the head-only path.

Creator: real body mesh. Expect `host` (per-piece) vertex traffic and little/no `cached` mirror.

Env knobs:

| Variable | Effect |
|----------|--------|
| `LARECOMP_VK_XFB_MIRROR=1` | Force mirror on (debug; often head-only on Thor) |
| `LARECOMP_TURNIP_GMEM_VB=0` | CPU-copy VBs instead of per-piece binds |
| `LARECOMP_TURNIP_DMABUF_SYNC=1` | Unpaired END\|WRITE before submit (hid body) |
| `LARECOMP_TURNIP_WINDOW=0` | Disable share (solid blocks) |

### Gamescope HDR

Armada Game Mode may force an HDR10 swapchain (`A2R10G10B10` / `HDR10_ST2084`). If colours look crushed after gmem is fixed, try launching outside Gamescope or with HDR disabled in the session.

If it still fails, send `~/.local/share/zarecomp/lasrecomp.log` (`gmem`, `turnip window`, `host memory import`, `#271`, `xfb`).

## Fonts (“No font for the game's text”)

That dialog means the runtime found **neither** Switch fonts nor the bundled substitute (`DejaVuSans.ttf` / `BIZUDPGothic`). Nintendo fonts are **not** required to play.

### Fix without rebuilding (current binary)

From the repo (after `git pull`):

```bash
tools/za/fix-fonts-armada.sh
# or point at your binary:
# tools/za/fix-fonts-armada.sh /path/to/lasrecomp
```

That copies the substitute fonts next to `lasrecomp` and into `~/.local/share/zarecomp/`. Then launch the **same** binary again.

One-shot override (no copy):

```bash
LARECOMP_FONT="$PWD/third_party/fonts/DejaVu/DejaVuSans.ttf" \
  ./build/linux/aarch64/za/lasrecomp
```

Optional: layer the system package (reboot required on rpm-ostree):

```bash
sudo rpm-ostree install --allow-inactive dejavu-sans-fonts
sudo systemctl reboot
```

### After the next rebuild

`prepare-gamebuild.sh` / `build-armada.sh` also place those `.ttf` files next to `lasrecomp`. Prefer that binary under `build/linux/aarch64/za/`.

Optional original Switch look: **Settings → Files → Import fonts**, and pick files from your own firmware dump (`nintendo_udsg-r_std_003.bfttf`, `nintendo_ext_003.bfttf`). They land in `~/.local/share/zarecomp/fonts/`.

## Suggested first-run settings (F1)

- Resolution: **75%** or lower / Dynamic
- Frame rate: **30** until you know the Thor can hold 60
- Upscaler: FSR 1 if needed
- Leave optional AO / high shadows off at first
- If textures pop or lights stretch: confirm log shows Adreno/Turnip; those paths force dynamic state after pipeline binds

Useful env overrides (optional):

```bash
export LARECOMP_SHADER_QUAD_CONTROL=1
# leave XFB mirror on (default on Turnip) — =0 hides the character
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
- Turnip dma-buf guest window (top ~2 GB, set up at gwin pick) when `VK_EXT_external_memory_host` is missing; XFB mirror auto-on on Linux Adreno

## Reporting issues

Include: Armada build id, `uname -r`, `getconf PAGESIZE`, `vulkaninfo --summary` (deviceName / driverName), game version, and `~/.local/share/zarecomp/logs/play.log` (or `lasrecomp.log`).

## Credits

- [Hexveil404/PokeZ-ARecomp](https://github.com/Hexveil404/PokeZ-ARecomp) and [peiikai/LASRecomp](https://github.com/peiikai/LASRecomp)
- Adreno workarounds adapted from [fleckaben’s Android port](https://github.com/fleckaben/PokeZ-ARecomp) / [MrSw4gGuy/PokeZ-Android](https://github.com/MrSw4gGuy/PokeZ-Android)
