// SPDX-FileCopyrightText: Copyright 2026 Eden DSMod fork / Thor Fritchii
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Second-screen SDL window for AYN Thor bottom display (Game Mode). Adapted from
// Duo's AuxWindow_SDL3 harness so the Qt frontend can call Renderer::SetAuxWindow.

#pragma once

#include <memory>
#include <utility>

#include <SDL3/SDL.h>

#include "common/common_types.h"
#include "core/frontend/emu_window.h"
#include "video_core/dsmod/aux_routing.h"

namespace Core {
class System;
}

namespace DuoQt {

class DuoAuxWindow final : public Core::Frontend::EmuWindow {
public:
    explicit DuoAuxWindow(Core::System& system_, u32 width, u32 height, bool fullscreen);
    ~DuoAuxWindow() override;

    std::unique_ptr<Core::Frontend::GraphicsContext> CreateSharedContext() const override;

    bool IsShown() const override {
        return is_shown && window != nullptr;
    }

    bool IsValid() const {
        return window != nullptr && window_info.render_surface != nullptr;
    }

    bool OwnsEvent(const SDL_Event& event) const;
    void OnEvent(SDL_Event& event);

private:
    void OnResize();
    void PublishTouch(bool down, bool started);
    void ResetInput();
    std::pair<u32, u32> ToPixels(float x, float y) const;

    Core::System& system;
    SDL_Window* window{};
    SDL_WindowID window_id{};
    bool is_shown = true;
    bool mouse_down = false;
    float mouse_x{};
    float mouse_y{};
    VideoCore::DSMod::AuxFingerTracker fingers;
    bool native_pinch_active{};
    u32 native_pinch_x{}, native_pinch_y{};
};

/// Attach (or no-op) a Thor bottom-screen aux window when /armada-bottom-env is present.
void AttachThorAuxWindow(Core::System& system);
/// Detach and destroy the Thor aux window.
void DetachThorAuxWindow(Core::System& system);
/// Pump SDL events for the aux window (call from a Qt timer while attached).
void PumpThorAuxEvents();

} // namespace DuoQt
