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
2. Re-merge Eden Duo (`igawa6/eden-duo`): `src/core/mods`, loader dual-screen hooks, `video_core` DSMod / aux Vulkan surface, Qt “Install Duo Companion” menus, Linux bottom-screen helper.
3. Build **linux-aarch64** (GitHub Actions `ubuntu-24.04-arm`, same flags as the old Lichii `thor-build` PGO job when a profile exists).
4. Package with sharun/Anylinux (or linuxdeploy), then drop in the hooks from this folder and `share/duo-modules/`.
5. Ship as `Fritchii-Thor-aarch64.AppImage` (keep `eden` TryExec / StartupWMClass if Steam entries already use them).

Upstream v6.1 only publishes Windows + Linux x86_64; the Thor file must be built here.
