// SPDX-FileCopyrightText: Copyright 2026 Claude / Christian Ball (Thor Fritchii build)
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/duo/duo_bottom_bridge.h"

#include "yuzu/duo/duo_bottom_helper.h"
#include "yuzu/duo/duo_bottom_shared.h"

#include <cerrno>
#include <cstring>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/common_types.h"
#include "common/logging.h"
#include "core/core.h"
#include "video_core/dsmod/aux_routing.h"
#include "video_core/gpu.h"

namespace DuoQt {
namespace {

constexpr const char* kArmadaBottomEnv = "/armada-bottom-env";

int g_helper_pid = -1;
int g_fd = -1;
void* g_map = nullptr;
std::string g_path;
u64 g_ui_serial = 0;
std::vector<u32> g_ui_scratch;

std::string MakeSharedPath() {
    const char* runtime = std::getenv("XDG_RUNTIME_DIR");
    if (!runtime || !*runtime) {
        runtime = "/tmp";
    }
    return std::string(runtime) + "/eden-duo-bottom-" + std::to_string(getpid());
}

bool CreateMap() {
    if (g_map) {
        return true;
    }
    if (g_fd < 0) {
        return false;
    }
    if (ftruncate(g_fd, static_cast<off_t>(kDuoBottomMapBytes)) != 0) {
        LOG_ERROR(Frontend, "Thor bottom: cannot create {}: {}", g_path, std::strerror(errno));
        return false;
    }
    void* map = mmap(nullptr, kDuoBottomMapBytes, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, 0);
    if (map == MAP_FAILED) {
        LOG_ERROR(Frontend, "Thor bottom: cannot create {}: {}", g_path, std::strerror(errno));
        return false;
    }
    std::memset(map, 0, kDuoBottomHeaderBytes);
    DuoBottomWriteU32(map, 0x00, kDuoBottomMagic);
    DuoBottomWriteU32(map, 0x04, kDuoBottomVersion);
    DuoBottomWriteU32(map, 0x08, static_cast<std::uint32_t>(getpid()));
    DuoBottomWriteU32(map, 0x70, 1240);
    DuoBottomWriteU32(map, 0x74, 1080);
    DuoBottomWriteU64(map, 0x68, 0);
    g_map = map;
    LOG_INFO(Frontend, "Thor bottom: helper screen is {}x{}", 1240, 1080);
    return true;
}

} // namespace

void StartThorBottomBridge(Core::System& system) {
    StopThorBottomBridge();

    if (!DuoScreenEnabled()) {
        return;
    }
    const bool armada = access(kArmadaBottomEnv, R_OK) == 0;
    LOG_INFO(Frontend, "Thor bottom: companion found (asked {}, armada bottom session {})", true,
             armada);
    if (!armada) {
        return;
    }

    g_path = MakeSharedPath();
    g_fd = open(g_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (g_fd < 0) {
        LOG_ERROR(Frontend, "Thor bottom: cannot create {}: {}", g_path, std::strerror(errno));
        g_path.clear();
        return;
    }
    if (!CreateMap()) {
        StopThorBottomBridge();
        return;
    }

    if (system.IsPoweredOn()) {
        auto& aux = system.GPU().DSModAux();
        aux.width.store(1240);
        aux.height.store(1080);
        aux.present.store(true);
        aux.panel_is_inline.store(false);
    }

    g_helper_pid = MaybeStartThorBottomHelper(g_path);
    if (g_helper_pid < 0) {
        LOG_ERROR(Frontend, "Thor bottom: runner ended with code {} (output in {})", -1,
                  "/tmp/duo-bottom-runner.log");
        StopThorBottomBridge();
        return;
    }

    LOG_INFO(Frontend, "Thor bottom: companion goes to the bottom screen through {} (helper {}, {})",
             std::getenv("DISPLAY") ? std::getenv("DISPLAY") : "(null)", g_path, kArmadaBottomEnv);
}

void PumpThorBottomBridge(Core::System& system) {
    if (!g_map || g_fd < 0) {
        return;
    }
    if (!system.IsPoweredOn()) {
        return;
    }

    auto& aux = system.GPU().DSModAux();
    if (!aux.HasUi()) {
        return;
    }

    u32 w = 0, h = 0;
    if (!aux.TakeUi(g_ui_scratch, w, h, g_ui_serial)) {
        return;
    }
    if (w == 0 || h == 0 || w > kDuoBottomMaxW || h > kDuoBottomMaxH ||
        g_ui_scratch.size() != static_cast<std::size_t>(w) * h) {
        return;
    }

    auto* pixels = reinterpret_cast<u32*>(static_cast<std::uint8_t*>(g_map) + kDuoBottomPixelsOff);
    std::memcpy(pixels, g_ui_scratch.data(), g_ui_scratch.size() * sizeof(u32));
    DuoBottomWriteU32(g_map, 0x70, w);
    DuoBottomWriteU32(g_map, 0x74, h);
    DuoBottomWriteU32(g_map, 0x20, 3);
    // seq last — readers never see a half-written frame with a new seq.
    DuoBottomWriteU64(g_map, 0x68, DuoBottomReadU64(g_map, 0x68) + 1);

    aux.width.store(w);
    aux.height.store(h);
    aux.present.store(true);
}

void StopThorBottomBridge() {
    if (g_helper_pid > 0) {
        StopThorBottomHelper(g_helper_pid);
        g_helper_pid = -1;
    }
    if (g_map) {
        munmap(g_map, kDuoBottomMapBytes);
        g_map = nullptr;
    }
    if (g_fd >= 0) {
        close(g_fd);
        g_fd = -1;
    }
    if (!g_path.empty()) {
        unlink(g_path.c_str());
        g_path.clear();
    }
    g_ui_serial = 0;
}

} // namespace DuoQt
