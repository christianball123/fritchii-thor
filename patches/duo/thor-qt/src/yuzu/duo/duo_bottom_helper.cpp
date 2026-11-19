// SPDX-FileCopyrightText: Copyright 2026 Claude / Christian Ball (Thor Fritchii build)
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Thor bottom-screen helper matching Claude's working AppImage (thor_bottom.cpp):
//   /armada-bottom-env → SDL window → STEAM_GAME=852199 claim → blit shared DOUB map.

#include "yuzu/duo/duo_bottom_helper.h"

#include "yuzu/duo/duo_bottom_shared.h"

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
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
constexpr const char* kArmadaRunBottom = "/usr/bin/armada-run-bottom";
// Claude's binary stamps this Steam app id so gamescope/Armada shows the window
// on the bottom layer (not _NET_WM_STATE).
constexpr std::uint32_t kThorSteamGameId = 852199u; // 0xd00e7
constexpr unsigned long kXaCardinal = 6;            // XA_CARDINAL

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
    const char* override_path = std::getenv("EDEN_DUO_BOTTOM_ENV");
    const char* path =
        (override_path && *override_path) ? override_path : kArmadaBottomEnv;
    std::ifstream in(path);
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

using XDisplay = struct _XDisplay;
using XWindow = unsigned long;
using XAtom = unsigned long;
using XStatus = int;

struct X11Api {
    void* lib{};
    XAtom (*XInternAtom)(XDisplay*, const char*, int){};
    XWindow (*XDefaultRootWindow)(XDisplay*){};
    int (*XChangeProperty)(XDisplay*, XWindow, XAtom, XAtom, int, int, const unsigned char*,
                           int){};
    XStatus (*XGetWindowProperty)(XDisplay*, XWindow, XAtom, long, long, int, XAtom, XAtom*, int*,
                                  unsigned long*, unsigned long*, unsigned char**){};
    int (*XFlush)(XDisplay*){};
    int (*XRaiseWindow)(XDisplay*, XWindow){};
    int (*XFree)(void*){};
    XStatus (*XQueryTree)(XDisplay*, XWindow, XWindow*, XWindow*, XWindow**, unsigned int*){};
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
    } while (0)
    LOAD(XInternAtom);
    LOAD(XDefaultRootWindow); // may be missing (macro); QueryTree fallback below
    LOAD(XChangeProperty);
    LOAD(XGetWindowProperty);
    LOAD(XFlush);
    LOAD(XRaiseWindow);
    LOAD(XFree);
    LOAD(XQueryTree);
#undef LOAD
    if (!x.XInternAtom || !x.XChangeProperty || !x.XGetWindowProperty || !x.XFlush ||
        !x.XRaiseWindow || !x.XFree) {
        LOG_ERROR(Frontend, "duo bottom helper: incomplete libX11");
        return false;
    }
    return true;
}

XWindow ResolveRoot(X11Api& x, XDisplay* dpy, XWindow xid) {
    if (x.XDefaultRootWindow) {
        return x.XDefaultRootWindow(dpy);
    }
    if (x.XQueryTree) {
        XWindow root = 0, parent = 0, *children = nullptr;
        unsigned int n = 0;
        if (x.XQueryTree(dpy, xid, &root, &parent, &children, &n)) {
            if (children) {
                x.XFree(children);
            }
            if (root) {
                return root;
            }
        }
    }
    return xid;
}

