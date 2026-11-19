// SPDX-FileCopyrightText: Copyright 2026 Eden DSMod fork / Thor Fritchii
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/duo/duo_aux_window.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>

#include <unistd.h>

#include "common/logging.h"
#include "core/core.h"
#include "core/frontend/framebuffer_layout.h"
#include "qt_common/render/context.h"
#include "video_core/dsmod/aux_routing.h"
#include "video_core/gpu.h"
#include "video_core/renderer_base.h"
#include "yuzu/duo/duo_bottom_helper.h"

namespace DuoQt {
namespace {

constexpr const char* kArmadaBottomEnv = "/armada-bottom-env";

std::unique_ptr<DuoAuxWindow> g_aux;
bool g_sdl_video_ready = false;

bool FillWindowSystemInfoFromSDL(SDL_Window* window, Core::Frontend::EmuWindow::WindowSystemInfo& info) {
    if (window == nullptr) {
        return false;
    }
    const SDL_PropertiesID props = SDL_GetWindowProperties(window);
    const char* driver = SDL_GetCurrentVideoDriver();
    if (driver == nullptr) {
        return false;
    }
#if defined(SDL_PLATFORM_LINUX) || defined(__linux__)
    if (std::strcmp(driver, "x11") == 0) {
        info.type = Core::Frontend::WindowSystemType::X11;
        info.display_connection =
            SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        const auto xid =
            SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        info.render_surface = reinterpret_cast<void*>(static_cast<uintptr_t>(xid));
        return info.display_connection != nullptr && xid != 0;
    }
    if (std::strcmp(driver, "wayland") == 0) {
        info.type = Core::Frontend::WindowSystemType::Wayland;
        info.display_connection =
            SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
        info.render_surface =
            SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
        return info.display_connection != nullptr && info.render_surface != nullptr;
    }
#endif
    return false;
}

// KEY=VALUE from /armada-bottom-env → process env (returns whether DISPLAY was set).
bool LoadArmadaBottomEnv(std::string* display_out) {
    std::ifstream in(kArmadaBottomEnv);
    if (!in) {
        return false;
    }
    std::string line;
    bool have_display = false;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if (!val.empty() && val.front() == '"' && val.back() == '"') {
            val = val.substr(1, val.size() - 2);
        }
        setenv(key.c_str(), val.c_str(), 1);
        if (key == "DISPLAY" && !val.empty()) {
            have_display = true;
            if (display_out) {
                *display_out = val;
            }
        }
    }
    return have_display;
}

bool EnsureSdlVideo() {
    if (g_sdl_video_ready) {
        return true;
    }
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        LOG_ERROR(Frontend, "Duo aux: SDL_InitSubSystem(VIDEO) failed: {}", SDL_GetError());
        return false;
    }
    g_sdl_video_ready = true;
    return true;
}

} // namespace

DuoAuxWindow::DuoAuxWindow(Core::System& system_, u32 width, u32 height, bool fullscreen)
    : system{system_} {
    Uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }
    window = SDL_CreateWindow("Fritchii Duo (bottom)", static_cast<int>(width),
                              static_cast<int>(height), flags);
    if (window == nullptr) {
        LOG_CRITICAL(Frontend, "Duo aux: Failed to create SDL window: {}", SDL_GetError());
        return;
    }
    window_id = SDL_GetWindowID(window);
    if (!FillWindowSystemInfoFromSDL(window, window_info)) {
        LOG_CRITICAL(Frontend, "Duo aux: unable to determine native window backend");
        SDL_DestroyWindow(window);
        window = nullptr;
        return;
    }
    OnResize();
    LOG_INFO(Frontend, "Duo aux window created ({}x{}, id {}, fullscreen={})", width, height,
             window_id, fullscreen);
}

DuoAuxWindow::~DuoAuxWindow() {
    if (window != nullptr) {
        SDL_DestroyWindow(window);
    }
}

std::unique_ptr<Core::Frontend::GraphicsContext> DuoAuxWindow::CreateSharedContext() const {
    return std::make_unique<DummyContext>();
}

