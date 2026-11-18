# Fritchii Thor AppImage (ArmadaOS / AYN Thor)

How Claude’s custom Thor AppImage is put together, and how to rebuild it on Fritchii v6.1+.

## What the Drive AppImage is

- Brand: **Fritchii** (Switch + Switch 2), packaged like Eden’s Anylinux/sharun AppImage (`eden` binary name).
- Arch: **aarch64**.
- Built from a Fritchii tree with Eden Duo merged in (`/home/claude/frizti/fritchii` on Claude’s machine).
- Desktop stamp on the uploaded build: `X-AppImage-Version=v1794350456.cf654052c4` (Eden-style nightly id).
- Extra Thor pieces (this folder):
  - `00-custom-odin.hook` — Turnip GPU pick, qt-config tweaks, **clock-max** launcher
  - `eden-maxclock.sh` / `eden-maxclock-setup.sh` — CPU+GPU max clocks while the emulator runs
  - `20-duo-companion.hook` + `duo-companion.py` — install/list/remove `.dsmod.zip` companions
  - `duo-modules/` — linux-aarch64 companion reader `.so` files (keep next to the AppImage build; not in git)
  - `nx2_switches.txt` — Switch 2 performance defaults

Steam ROM Manager can keep pointing at the same AppImage path; replace the file in place.

## Clock max

On first launch (unless declined), the hook installs `/usr/local/libexec/eden-maxclock` with a one-time sudo/pkexec prompt, then every launch pins CPU governors + Adreno GPU to max until the emulator exits.

- Off: `EDEN_MAX_CLOCKS=0`
- Redo setup: `EDEN_SETUP_CLOCKS=1`
- Undo: `sudo rm /etc/sudoers.d/eden-maxclock /usr/local/libexec/eden-maxclock`

## Duo companions

```bash
./Fritchii-Thor-aarch64.AppImage --install-companion Game.dsmod.zip
./Fritchii-Thor-aarch64.AppImage --list-companions
./Fritchii-Thor-aarch64.AppImage --remove-companion TITLEID
```

Bottom screen off: `EDEN_DUO_SCREEN=off`. Companions live under `~/.local/share/eden/load/`.

## Rebuild plan (Fritchii v6.1)

1. Start from `Frizti/Fritchii` tag `fritchii-v6.1`.
2. Apply the Duo patch kit: `packaging/thor/scripts/apply-patches.sh /path/to/Fritchii` (hooks `0001`–`0008` cover DualScreen runtime, Vulkan Composite/aux present, NSO mailbox, npad gate; leftover texture-cache / kernel ports are in `patches/duo/CONFLICTS.md`).
3. Configure + build on **linux-aarch64**:
   ```bash
   packaging/thor/scripts/configure-thor.sh   # from Fritchii root
   cmake --build build/release --parallel
   ```
4. Package:
   ```bash
   packaging/thor/scripts/package-appimage.sh /path/to/Fritchii /path/to/memvault
   ```
   This uses linuxdeploy (Thor excludes system Vulkan/Turnip), injects
   `00-custom-odin.hook`, clock-max, Duo companion CLI, and `duo-modules/*.so`.
5. Ship as `Fritchii-Thor-aarch64.AppImage` (keep `eden` TryExec / StartupWMClass).

### GitHub Actions

Workflow: `.github/workflows/fritchii-thor-aarch64.yml` (manual dispatch on
`ubuntu-24.04-arm`). Origin does not run GitHub Actions — mirror this repo to
GitHub or copy `patches/duo` + `packaging/thor` into a Frizti/Fritchii fork and
run the workflow there. Artifact: `Fritchii-Thor-aarch64.AppImage`.

Upstream v6.1 only publishes Windows + Linux x86_64; the Thor file must be built
on arm64 CI (not this x86_64 agent). The older Claude AppImage used sharun;
`AppRun.sh` / `AppRun.lib` remain for that layout. New builds use linuxdeploy +
`AppRun` (same hooks).
