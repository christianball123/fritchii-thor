---
name: "lasrecomp-thor"
description: "Getting LASRecomp (Link's Awakening Switch recomp, peiikai/LASRecomp) running on ArmadaOS on the AYN Thor"
---

- Wants peiikai/LASRecomp (static recomp of Link's Awakening, Switch) working on their ArmadaOS install on the [[ayn-thor]]; runs the official aarch64 AppImage from ~/Applications (Oct 2026, beta_5.4)
- Got it running with game text visible (Oct 30 2026) on a patched build, launched with LARECOMP_VK_HEAP_DMABUF=1 LARECOMP_SOFT_WRITE_TRACKING=0 LARECOMP_GPU_OVERLAP=0; data folder is ~/Documents/LAS Recomp
