// SPDX-FileCopyrightText: Copyright 2026 Claude / Christian Ball (Thor Fritchii build)
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Parent-side Thor bottom bridge: create shared companion framebuffer, spawn
// --duo-bottom-helper, and copy DSMod aux UI pixels into the mapping each tick.

#pragma once

namespace Core {
class System;
}

namespace DuoQt {

/// Start helper + shared buffer when /armada-bottom-env is present (Game Mode).
void StartThorBottomBridge(Core::System& system);

/// Copy latest companion UI into the shared buffer (call from a Qt timer).
void PumpThorBottomBridge(Core::System& system);

/// Stop helper, unmap, unlink shared file.
void StopThorBottomBridge();

} // namespace DuoQt
