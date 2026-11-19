---
name: "eden-custom-armada"
description: "Custom Fritchii + Duo aarch64 AppImage for ArmadaOS on the AYN Thor; current baseline is Drive Fritchii v6.2 Thor AppImage (docs/fritchii-duo-newest)"
---

- Has a custom Eden aarch64 AppImage (for ArmadaOS on the AYN Thor) and wants it kept on the latest Eden nightly with "our patches" re-applied
- Wants the custom Eden build to set CPU/GPU clock speed to max while running, as was done for their nxnative ports
- Has Steam ROM Manager set up for Eden on the Thor and doesn't want to redo it
- Wants one all-in-one Switch + Switch 2 app: Fritchii (Frizti/Fritchii) built for the Thor with "our patches", reading all existing Eden saves and data
- Wants Eden Duo's companion screens (igawa6/eden-duo, .dsmod.zip companion packages) working on ArmadaOS in the custom Fritchii Thor build
- Wants a new Eden Duo companion made for Pokémon Mystery Dungeon: Rescue Team DX (owns version 1.0.2) showing the stats menu and dungeon map on the bottom screen
- Runs ArmadaLive on the Thor's bottom screen; dual-screen companions must show above it in Game Mode
- Decided to merge Eden Duo's companion ability directly into Fritchii (one engine) after the separate Duo engine gave bad fps, long loads and a crash
- **Current Thor baseline (2026-11-19):** Christian’s Google Drive AppImage `Fritchii-Thor-aarch64.AppImage` (file id `1CQcHTcY4DGM_EQIAiaVrkBIboFcKwtkM`) — Fritchii **v6.2** + Duo/Thor patches kept via another AI. SHA-256 `82b1b8bcb7fb32fcfef0f17694cd5466a2bce526f433366b385a0de5f3354677`. Durable copy + README: Project store `docs/fritchii-duo-newest/` (AppImage split into `.part0`/`.part1` because store files max out at 100 MiB; also `extracted-packaging/` and `duo-modules-list.txt`). **Build future Duo work on top of this baseline**, not older Claude/v6.1-only drops.
- Upstream Fritchii **v6.2** (Frizti/Fritchii) is what that Drive AppImage embeds (What’s New / AppRun). Published upstream Linux assets remain x86_64; Thor still needs the custom aarch64 AppImage path.
- Memvault Origin tip around `c8428de` still carries the Duo→Fritchii **v6.1** patch kit (`patches/duo/`, apply onto `e0f673a` / fritchii-v6.1) plus Thor packaging/GHA. That lineage landed bottom companion over ArmadaLive, touch IPC, BGRA colors, and install-to-`~/.local/share/fritchii`. When continuing: **rebase patches onto v6.2+**, using `docs/fritchii-duo-newest/` as the smoke-test binary and packaging reference.
- Duo companions install under `~/.local/share/fritchii/load/` (not `eden`). Packaging: `packaging/thor/` + `.github/workflows/fritchii-thor-aarch64.yml` (GitHub `ubuntu-24.04-arm`).
- Older references: Claude’s earlier Thor Drive AppImage / memvault v6.1 CI artifacts; Lichii `thor-*` releases (no Duo / no clock-max). Prefer the Nov 19 Drive **v6.2** AppImage above as the real current custom build.