bool DuoAuxWindow::OwnsEvent(const SDL_Event& event) const {
    if (window == nullptr) {
        return false;
    }
    if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) {
        return event.window.windowID == window_id;
    }
    switch (event.type) {
    case SDL_EVENT_MOUSE_MOTION:
        return event.motion.windowID == window_id;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return event.button.windowID == window_id;
    case SDL_EVENT_MOUSE_WHEEL:
        return event.wheel.windowID == window_id;
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_MOTION:
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
        return event.tfinger.windowID == window_id;
    default:
        return false;
    }
}

void DuoAuxWindow::OnEvent(SDL_Event& event) {
    switch (event.type) {
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_MAXIMIZED:
    case SDL_EVENT_WINDOW_RESTORED:
        return OnResize();
    case SDL_EVENT_WINDOW_MINIMIZED:
        is_shown = false;
        return ResetInput();
    case SDL_EVENT_WINDOW_HIDDEN:
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        return ResetInput();
    case SDL_EVENT_WINDOW_EXPOSED:
        is_shown = true;
        return OnResize();
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        SDL_HideWindow(window);
        is_shown = false;
        return ResetInput();
    case SDL_EVENT_MOUSE_MOTION:
        if (event.motion.which == SDL_TOUCH_MOUSEID || !fingers.Empty()) {
            return;
        }
        mouse_x = event.motion.x;
        mouse_y = event.motion.y;
        if (mouse_down) {
            PublishTouch(true, false);
        }
        return;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event.button.which == SDL_TOUCH_MOUSEID || !fingers.Empty()) {
            return;
        }
        if (event.button.button != SDL_BUTTON_LEFT) {
            return;
        }
        mouse_down = true;
        mouse_x = event.button.x;
        mouse_y = event.button.y;
        return PublishTouch(true, true);
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event.button.which == SDL_TOUCH_MOUSEID || !fingers.Empty()) {
            return;
        }
        if (event.button.button != SDL_BUTTON_LEFT) {
            return;
        }
        mouse_down = false;
        return PublishTouch(false, false);
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_MOTION:
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED: {
        // Finger coordinates are normalized; map through window pixel size.
        int pw = 1, ph = 1;
        SDL_GetWindowSizeInPixels(window, &pw, &ph);
        const float fx = event.tfinger.x * static_cast<float>(pw);
        const float fy = event.tfinger.y * static_cast<float>(ph);
        const bool down =
            event.type == SDL_EVENT_FINGER_DOWN || event.type == SDL_EVENT_FINGER_MOTION;
        const bool started = event.type == SDL_EVENT_FINGER_DOWN;
        if (!down) {
            fingers.Clear();
            system.GPU().DSModAux().SetTouch({});
            return;
        }
        mouse_x = fx;
        mouse_y = fy;
        mouse_down = true;
        return PublishTouch(true, started);
    }
    default:
        return;
    }
}

void DuoAuxWindow::OnResize() {
    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(window, &width, &height);
    if (width <= 0 || height <= 0) {
        return;
    }
    Layout::FramebufferLayout layout{};
    layout.width = static_cast<u32>(width);
    layout.height = static_cast<u32>(height);
    layout.screen = Common::Rectangle<u32>{0, 0, static_cast<u32>(width), static_cast<u32>(height)};
    layout.is_srgb = false;
    NotifyFramebufferLayoutChanged(layout);
    auto& aux = system.GPU().DSModAux();
    aux.width.store(layout.width);
    aux.height.store(layout.height);
}

void DuoAuxWindow::PublishTouch(bool down, bool started) {
    auto& aux = system.GPU().DSModAux();
    if (!down) {
        aux.SetTouch({});
        return;
    }
    const auto [x, y] = ToPixels(mouse_x, mouse_y);
    const VideoCore::DSMod::AuxTouchPoint point{
        .finger_id = 0,
        .x = x,
        .y = y,
        .attributes = started ? 1u : 0u,
        .delta_ns = 0,
    };
    aux.SetTouch(std::span{&point, 1});
}

