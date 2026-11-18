// SPDX-FileCopyrightText: Copyright 2026 Claude / Christian Ball (Thor Fritchii build)
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Recreated from Thor AppImage strings:
//   EDEN_DUO_SCREEN / EDEN_DUO_RUN_BOTTOM / EDEN_DUO_HELPER_DRIVER
//   --duo-bottom-helper
//   "Thor bottom: companion screen off (EDEN_DUO_SCREEN=off)"
//   "duo bottom helper: ..."
//   /armada-bottom-env
//
// On the AYN Thor under ArmadaOS Game Mode, the companion is shown on the bottom
// screen by re-execing this process as a helper that claims the bottom Xwayland
// display (from /armada-bottom-env) and presents a shared companion framebuffer.

#pragma once

#include <optional>
#include <string>

namespace DuoQt {

/// True when EDEN_DUO_SCREEN is unset or not "off".
[[nodiscard]] bool DuoScreenEnabled();

/// If argv requested --duo-bottom-helper <shm-path>, run the helper and return
/// the process exit code. Returns nullopt when this is a normal UI launch.
[[nodiscard]] std::optional<int> MaybeRunBottomHelper(int argc, char** argv);

/// After the main window has a Vulkan aux presenter, spawn the bottom-screen
/// helper for Thor when DuoScreenEnabled() and /armada-bottom-env exists.
/// No-ops on desktops without that file.
void MaybeStartThorBottomHelper(const std::string& shared_companion_path);

} // namespace DuoQt
