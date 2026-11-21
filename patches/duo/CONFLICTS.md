# Duo → Fritchii v6.5: remaining manual ports

The patches under `hooks/` plus the Duo-only `tree/` apply cleanly on
`fritchii-v6.5` (`61fa4fdda`). Stock Duo is an Eden Android fork; Fritchii v6.5 has
Switch 2 / Lichii divergence (especially `video_core/gpu.cpp`,
`rasterizer_interface.h`, `renderer_vulkan/*`). Eden→Duo reference diffs live in
`refs/eden-duo-vs-eden/` (generated against Duo’s Eden base `6828dc59dc`).

## Already covered (applies cleanly)

| Piece | Where |
|---|---|
| Duo-only engine (`mods/`, `dsm:u`, `dsmod/`, shaders, pad gate) | `tree/` |
| CMake includes + `EDEN_DSMOD_BUILD_DEV_TOOLS` + `unordered-dense` + PNG stb | `hooks/0001` |
| `System` DualScreen / ModRuntime / mailbox / input subsystem | `hooks/0002` |
| GPU `DSModAux`, rasterizer readback interface, `SetAuxWindow` decl/method | `hooks/0003` |
| Thor Qt Install/Remove Duo Companion + `--duo-bottom-helper` | `hooks/0004` + `thor-qt/` / `tree/src/yuzu/duo/` |
| Vulkan `DSModAuxPresenter` construction + `Composite()` Route / inline companion quad + `TakePendingScreenshot` | `hooks/0005` |
| `PresentManager` force/allow present thread + `TryGetRenderFrame` decl; `Swapchain` `dsmod_aux` ctor arg | `hooks/0006` |
| Loader NSO `DualScreenMailboxSlot` / `RegisterDualScreenMod` / mailbox + `CompanionCodeSegment` + GeneratedCode page-table | `hooks/0007` |
| `npad` `DSModPadGate` focus-mode filter | `hooks/0008` |
| Texture cache `ColorTarget` flags + `DSModForEachImage` / `GetImage` / `GetColorBufferId` / `DSModMarkColorTarget` (+ `UpdateRenderTargets` / `JoinImages` sites) | `hooks/0009` |
| `vk_rasterizer` DSMod readback overrides + pending-readback state; `TickFrame` / `PrepareDraw` probe call sites | `hooks/0010` |
| GPU `DSModRequestTextureReadback` via `RequestSyncOperation` + `TickGPU` | `hooks/0011` |
| Kernel `RetainApplicationProcess` / epoch + `application_process_lock` (+ pad-gate on process swap) | `hooks/0012` |
| `registered_cache` mutex + lazy `EnsureScanned` + Duo external content index | `hooks/0013` |
| `mod_manager` `dualscreen` folder awareness | `hooks/0014` |
| `common/thread.cpp` `nx2_debug_control.h` for aarch64 EXP103 | `hooks/0017` |
| `PerfStats::GetTotals` definition inside `namespace Core` | `hooks/0015` |
| `AppLoader::GetFile` + `OpenProgramNcaFromGameFile` (Duo private RomFS) | `hooks/0016` |
| `fritchii_write_ring.h` `strncpy` bound `size-1` (stringop-truncation) | `hooks/0018` |
| Duo Companion actions inside `main.ui` widget | `hooks/0019` |
| DuoQt includes in `main.cpp` / `main_window.cpp` | `hooks/0020` |
| `video_core` `-frtti` (Fritchii `-fno-rtti` vs `dynamic_cast`) | `tree/…/duo_video_core.cmake` |
| Aux `RecreateFrame` without Duo `storage` flag | `tree/…/vk_dsmod_aux.cpp` |
| Qt6 `QLatin1Char('/')` (not `QChar('/')`) in companion menus | `tree/…/duo_companion_menus.cpp` |
| Thor aarch64: `-DENABLE_WERROR=OFF` + selected `-Wno-error=*` | `packaging/thor/scripts/configure-thor.sh` |

