# Duo → Fritchii v6.1: remaining manual ports

The patches under `hooks/` plus the Duo-only `tree/` apply cleanly on
`fritchii-v6.1` (`e0f673a`). Stock Duo is an Eden Android fork; Fritchii v6.1 has
Switch 2 / Lichii divergence (especially `video_core/gpu.cpp`,
`rasterizer_interface.h`, `renderer_vulkan/*`). Eden→Duo reference diffs live in
`refs/eden-duo-vs-eden/` (generated against Duo’s Eden base `6828dc59dc`).

## Already covered (applies cleanly)

| Piece | Where |
|---|---|
| Duo-only engine (`mods/`, `dsm:u`, `dsmod/`, shaders, pad gate) | `tree/` |
| CMake includes + `EDEN_DSMOD_BUILD_DEV_TOOLS` + `unordered-dense` + PNG stb | `hooks/0001` |
| `System` DualScreen / ModRuntime / mailbox / input subsystem | `hooks/0002` |
| GPU `DSModAux`, rasterizer readback stubs, `SetAuxWindow` decl/method | `hooks/0003` |
| Thor Qt Install/Remove Duo Companion + `--duo-bottom-helper` | `hooks/0004` + `thor-qt/` / `tree/src/yuzu/duo/` |
| Vulkan `DSModAuxPresenter` construction + `Composite()` Route / inline companion quad + `TakePendingScreenshot` | `hooks/0005` |
| `PresentManager` force/allow present thread + `TryGetRenderFrame` decl; `Swapchain` `dsmod_aux` ctor arg | `hooks/0006` |
| Loader NSO `DualScreenMailboxSlot` / `RegisterDualScreenMod` / mailbox + `CompanionCodeSegment` + GeneratedCode page-table | `hooks/0007` |
| `npad` `DSModPadGate` focus-mode filter | `hooks/0008` |

## Must still port by hand

Use the named file under `refs/eden-duo-vs-eden/` as the Duo-side hunk list.
Line numbers will not match Fritchii; port by symbol.

### Critical for second-screen texture / GPU composite quality

1. **`vk_rasterizer` + texture cache `DSModForEachImage` / MarkColorTarget** —
   `tree/` already has `vk_dsmod_texture_readback.cpp` / `vk_dsmod_rt_probe.cpp`,
   but Fritchii’s texture cache lacks Duo’s `DSModForEachImage` helpers and the
   rasterizer still needs override decls + `TickFrame` / draw-probe call sites.
   Highest remaining video_core conflict with Fritchii’s OneFollower / MemStage
   texture path.
2. **`src/video_core/gpu.cpp`** — DSMod readback should go through
   `RequestSyncOperation` like Duo when Fritchii’s GPU thread API matches;
   current stub (0003) calls `DSModReadbackTexture` directly.

### Needed for guest hooks / input / quality

3. **`src/core/hle/kernel/kernel.{h,cpp}`** — application-process lock + epoch
   (`RetainApplicationProcess`, …) — Duo divergence #4.
4. **`src/core/file_sys/registered_cache.*`** — lazy external content + Duo
   index (optional on Linux if load paths already work).
5. **`src/frontend_common/mod_manager.cpp`** — dualscreen folder awareness.
6. **Android-only / skip for Thor Linux:** everything under `src/android/`,
   JNI `dsmod_native`, Kotlin `duo.*`. Android `VK_GOOGLE_display_timing`
   stamp in `vk_swapchain::Present` was intentionally not ported (Thor Linux
   ignores the `dsmod_aux` ctor flag).

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
