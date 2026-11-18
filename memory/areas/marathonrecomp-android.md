---
name: "marathonrecomp-android"
description: "Porting Xbox 360 static recompilation projects to Android; current MarathonRecomp investigation"
---

- Investigating whether MarathonRecomp (Sonic '06) can be ported to Android following the same approach used for a prior UnleashedRecomp Android port
- Built and troubleshot a GitHub Actions-based Android APK build pipeline for a fork of UnleashedRecomp_Android (github.com/christianball123/UnleashedRecomp_Android)
- Resolved memory-related runner crashes in the pipeline by reducing CMake parallel compile jobs
