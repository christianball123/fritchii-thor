# Eden Duo → Fritchii v6.1 (Thor) patch kit

Merges **igawa6/eden-duo**’s dual-screen companion engine into
**Frizti/Fritchii** tag **`fritchii-v6.1`** (`e0f673a`), plus Claude’s Thor-only
Linux Qt pieces recovered from the working AppImage
(`/tmp/fritchii-thor/squashfs-root`, binary built from
`/home/claude/frizti/fritchii`).

Android JNI / Kotlin from Duo is **not** included. Packaging hooks stay in
`packaging/thor/` (not this folder).

## Layout

| Path | Role |
|---|---|
| `tree/` | Duo-only sources to copy in: `src/core/mods/`, `hle/service/dsm/`, `file_sys/duo_external_content_index.*`, `video_core/dsmod/`, `vk_dsmod_*`, companion shaders, `common/dsmod_dev_tools.h`, `hid_core/.../dsmod_pad_gate.h`, Thor `src/yuzu/duo/`, docs |
| `hooks/0001-*.patch` … `0019-*.patch` | Fritchii-v6.1-targeted hooks — **`git apply --check` clean** |
| `thor-qt/` | Same Thor Qt sources + `main.ui` patch (also mirrored under `tree/src/yuzu/duo/`) |
| `refs/eden-duo-vs-eden/` | Eden base `6828dc59` → Duo diffs for files still needing a **manual** Fritchii port |
| `refs/upstream-hooks-linux.tsv` | Duo’s ledger of non-Android upstream touches |
| `CONFLICTS.md` | What still blocks full companion quality / packaging |
| `scripts/apply.sh` | Copy `tree/` + apply `hooks/0*.patch` in order |

## Apply on fritchii-v6.1

```bash
git clone https://github.com/Frizti/Fritchii.git
cd Fritchii
git checkout fritchii-v6.1

# from the memvault repo:
./patches/duo/scripts/apply.sh /path/to/Fritchii
```

Or manually:

```bash
cp -a patches/duo/tree/. /path/to/Fritchii/
cd /path/to/Fritchii
git apply /path/to/memvault/patches/duo/hooks/0*.patch
```

Verify:

```bash
test -f src/core/mods/duo_core.cmake
grep -n OnMenuInstallDuoCompanion src/yuzu/main_window.h
grep -n duo_core.cmake src/core/CMakeLists.txt
grep -n 'make_unique<DSModAuxPresenter>' src/video_core/renderer_vulkan/renderer_vulkan.cpp
grep -n DualScreenMailboxSlot src/core/loader/nso.h
grep -n DSModForEachImage src/video_core/texture_cache/texture_cache_base.h
grep -n RetainApplicationProcess src/core/hle/kernel/kernel.h
grep -n dualscreen src/frontend_common/mod_manager.cpp
```

## What stock Duo vs Claude’s Thor build contribute

**From eden-duo (engine):** `ModRuntime`, DSMod modules, loader mailbox,
`dsm:u`, Vulkan `DSModAuxPresenter` / companion quad / texture readback, desktop
`yuzu_cmd` harness (reference only; Fritchii has no `yuzu_cmd` target in-tree).

**Not in stock Duo (recreated under `thor-qt/` / `tree/src/yuzu/duo/`):**

- `OnMenuInstallDuoCompanion` / `OnMenuRemoveDuoCompanion` / dump files
- `EDEN_DUO_SCREEN=off`, `--duo-bottom-helper`, `/armada-bottom-env`
- Strings: `Thor bottom: companion screen off`, `duo bottom helper: …`

CLI install remains `packaging/thor/duo-companion.py` via
`20-duo-companion.hook`.

## Hooks landed for companions on screen

| Patch | Role |
|---|---|
| `0005` | Construct `dsmod_aux`, `Composite()` Route / FitInline / companion quad through blit + WindowAdaptPass, `TakePendingScreenshot` |
| `0006` | Aux present-thread + `TryGetRenderFrame` declaration; Swapchain `dsmod_aux` ctor (Linux no-op) |
| `0007` | NSO mailbox / load plan / `RegisterDualScreenMod` + CompanionCodeSegment + GeneratedCode |
| `0008` | `DSModPadGate` in `npad` |
| `0009` | Texture-cache `ColorTarget` / `DSModForEachImage` / Mark + inherit (feeds `vk_dsmod_texture_readback.cpp`) |
| `0010` | Rasterizer DSMod readback decls + `TickFrame` / `PrepareDraw` probe sites |
| `0011` | GPU readback scheduled with `RequestSyncOperation` |
| `0012` | Kernel application-process lock + epoch (`RetainApplicationProcess`) |
| `0013` | `registered_cache` lazy scan + Duo external index |
| `0014` | `mod_manager` recognizes `dualscreen/` mod folders |
| `0015` | Move `PerfStats::GetTotals` inside `namespace Core` (0002 placed it after the close) |
| `0016` | `AppLoader::GetFile` + `FileSys::OpenProgramNcaFromGameFile` for Duo private RomFS |
| `0017` | `common/thread.cpp` includes `nx2_debug_control.h` on aarch64 (EXP103) |
| `0018` | `fritchii_write_ring.h` `strncpy` uses `size-1` (GCC `-Werror=stringop-truncation`) |
| `0019` | Move Duo Companion `<action>` defs inside main widget (0004 left them after `</widget>`) |

Also in `tree/`:
- `duo_video_core.cmake`: `-frtti` for `video_core` (Fritchii `-fno-rtti` +
  `dynamic_cast` in e_split/gpu_split/memstage_shadow)
- `vk_dsmod_aux.cpp`: call Fritchii’s 5-arg `RecreateFrame` (Duo’s `storage`
  flag not ported)

## After apply: rebuild Thor AppImage

1. Build **linux-aarch64** (e.g. GitHub Actions `ubuntu-24.04-arm`). This x86_64
   environment does not produce the final AppImage.
2. Package with sharun/Anylinux; install hooks from `packaging/thor/`:
   `00-custom-odin.hook`, `eden-maxclock*.sh`, `20-duo-companion.hook`,
   `duo-companion.py`, `nx2_switches.txt`, and `share/duo-modules/*.so`
   (cached at `/tmp/fritchii-thor/duo-modules-cache` / `packaging/thor/duo-modules/`).
3. Ship as `Fritchii-Thor-aarch64.AppImage` (keep `eden` StartupWMClass for Steam).

Details: `packaging/thor/BUILD.md`.

## Sources used

- Duo: `https://github.com/igawa6/eden-duo` (sparse under `/tmp/src/eden-duo`)
- Duo Eden base: `6828dc59dc` from `git.eden-emu.dev/eden-emu/eden`
- Fritchii: `https://github.com/Frizti/Fritchii` tag `fritchii-v6.1`
- Working AppImage strings: `/tmp/fritchii-thor/squashfs-root/shared/bin/eden`
