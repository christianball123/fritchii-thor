// SPDX-FileCopyrightText: Copyright 2026 Claude / Christian Ball (Thor Fritchii build)
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Minimal Thor bottom-screen helper. Claude's binary linked SDL for the helper
// path ("duo bottom helper: SDL_Init failed"). Full pixel pumping into the
// shared companion buffer still needs the video_core DSMod aux path; this file
// recreates process/env orchestration so the Thor AppImage packaging works.

#include "yuzu/duo/duo_bottom_helper.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>

#include <unistd.h>

#include "common/logging.h"

namespace DuoQt {
namespace {

constexpr const char* kArmadaBottomEnv = "/armada-bottom-env";
constexpr const char* kHelperFlag = "--duo-bottom-helper";

bool EnvIsOff(const char* name) {
    const char* v = std::getenv(name);
    return v && (std::strcmp(v, "off") == 0 || std::strcmp(v, "0") == 0 ||
                 std::strcmp(v, "false") == 0);
}

// Parse KEY=VALUE lines from /armada-bottom-env into the process environment.
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

int RunBottomHelperMain(const char* shared_path) {
    // Optional driver override used by Claude's build.
    if (const char* driver = std::getenv("EDEN_DUO_HELPER_DRIVER")) {
        setenv("SDL_VIDEODRIVER", driver, 1);
        setenv("QT_QPA_PLATFORM", driver, 0);
    }

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

    if (access(shared_path, R_OK) != 0) {
        LOG_ERROR(Frontend, "duo bottom helper: no shared companion at {}", shared_path);
        return 1;
    }

    // Claude logged: "duo bottom helper: bottom screen was showing app ids [...]; taking it back"
    // and "bottom Xwayland ...". Claiming the screen and presenting frames is left to the
    // DSMod aux presenter once Composite() is wired; the helper stays alive while the parent
    // keeps the shared mapping updated.
    setenv("EDEN_DUO_RUN_BOTTOM", "1", 1);
    LOG_INFO(Frontend, "Thor bottom: companion goes to the bottom screen through {} (helper {}, {})",
             display, shared_path, kArmadaBottomEnv);

    // Stay resident until the parent dies or the shared path disappears.
    while (access(shared_path, R_OK) == 0) {
        pause(); // woken by signals; parent should SIGTERM on exit
    }
    LOG_INFO(Frontend, "duo bottom helper: done");
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

void MaybeStartThorBottomHelper(const std::string& shared_companion_path) {
    if (!DuoScreenEnabled()) {
        return;
    }
    if (access(kArmadaBottomEnv, R_OK) != 0) {
        return; // not a Thor/Armada Game Mode session
    }
    if (shared_companion_path.empty()) {
        return;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        LOG_ERROR(Frontend, "duo bottom helper: fork failed");
        return;
    }
    if (pid == 0) {
        // child: re-exec self as helper
        const char* self = "/proc/self/exe";
        execl(self, self, kHelperFlag, shared_companion_path.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }
    LOG_INFO(Frontend, "helper: {} {} {}", "/proc/self/exe", kHelperFlag, shared_companion_path);
}

} // namespace DuoQt
