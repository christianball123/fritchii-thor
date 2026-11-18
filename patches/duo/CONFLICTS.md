# Duo → Fritchii v6.1: remaining manual ports

The patches under `hooks/` plus the Duo-only `tree/` apply cleanly on
`fritchii-v6.1` (`e0f673a`). They are **not** enough for a working dual-screen
Thor build. Stock Duo is an Eden Android fork; Fritchii v6.1 has Switch 2 /
Lichii divergence (especially `video_core/gpu.cpp`, `rasterizer_interface.h`,
`renderer_vulkan/*`). Eden→Duo reference diffs live in
`refs/eden-duo-vs-eden/` (generated against Duo’s Eden base
`6828dc59dc`).

## Already covered (applies cleanly)

| Piece | Where |
|---|---|
| Duo-only engine (`mods/`, `dsm:u`, `dsmod/`, shaders, pad gate) | `tree/` |
| CMake includes + `EDEN_DSMOD_BUILD_DEV_TOOLS` + `unordered-dense` + PNG stb | `hooks/0001` |
| `System` DualScreen / ModRuntime / mailbox / input subsystem | `hooks/0002` |
| GPU `DSModAux`, rasterizer readback stubs, `SetAuxWindow` decl/method | `hooks/0003` |
| Thor Qt Install/Remove Duo Companion + `--duo-bottom-helper` | `hooks/0004` + `thor-qt/` / `tree/src/yuzu/duo/` |

## Must still port by hand (blockers for companions on screen)

Use the named file under `refs/eden-duo-vs-eden/` as the Duo-side hunk list.
Line numbers will not match Fritchii; port by symbol.

### Critical for boot + companion discovery

1. **`src/core/loader/nso.{h,cpp}`** — `DualScreenMailboxSlot`,
   `LoadPlanTargetModule`, `PeekLoadPlanMailboxSize`, `RegisterDualScreenMod` /
   mailbox placement after the last module. Fritchii’s NSO loader already
   differs (Switch 2). Highest conflict risk.
2. **`src/core/loader/deconstructed_rom_directory.cpp`** — multi-module mailbox
   slot wiring.
3. **`src/core/loader/nro.cpp`** — homebrew `RegisterDualScreenMod`.
4. **`src/core/hle/service/services.cpp`** — *done* (`dsm` registered). Confirm
   `DSM::LoopProcess` builds against Fritchii’s `RegisterHandlers` form
   (`duo_core.cmake` auto-detects `CreateStaticMap`).

### Critical for second-screen present (Vulkan)

5. **`src/video_core/renderer_vulkan/renderer_vulkan.cpp`** — construct
   `dsmod_aux = std::make_unique<DSModAuxPresenter>(…)` in the ctor; call
   `dsmod_aux->Route` / inline companion quad from `Composite()` (see Duo
   `renderer_vulkan.cpp.diff`). Current patch only adds `SetAuxWindow` and the
   include; **without Composite wiring the bottom screen stays black**.
6. **`vk_swapchain`, `vk_blit_screen`, `vk_present_manager`,
   `present/window_adapt_pass`, `vk_rasterizer`** — DSMod aux present-queue /
   companion quad / texture readback hooks. Reference diffs exist; Fritchii’s
   present path has Streamline/DLSS extras.
7. **`src/video_core/gpu.cpp`** — DSMod readback should go through
   `RequestSyncOperation` like Duo when Fritchii’s GPU thread API matches;
   current stub calls `DSModReadbackTexture` directly.

### Needed for guest hooks / input / quality

8. **`src/core/hle/kernel/kernel.{h,cpp}`** — application-process lock + epoch
   (`RetainApplicationProcess`, …) — Duo divergence #4.
9. **`src/hid_core/resources/npad/npad.cpp`** — `dsmod_pad_gate` integration.
10. **`src/core/file_sys/registered_cache.*`** — lazy external content + Duo
    index (optional on Linux if load paths already work).
11. **`src/frontend_common/mod_manager.cpp`** — dualscreen folder awareness.
12. **Android-only / skip for Thor Linux:** everything under `src/android/`,
    JNI `dsmod_native`, Kotlin `duo.*`.

### Deliberate Duo divergences to re-evaluate on Fritchii

See `tree/docs/DUO_DIVERGENCES.md`. For Thor Linux specifically:

- **stb PNG write** — applied (`STBI_ONLY_PNG`; dropped `STBI_WRITE_NO_STDIO`).
- **unordered_dense + nlohmann** — cpm entry added; confirm Fritchii’s
  `externals/CMakeLists.txt` actually `AddJsonPackage`s `nlohmann` (it was
  already in `cpmfile.json`; unordered-dense was appended).
- **NCE #4530 / dynarmic code_gen / fiber TSAN** — only if those bugs show up
  on aarch64 Thor; not required to start the merge.

## Known non-apply of raw Eden→Duo diffs onto Fritchii

`git apply refs/eden-duo-vs-eden/*.diff` against fritchii-v6.1 **will fail** for
nearly every non-trivial file (`core.cpp`, `gpu.cpp`, `nso.cpp`,
`renderer_vulkan.cpp`, …). That is expected: those diffs are documentation /
porting guides, not Fritchii patches. Only `hooks/000*.patch` are authored
against fritchii-v6.1.
