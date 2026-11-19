// SPDX-FileCopyrightText: Copyright 2026 Claude / Christian Ball (Thor Fritchii build)
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Thor bottom-screen helper — matches Claude's working AppImage behaviour:
//   load /armada-bottom-env → SDL window on bottom DISPLAY → X11 claim over
//   ArmadaLive ("taking it back") → blit shared /eden-duo-bottom-* frames.

#include "yuzu/duo/duo_bottom_helper.h"

#include "yuzu/duo/duo_bottom_shared.h"

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <dlfcn.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL3/SDL.h>

#include "common/logging.h"

namespace DuoQt {
namespace {

constexpr const char* kArmadaBottomEnv = "/armada-bottom-env";
constexpr const char* kHelperFlag = "--duo-bottom-helper";

std::atomic<bool> g_helper_stop{false};

void OnHelperSignal(int) {
    g_helper_stop.store(true, std::memory_order_relaxed);
}

bool EnvIsOff(const char* name) {
    const char* v = std::getenv(name);
    return v && (std::strcmp(v, "off") == 0 || std::strcmp(v, "0") == 0 ||
                 std::strcmp(v, "false") == 0);
}

bool LoadArmadaBottomEnv() {
    std::ifstream in(kArmadaBottomEnv);
    if (!in) {
        return false;
    }
    std::string line;
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
    }
    return true;
}

// ---- X11 claim (dlopen; no link-time libX11) ---------------------------------

using XDisplay = struct _XDisplay;
using XWindow = unsigned long;
using XAtom = unsigned long;
using XStatus = int;

struct X11Api {
    void* lib{};
    XDisplay* (*XOpenDisplay)(const char*){};
    int (*XCloseDisplay)(XDisplay*){};
    XStatus (*XGetWindowProperty)(XDisplay*, XWindow, XAtom, long, long, int, XAtom, XAtom*, int*,
                                  unsigned long*, unsigned long*, unsigned char**){};
    int (*XFree)(void*){};
    XAtom (*XInternAtom)(XDisplay*, const char*, int){};
    int (*XRaiseWindow)(XDisplay*, XWindow){};
    int (*XChangeProperty)(XDisplay*, XWindow, XAtom, XAtom, int, int, const unsigned char*, int){};
    int (*XFlush)(XDisplay*){};
    XStatus (*XQueryTree)(XDisplay*, XWindow, XWindow*, XWindow*, XWindow**, unsigned int*){};
    int (*XMapRaised)(XDisplay*, XWindow){};
};

bool LoadX11(X11Api& x) {
    x.lib = dlopen("libX11.so.6", RTLD_NOW | RTLD_LOCAL);
    if (!x.lib) {
        LOG_ERROR(Frontend, "duo bottom helper: no libX11");
        return false;
    }
#define LOAD(sym)                                                                                  \
    do {                                                                                           \
        x.sym = reinterpret_cast<decltype(x.sym)>(dlsym(x.lib, #sym));                             \
        if (!x.sym) {                                                                              \
            LOG_ERROR(Frontend, "duo bottom helper: missing {}", #sym);                            \
            return false;                                                                          \
        }                                                                                          \
    } while (0)
    LOAD(XOpenDisplay);
    LOAD(XCloseDisplay);
    LOAD(XGetWindowProperty);
    LOAD(XFree);
    LOAD(XInternAtom);
    LOAD(XRaiseWindow);
    LOAD(XChangeProperty);
    LOAD(XFlush);
    LOAD(XQueryTree);
    LOAD(XMapRaised);
#undef LOAD
    return true;
}

std::string CollectSteamAppIds(X11Api& x, XDisplay* dpy, XWindow root) {
    XWindow root_ret{}, parent{};
    XWindow* children{};
    unsigned int n = 0;
    if (!x.XQueryTree(dpy, root, &root_ret, &parent, &children, &n) || !children) {
        return {};
    }
    const XAtom steam_game = x.XInternAtom(dpy, "STEAM_GAME", /*only_if_exists=*/1);
    std::string ids;
    if (steam_game) {
        for (unsigned int i = 0; i < n; ++i) {
            XAtom actual_type{};
            int actual_format = 0;
            unsigned long nitems = 0, bytes_after = 0;
            unsigned char* prop = nullptr;
                const XStatus st = x.XGetWindowProperty(
                    dpy, children[i], steam_game, 0, 1, /*delete=*/0, /*AnyPropertyType*/ 0,
                    &actual_type, &actual_format, &nitems, &bytes_after, &prop);
            if (st == 0 /*Success*/ && prop && nitems > 0 && actual_format == 32) {
                const auto app = *reinterpret_cast<unsigned long*>(prop);
                if (!ids.empty()) {
                    ids.push_back(' ');
                }
                ids += std::to_string(app);
            }
            if (prop) {
                x.XFree(prop);
            }
        }
    }
    x.XFree(children);
    return ids;
}

void ClaimBottomScreen(X11Api& x, SDL_Window* window) {
    if (!window) {
        return;
    }
    const SDL_PropertiesID props = SDL_GetWindowProperties(window);
    void* display_ptr =
        SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    const auto xid =
        static_cast<XWindow>(SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
    if (!display_ptr || xid == 0) {
        return;
    }
    auto* dpy = static_cast<XDisplay*>(display_ptr);
    // XDefaultRootWindow is an Xlib macro — get root via QueryTree on our window.
    XWindow root = 0, parent = 0, *children = nullptr;
    unsigned int n = 0;
    if (x.XQueryTree(dpy, xid, &root, &parent, &children, &n) && children) {
        x.XFree(children);
    }
    if (root == 0) {
        root = xid;
    }

    const std::string app_ids = CollectSteamAppIds(x, dpy, root);
    LOG_INFO(Frontend, "duo bottom helper: bottom screen was showing app ids [ {}]; taking it back",
             app_ids);

    // Above + fullscreen WM hints so ArmadaLive / Steam stay underneath.
    const XAtom net_state = x.XInternAtom(dpy, "_NET_WM_STATE", 0);
    const XAtom atom_above = x.XInternAtom(dpy, "_NET_WM_STATE_ABOVE", 0);
    const XAtom atom_fs = x.XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", 0);
    const XAtom atom_atom = x.XInternAtom(dpy, "ATOM", 0);
    XAtom states[2] = {atom_above, atom_fs};
    x.XChangeProperty(dpy, xid, net_state, atom_atom, 32, /*PropModeReplace*/ 0,
                      reinterpret_cast<const unsigned char*>(states), 2);

    x.XMapRaised(dpy, xid);
    x.XRaiseWindow(dpy, xid);
    x.XFlush(dpy);

    LOG_INFO(Frontend, "duo bottom helper: claimed the bottom screen (window {})",
             static_cast<unsigned long>(xid));
}

int RunBottomHelperMain(const char* shared_path) {
    g_helper_stop.store(false);
    signal(SIGTERM, OnHelperSignal);
    signal(SIGINT, OnHelperSignal);

    if (const char* driver = std::getenv("EDEN_DUO_HELPER_DRIVER")) {
        setenv("SDL_VIDEODRIVER", driver, 1);
        setenv("QT_QPA_PLATFORM", driver, 0);
    }
    setenv("SDL_VIDEODRIVER", "x11", 0);

    LoadArmadaBottomEnv();

    const char* display = std::getenv("DISPLAY");
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    LOG_INFO(Frontend, "duo bottom helper: DISPLAY={} WAYLAND_DISPLAY={}",
             display ? display : "(null)", wayland ? wayland : "(null)");

    if (!display || !*display) {
        LOG_ERROR(Frontend, "duo bottom helper: not on X11 ({}), can't claim the screen",
                  wayland ? wayland : "no display");
        return 1;
    }
    LOG_INFO(Frontend, "duo bottom helper: bottom Xwayland {}", display);

    if (access(shared_path, R_OK) != 0) {
        LOG_ERROR(Frontend, "duo bottom helper: no shared companion at {}", shared_path);
        return 1;
    }

    setenv("EDEN_DUO_RUN_BOTTOM", "1", 1);

    X11Api x11{};
    const bool have_x11 = LoadX11(x11);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LOG_ERROR(Frontend, "duo bottom helper: SDL_Init failed: {}", SDL_GetError());
        return 1;
    }

    std::uint32_t win_w = 640, win_h = 740;
    if (const SDL_DisplayID* displays = SDL_GetDisplays(nullptr); displays != nullptr) {
        SDL_Rect bounds{};
        if (SDL_GetDisplayBounds(displays[0], &bounds) && bounds.w > 0 && bounds.h > 0) {
            win_w = static_cast<std::uint32_t>(bounds.w);
            win_h = static_cast<std::uint32_t>(bounds.h);
        }
        SDL_free(const_cast<SDL_DisplayID*>(displays));
    }

    SDL_Window* window = SDL_CreateWindow(
        "Fritchii Duo (bottom)", static_cast<int>(win_w), static_cast<int>(win_h),
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        LOG_ERROR(Frontend, "duo bottom helper: no window: {}", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        LOG_ERROR(Frontend, "duo bottom helper: no renderer: {}", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);

    LOG_INFO(Frontend, "duo bottom helper: {}x{} on {}", win_w, win_h, display);

    if (have_x11) {
        ClaimBottomScreen(x11, window);
    }

    SDL_Texture* texture = nullptr;
    std::uint32_t tex_w = 0, tex_h = 0;
    std::uint64_t last_seq = 0;
    std::uint32_t pictures = 0;
    std::uint32_t claim_tick = 0;

    while (!g_helper_stop.load(std::memory_order_relaxed)) {
        if (access(shared_path, R_OK) != 0) {
            break;
        }

        const int fd = open(shared_path, O_RDONLY);
        if (fd < 0) {
            SDL_Delay(16);
            continue;
        }
        struct stat st {};
        if (fstat(fd, &st) != 0 || st.st_size < static_cast<off_t>(sizeof(DuoBottomHeader))) {
            close(fd);
            SDL_Delay(16);
            continue;
        }
        void* map = mmap(nullptr, static_cast<size_t>(st.st_size), PROT_READ, MAP_SHARED, fd, 0);
        close(fd);
        if (map == MAP_FAILED) {
            SDL_Delay(16);
            continue;
        }

        const auto* header = static_cast<const DuoBottomHeader*>(map);
        if (DuoBottomHeaderOk(*header) && header->seq != last_seq) {
            const std::uint32_t w = header->width;
            const std::uint32_t h = header->height;
            const size_t need = DuoBottomMapBytes(w, h);
            if (static_cast<size_t>(st.st_size) >= need) {
                if (!texture || tex_w != w || tex_h != h) {
                    if (texture) {
                        SDL_DestroyTexture(texture);
                    }
                    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                                SDL_TEXTUREACCESS_STREAMING, static_cast<int>(w),
                                                static_cast<int>(h));
                    tex_w = w;
                    tex_h = h;
                }
                if (texture) {
                    const auto* pixels = reinterpret_cast<const std::uint8_t*>(map) +
                                         DuoBottomPixelsOffset();
                    SDL_UpdateTexture(texture, nullptr, pixels,
                                      static_cast<int>(header->stride_bytes));
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderClear(renderer);
                    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
                    SDL_RenderPresent(renderer);
                    last_seq = header->seq;
                    ++pictures;
                }
            }
        }
        munmap(map, static_cast<size_t>(st.st_size));

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                g_helper_stop.store(true);
            }
        }

        // ArmadaLive / Steam may fight for stacking — re-claim periodically.
        if (have_x11 && (++claim_tick % 45) == 0) {
            ClaimBottomScreen(x11, window);
        }

        SDL_Delay(8);
    }

    if (texture) {
        SDL_DestroyTexture(texture);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (x11.lib) {
        dlclose(x11.lib);
    }

    LOG_INFO(Frontend, "duo bottom helper: done, {} pictures shown", pictures);
    return 0;
}

} // namespace

bool DuoScreenEnabled() {
    if (EnvIsOff("EDEN_DUO_SCREEN")) {
        LOG_INFO(Frontend, "Thor bottom: companion screen off (EDEN_DUO_SCREEN=off)");
        return false;
    }
    return true;
}

std::optional<int> MaybeRunBottomHelper(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], kHelperFlag) == 0) {
            const char* path = (i + 1 < argc) ? argv[i + 1] : nullptr;
            if (!path || !*path) {
                LOG_ERROR(Frontend, "usage: {} {} <shared-companion-path>", argv[0], kHelperFlag);
                return 2;
            }
            return RunBottomHelperMain(path);
        }
    }
    return std::nullopt;
}

int MaybeStartThorBottomHelper(const std::string& shared_companion_path) {
    if (!DuoScreenEnabled()) {
        return -1;
    }
    if (access(kArmadaBottomEnv, R_OK) != 0) {
        return -1;
    }
    if (shared_companion_path.empty()) {
        return -1;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        LOG_ERROR(Frontend, "duo bottom helper: fork failed");
        return -1;
    }
    if (pid == 0) {
        const char* self = "/proc/self/exe";
        execl(self, self, kHelperFlag, shared_companion_path.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }
    LOG_INFO(Frontend, "helper: {} {} {}", "/proc/self/exe", kHelperFlag, shared_companion_path);
    return static_cast<int>(pid);
}

void StopThorBottomHelper(int helper_pid) {
    if (helper_pid > 0) {
        kill(helper_pid, SIGTERM);
    }
}

} // namespace DuoQt
