---
name: "gaming"
description: "Gaming platforms, emulation frontends, and Steam/Proton setup"
---

- Games on both PC (via Steam/Proton) and Android
- Configured Cocoon (an Android game frontend using the Daijishou schema) to add RPCSX-Clanker as a PS3 emulator player entry
- Set up GE-Proton via ProtonUp-Qt
- Troubleshot Steam/Proton issues on CachyOS stemming from NTFS external storage
- External SSD (`/dev/sdb2`, UUID `02A00410A0040CBB`, label "SSD") and SD card (`/dev/mmcblk0p1`, UUID `A45C20B25C2080E2`, label "SDCARD") are mounted via `ntfs3` driver with `rw,uid=1000,gid=1000,force,exec,nofail` in fstab
- Moved Proton's Wine prefix (`compatdata`) to local storage and symlinked back to avoid NTFS rename operation failures
- Explored using their phone as a virtual second monitor via Weylus, Deskreen (resolved by using AUR package over AppImage), and Sunshine/Artemis — encountering persistent Wayland input-handling conflicts
- Runs WinNative (Winlator-based Windows emulator) on Android, on the Pubg-spoofed build with package `com.tencent.ig` and launcher activity `com.winlator.cmod.app.shell.UnifiedActivity`; set up a custom Cocoon/Daijishō platform JSON to launch its frontend-exported `.desktop` shortcuts
- Has an AYN Thor (Android handheld) and wants native Android ports to run on it
- Uses ArmadaOS (SteamOS-like ARM64 Linux distro, FEX + CachyOS Proton) on their AYN Thor handheld (not an Odin2 — SD card label "Odin2" is just a label); reinstalled it on Oct 25 2026 and is redoing the SD card setup (persistent fstab mount at `/var/media/sdcard`, no automount flag, UUID `7F7F-E567`, exFAT, label Odin2) — uses a Steam library on an exFAT/FAT32 SD card with a Steam library on an exFAT/FAT32 SD card; Proton/pressure-vessel prefix creation needs symlinks which that filesystem can't support, causing games to install but crash-loop with `PermissionError: Operation not permitted` on symlink creation into `compatdata`
- Fixed it on ArmadaOS by bind-mounting the game's `compatdata/<appid>` folder (e.g. `/var/media/sdcard/SteamLibrary/steamapps/compatdata/2513280`) over to a real folder on internal storage (ext4) via `sudo mount --bind`, so Steam's existing SD-card compat data path stays the same but writes land on ext4 — worked after Armada's own `armada-game-launch` wrapper (config at `/etc/armada/game-tweaks.json`, reset by the Armada Control GUI) and manual `STEAM_COMPAT_DATA_PATH` env overrides both failed to actually redirect the path
- ROMs folder on the SD card is at `/run/media/sdcard/Roms`; hit "there was an error detecting your storage" trying to set EmuDeck to use the SD card on ArmadaOS, working around it by pointing the emulator frontend directly at that path instead of using EmuDeck's storage picker
- Runs ArmadaOS on their AYN Thor and plays PC games there via Steam/Proton (installs Proton builds with ProtonPlus)
- GitHub username is christianball123; has repo christianball123/vita3k-plus-arm64 set up to auto-build a native ARM64 Linux Vita3K Plus AppImage for ArmadaOS (self-updating from that repo's releases)
- Has a new Samsung Galaxy Fold 8 (Android phone) and wants old Android games to run on it
- Switch prod.keys live in /run/media/armada/Odin2/Switch Keys 23.0.0 on the Thor's SD card; extra Switch content (DLC NSPs) under /run/media/armada/Odin2/Switch Extra/
