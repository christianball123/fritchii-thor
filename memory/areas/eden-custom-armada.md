---
name: "eden-custom-armada"
description: "User's custom Eden (Switch emulator) aarch64 AppImage for ArmadaOS on the AYN Thor, kept updated to new Eden nightlies"
---

- Has a custom Eden aarch64 AppImage (for ArmadaOS on the AYN Thor) and wants it kept on the latest Eden nightly with "our patches" re-applied
- Wants the custom Eden build to set CPU/GPU clock speed to max while running, as was done for their nxnative ports
- Has Steam ROM Manager set up for Eden on the Thor and doesn't want to redo it
- Wants one all-in-one Switch + Switch 2 app: Fritchii (Frizti/Fritchii) built for the Thor with "our patches", reading all existing Eden saves and data
- Wants Eden Duo's companion screens (igawa6/eden-duo, .dsmod.zip companion packages) working on ArmadaOS in the custom Fritchii Thor build
- Wants a new Eden Duo companion made for Pokémon Mystery Dungeon: Rescue Team DX (owns version 1.0.2) showing the stats menu and dungeon map on the bottom screen
- Runs ArmadaLive on the Thor's bottom screen; dual-screen companions must show above it in Game Mode
- Decided to merge Eden Duo's companion ability directly into Fritchii (one engine) after the separate Duo engine gave bad fps, long loads and a crash
- Upstream Fritchii v6.1 (Frizti/Fritchii tag fritchii-v6.1, commit e0f673a) shipped 18 Nov 2026: pot-room VRAM fix, and the Linux build matches the Windows build. Published files are Windows x64 and Linux x86_64 only. The Thor still needs an arm64 AppImage with the Duo companion host and clock-max patches reapplied.
- Claude's Thor AppImage (Drive upload Nov 2026) is a custom Fritchii aarch64 sharun/Anylinux package: binary name `eden`, Duo merged into the engine (`video_core/dsmod`, Install Duo Companion menus), clock-max via `eden-maxclock.sh`, Turnip hooks in `00-custom-odin.hook`. Desktop stamp `v1794350456.cf654052c4`. Packaging kit lives in `packaging/thor/` in memvault. Source tree was `/home/claude/frizti/fritchii` (not published). Rebuild target: rebase Duo + hooks onto fritchii-v6.1.
- Older Lichii Thor CI AppImages also exist at christianball123/Lichii releases `thor-*` (no Duo / no clock-max). Prefer the Drive/Claude Fritchii AppImage as the real custom build.