## Must still port by hand / out of Thor Linux scope

Use the named file under `refs/eden-duo-vs-eden/` as the Duo-side hunk list.
Line numbers will not match Fritchii; port by symbol.

### Android-only / skip for Thor Linux

1. Everything under `src/android/`, JNI `dsmod_native`, Kotlin `duo.*`.
2. Android `VK_GOOGLE_display_timing` stamp in `vk_swapchain::Present` was
   intentionally not ported (Thor Linux ignores the `dsmod_aux` ctor flag).

### Deliberate Duo divergences to re-evaluate on Fritchii

See `tree/docs/DUO_DIVERGENCES.md`. For Thor Linux specifically:

- **stb PNG write** — applied (`STBI_ONLY_PNG`; dropped `STBI_WRITE_NO_STDIO`).
- **unordered_dense + nlohmann** — cpm entry added; confirm Fritchii’s
  `externals/CMakeLists.txt` actually `AddJsonPackage`s `nlohmann` (it was
  already in `cpmfile.json`; unordered-dense was appended).
- **NCE #4530 / dynarmic code_gen / fiber TSAN** — only if those bugs show up
  on aarch64 Thor; not required to start the merge.

### Runtime / packaging still required for a working bottom screen

Hooks + `tree/` alone do not ship a Thor companion bottom screen. Still needed:

1. **linux-aarch64 build** of Fritchii+Duo (this x86_64 environment does not
   produce the AppImage).
2. **Packaging** with `packaging/thor/` hooks + `share/duo-modules/*.so`
   companions installed for the title under test.
3. **Smoke** Install Duo Companion / `--duo-bottom-helper` against a title that
   has a dualscreen module; confirm aux present path and texture readback at
   runtime (flags in `0009` matter for RT-style selectors).

## Known non-apply of raw Eden→Duo diffs onto Fritchii

`git apply refs/eden-duo-vs-eden/*.diff` against fritchii-v6.5 **will fail** for
nearly every non-trivial file (`core.cpp`, `gpu.cpp`, `nso.cpp`,
`renderer_vulkan.cpp`, …). That is expected: those diffs are documentation /
porting guides, not Fritchii patches. Only `hooks/0*.patch` are authored
against fritchii-v6.5.

- Hook 0021: guard nvhost_gpu.cpp immintrin/_mm_prefetch for aarch64 (stock Fritchii v6.5).

- Hook 0022: known_issues IntelCpu guards cpu_string for aarch64 stub CPUCaps.

- apply.sh: tree/ wins over thor-qt (QLatin1Char must survive).
- Packaging: Fritchii Linux GUI is lichii; eden-cli optional.
- CI: ccache restore/save; ninja -k 0; QChar guard after apply.

- Hook 0023: provide GetProcessorCount + stub g_cpu_caps on aarch64 (link lichii).

- Hook 0024: Fritchii RestructureMenus keeps Install/Remove Duo Companion.
- Hook 0025 + duo_aux_window: Thor bottom SetAuxWindow via SDL on /armada-bottom-env DISPLAY.
- Packaging: usr/bin + APPDIR/bin share compatibility links for duo-companion.

- Real --duo-bottom-helper: SDL present + X11 raise/claim over ArmadaLive; shared /eden-duo-bottom-* buffer via duo_bottom_bridge (Claude AppImage path).

- Bottom claim fix: STEAM_GAME=852199 + GAMESCOPECTRL_BASELAYER_APPID (gamescope), shared DOUB header @0x7c pixels (Claude binary).

## Obsolete on fritchii-v6.5+

- Hook **0019** (move Duo actions inside `</widget>`) is folded into **0004**.
- Hook **0022** (aarch64 `IntelCpu`/`cpu_string` in `known_issues.cpp`) is gone: v6.5 uses `Common::IntelHybridCpu()` and no longer reads `cpu_string` there.