std::string CollectSteamAppIds(X11Api& x, XDisplay* dpy, XWindow root) {
    if (!x.XQueryTree) {
        return {};
    }
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
            if (st == 0 && prop && nitems > 0 && actual_format == 32) {
                const auto app = static_cast<unsigned long>(*reinterpret_cast<std::uint32_t*>(prop));
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
    const XWindow root = ResolveRoot(x, dpy, xid);

    const std::string app_ids = CollectSteamAppIds(x, dpy, root);
    LOG_INFO(Frontend, "duo bottom helper: bottom screen was showing app ids [ {}]; taking it back",
             app_ids);

    // Gamescope / ArmadaLive: mark this window as the Steam game layer (Claude: 852199).
    const XAtom steam = x.XInternAtom(dpy, "STEAM_GAME", 0);
    const XAtom baselayer = x.XInternAtom(dpy, "GAMESCOPECTRL_BASELAYER_APPID", 0);
    std::uint32_t appid = kThorSteamGameId;
    x.XChangeProperty(dpy, xid, steam, kXaCardinal, 32, /*PropModeReplace*/ 0,
                      reinterpret_cast<const unsigned char*>(&appid), 1);
    if (baselayer) {
        x.XChangeProperty(dpy, xid, baselayer, kXaCardinal, 32, 0,
                          reinterpret_cast<const unsigned char*>(&appid), 1);
    }
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
    }
    setenv("SDL_VIDEODRIVER", "x11", 0);

    // armada-run-bottom may create /armada-bottom-env just before/during our start.
    for (int i = 0; i < 50; ++i) {
        LoadArmadaBottomEnv();
        const char* d = std::getenv("DISPLAY");
        if (d && *d) {
            break;
        }
        usleep(100000);
    }

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

    // Claude used 1240×1080 “Eden Duo Companion”; fall back to display bounds.
    std::uint32_t win_w = 1240, win_h = 1080;
    if (const SDL_DisplayID* displays = SDL_GetDisplays(nullptr); displays != nullptr) {
        SDL_Rect bounds{};
        if (SDL_GetDisplayBounds(displays[0], &bounds) && bounds.w > 0 && bounds.h > 0) {
            win_w = static_cast<std::uint32_t>(bounds.w);
            win_h = static_cast<std::uint32_t>(bounds.h);
        }
        SDL_free(const_cast<SDL_DisplayID*>(displays));
    }

    SDL_Window* window =
        SDL_CreateWindow("Eden Duo Companion", static_cast<int>(win_w), static_cast<int>(win_h),
                         SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS);
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

    const int fd = open(shared_path, O_RDWR);
    if (fd < 0) {
        LOG_ERROR(Frontend, "duo bottom helper: no shared companion at {}", shared_path);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    void* map = mmap(nullptr, kDuoBottomMapBytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (map == MAP_FAILED) {
        LOG_ERROR(Frontend, "duo bottom helper: mmap failed");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    DuoBottomWriteU32(map, 0x0c, static_cast<std::uint32_t>(getpid()));

    SDL_Texture* texture = nullptr;
    std::uint32_t tex_w = 0, tex_h = 0;
    std::uint64_t last_seq = 0;
    std::uint32_t pictures = 0;
    std::uint32_t claim_tick = 0;

    while (!g_helper_stop.load(std::memory_order_relaxed)) {
        if (access(shared_path, R_OK) != 0) {
            break;
        }

        if (DuoBottomHeaderOk(map)) {
            const std::uint64_t seq = DuoBottomReadU64(map, 0x68);
            if (seq != last_seq) {
                const std::uint32_t w = DuoBottomReadU32(map, 0x70);
                const std::uint32_t h = DuoBottomReadU32(map, 0x74);
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
                    const auto* pixels =
                        reinterpret_cast<const std::uint8_t*>(map) + kDuoBottomPixelsOff;
                    SDL_UpdateTexture(texture, nullptr, pixels, static_cast<int>(w * 4));
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderClear(renderer);
                    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
                    SDL_RenderPresent(renderer);
                    DuoBottomWriteU32(map, 0x20, 3);
                    last_seq = seq;
                    ++pictures;
                }
            }
        }

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                g_helper_stop.store(true);
            }
        }

        if (have_x11 && (++claim_tick % 45) == 0) {
            ClaimBottomScreen(x11, window);
        }
        SDL_Delay(8);
    }

    munmap(map, kDuoBottomMapBytes);
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

std::string ResolveSelfExe() {
    char buf[4096];
    const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        return std::string(buf);
    }
    return "eden";
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
    if (shared_companion_path.empty()) {
        return -1;
    }

    const std::string self = ResolveSelfExe();
    const bool use_armada = access(kArmadaRunBottom, X_OK) == 0;
    const char* override_path = std::getenv("EDEN_DUO_BOTTOM_ENV");
    const bool have_env =
        (override_path && *override_path && access(override_path, R_OK) == 0) ||
        access(kArmadaBottomEnv, R_OK) == 0;
    // On Thor, armada-run-bottom alone is enough — it prepares the bottom DISPLAY.
    if (!have_env && !use_armada) {
        return -1;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        LOG_ERROR(Frontend, "duo bottom helper: fork failed");
        return -1;
    }
    if (pid == 0) {
        setsid();
        // Log child output like Claude's /duo-bottom-runner.log
        const int logfd = open("/tmp/duo-bottom-runner.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (logfd >= 0) {
            dup2(logfd, STDOUT_FILENO);
            dup2(logfd, STDERR_FILENO);
            close(logfd);
        }
        if (use_armada) {
            execl(kArmadaRunBottom, kArmadaRunBottom, self.c_str(), kHelperFlag,
                  shared_companion_path.c_str(), static_cast<char*>(nullptr));
        }
        execl(self.c_str(), self.c_str(), kHelperFlag, shared_companion_path.c_str(),
              static_cast<char*>(nullptr));
        _exit(127);
    }
    LOG_INFO(Frontend, "helper: {} {} {}", use_armada ? kArmadaRunBottom : self.c_str(),
             kHelperFlag, shared_companion_path);
    return static_cast<int>(pid);
}

void StopThorBottomHelper(int helper_pid) {
    if (helper_pid > 0) {
        kill(helper_pid, SIGTERM);
    }
}

} // namespace DuoQt
