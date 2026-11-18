---
name: "linux-setup"
description: "CachyOS laptop (Acer Predator) setup, shell, kernel modules; ArmadaOS (Fedora atomic) on the AYN Thor"
---

- Runs CachyOS (Arch-based Linux) as daily driver on an Acer Predator PH16-71 laptop
- Uses Fish shell
- Resolved a broken linuwu_sense DKMS kernel module on CachyOS — key fix: always build with `make LLVM=1` on CachyOS
- The linuwu_sense module lives at `~/Linuwu-Sense` and must be rebuilt manually after kernel updates
- Also has linuwu-sense registered under DKMS (two versions, 1.0 and 1.0.0) which conflicts with the manual build — stale DKMS-built .ko.zst in updates/dkms/ takes priority over the manual build in kernel/drivers/platform/x86/, causing vermagic mismatch (Exec format error); fixed by `sudo dkms remove linuwu-sense/<version> -k <kernel>` for both versions, then `sudo depmod -a` and `sudo modprobe linuwu_sense`
- Suspects an AUR package may be auto-registering linuwu-sense with DKMS on updates, causing this conflict to recur
- The acer_wmi_battery module is blacklisted at `/etc/modprobe.d/acer-wmi-battery-blacklist.conf`
- Troubleshot a broken SDDM installation
- Resolved Fish shell / PATH issues related to SteaMidra and AppImage/FUSE dependencies
- Recently encountered a read-only filesystem issue on CachyOS with NTFS external storage
- Kernel updated to 7.2.0-1-cachyos, which broke linuwu_sense again — this time via two new issues: (1) kernel now built with clang, so building the module requires `CC=clang` in addition to `LLVM=1`; (2) kernel headers dropped `strncpy` entirely, requiring patching the 3 call sites in linuwu_sense.c (swapped to `memcpy` since the code already manually null-terminates after each copy) — fix not upstream yet, will need reapplying after a fresh clone/pull
- Laptop has hybrid graphics: Intel Arrow Lake-S iGPU (i915/xe) + NVIDIA RTX 5070 Ti Mobile (GB205M) dGPU, using proprietary nvidia driver (KMD 610.57.04); KDE Plasma/kwin_wayland runs directly on the NVIDIA GPU
- Hit a recurring issue: external HDMI monitor shows as connected with correct active mode in xrandr but displays pure black after reboot
- Also hit a related issue where after unplugging/replugging the HDMI monitor, the main laptop screen (touchpad) stops registering clicks (cursor still moves, libinput sees the button press) — this is a KWin pointer-routing bug triggered by the hotplug event, not a driver/hardware issue; fixed live (no reboot needed) by running `kwin_wayland --replace &` in a terminal to restart the compositor
- Wants to install the Archer Acer laptop management suite (otectus/Archer) for fan control, RGB, and power profiles
- Owns the AYN Thor Pro model
- AYN Thor runs ArmadaOS: a Fedora 44-based atomic image (rpm-ostree, read-only system, no pacman); has dnf, flatpak, toolbox and distrobox — build tools go in a toolbox container
- The AYN Thor also has an Android side (stock Android 13) alongside ArmadaOS; uses it for Android builds like the smm2recomp APK
- On the AYN Thor's ArmadaOS, needs Steam running to use the built-in controller; has been using the nested desktop inside Game Mode
