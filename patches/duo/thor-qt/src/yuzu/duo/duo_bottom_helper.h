// SPDX-FileCopyrightText: Copyright 2026 Claude / Christian Ball (Thor Fritchii build)
// SPDX-License-Identifier: GPL-3.0-or-later
//
// On AYN Thor ArmadaOS Game Mode, the companion is shown on the bottom screen by
// re-execing this process as --duo-bottom-helper: it loads /armada-bottom-env,
// opens an SDL window on the bottom Xwayland DISPLAY, raises it above ArmadaLive
// (XRaiseWindow / XChangeProperty), and presents frames from a shared mmap file.

#pragma once

#include <optional>
#include <string>

namespace DuoQt {

/// True when EDEN_DUO_SCREEN is unset or not "off".
[[nodiscard]] bool DuoScreenEnabled();

/// If argv requested --duo-bottom-helper <shm-path>, run the helper and return
/// the process exit code. Returns nullopt when this is a normal UI launch.
[[nodiscard]] std::optional<int> MaybeRunBottomHelper(int argc, char** argv);

/// Spawn --duo-bottom-helper for Thor when DuoScreenEnabled() and
/// /armada-bottom-env exists. Returns child pid, or -1 on skip/failure.
[[nodiscard]] int MaybeStartThorBottomHelper(const std::string& shared_companion_path);

/// SIGTERM a previously started helper (pid from MaybeStartThorBottomHelper).
void StopThorBottomHelper(int helper_pid);

} // namespace DuoQt