void DuoAuxWindow::ResetInput() {
    auto& aux = system.GPU().DSModAux();
    if (native_pinch_active) {
        aux.PushPinch(
            {VideoCore::DSMod::AuxPinchEvent::Phase::End, native_pinch_x, native_pinch_y, 1.0f});
        native_pinch_active = false;
    }
    if (mouse_down || !fingers.Empty()) {
        fingers.Clear();
        mouse_down = false;
        aux.SetTouch({});
    }
}

std::pair<u32, u32> DuoAuxWindow::ToPixels(float x, float y) const {
    int ww = 1, wh = 1, pw = 1, ph = 1;
    SDL_GetWindowSize(window, &ww, &wh);
    SDL_GetWindowSizeInPixels(window, &pw, &ph);
    const float sx = static_cast<float>(pw) / static_cast<float>(std::max(ww, 1));
    const float sy = static_cast<float>(ph) / static_cast<float>(std::max(wh, 1));
    return {static_cast<u32>(std::max(0.0f, x * sx)), static_cast<u32>(std::max(0.0f, y * sy))};
}

void AttachThorAuxWindow(Core::System& system) {
    DetachThorAuxWindow(system);
    if (!DuoScreenEnabled()) {
        return;
    }
    if (access(kArmadaBottomEnv, R_OK) != 0) {
        LOG_INFO(Frontend, "Duo aux: no {}; skipping bottom screen (not Game Mode?)",
                 kArmadaBottomEnv);
        return;
    }

    std::string bottom_display;
    if (!LoadArmadaBottomEnv(&bottom_display)) {
        LOG_ERROR(Frontend, "Duo aux: {} has no DISPLAY", kArmadaBottomEnv);
        return;
    }

    // Remember the top-screen DISPLAY (Qt/gamescope); SDL opens the bottom X server.
    const char* previous_display = std::getenv("DISPLAY");
    std::string previous = previous_display ? previous_display : "";
    setenv("DISPLAY", bottom_display.c_str(), 1);
    setenv("SDL_VIDEODRIVER", "x11", 1);

    if (!EnsureSdlVideo()) {
        if (!previous.empty()) {
            setenv("DISPLAY", previous.c_str(), 1);
        }
        return;
    }

    // Prefer the first display's bounds on the bottom X server; fall back to 640x740.
    u32 w = 640, h = 740;
    bool fullscreen = true;
    if (const SDL_DisplayID* displays = SDL_GetDisplays(nullptr); displays != nullptr) {
        const SDL_DisplayID id = displays[0];
        SDL_Rect bounds{};
        if (SDL_GetDisplayBounds(id, &bounds) && bounds.w > 0 && bounds.h > 0) {
            w = static_cast<u32>(bounds.w);
            h = static_cast<u32>(bounds.h);
        }
        SDL_free(const_cast<SDL_DisplayID*>(displays));
    }

    g_aux = std::make_unique<DuoAuxWindow>(system, w, h, fullscreen);
    if (!previous.empty()) {
        setenv("DISPLAY", previous.c_str(), 1);
    }

    if (!g_aux || !g_aux->IsValid()) {
        LOG_ERROR(Frontend, "Duo aux: window invalid on DISPLAY={}", bottom_display);
        g_aux.reset();
        return;
    }

    system.Renderer().SetAuxWindow(g_aux.get());
    LOG_INFO(Frontend, "Thor bottom: companion aux window attached ({}x{} on {})", w, h,
             bottom_display);
}

void DetachThorAuxWindow(Core::System& system) {
    if (g_aux) {
        system.Renderer().SetAuxWindow(nullptr);
        g_aux.reset();
        LOG_INFO(Frontend, "Thor bottom: companion aux window detached");
    }
}

void PumpThorAuxEvents() {
    if (!g_aux) {
        return;
    }
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (g_aux->OwnsEvent(event)) {
            g_aux->OnEvent(event);
        }
    }
}

} // namespace DuoQt
