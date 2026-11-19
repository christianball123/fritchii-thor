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
std::size_t g_map_bytes = 0;
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

bool ResizeMap(u32 width, u32 height) {
    const std::size_t bytes = DuoBottomMapBytes(width, height);
    if (g_map && g_map_bytes >= bytes) {
        return true;
    }
    if (g_map) {
        munmap(g_map, g_map_bytes);
        g_map = nullptr;
        g_map_bytes = 0;
    }
    if (g_fd < 0) {
        return false;
    }
    if (ftruncate(g_fd, static_cast<off_t>(bytes)) != 0) {
        LOG_ERROR(Frontend, "Thor bottom: cannot create {}: {}", g_path, std::strerror(errno));
        return false;
    }
    void* map = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, 0);
    if (map == MAP_FAILED) {
        LOG_ERROR(Frontend, "Thor bottom: cannot create {}: {}", g_path, std::strerror(errno));
        return false;
    }
    g_map = map;
    g_map_bytes = bytes;

    auto* header = static_cast<DuoBottomHeader*>(g_map);
    std::memset(header, 0, sizeof(DuoBottomHeader));
    header->magic = kDuoBottomMagic;
    header->version = kDuoBottomVersion;
    header->width = width;
    header->height = height;
    header->stride_bytes = width * 4u;
    header->format = kDuoBottomFormatRGBA8;
    header->seq = 0;
    header->ready = 1;

    LOG_INFO(Frontend, "Thor bottom: helper screen is {}x{}", width, height);
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

    // Default panel size until the first companion UI publish arrives.
    constexpr u32 kDefaultW = 640;
    constexpr u32 kDefaultH = 740;
    if (!ResizeMap(kDefaultW, kDefaultH)) {
        StopThorBottomBridge();
        return;
    }

    // Advertise a physical aux panel so DSMod companions publish UI even without
    // Renderer::SetAuxWindow (Thor uses the SDL helper instead).
    if (system.IsPoweredOn()) {
        auto& aux = system.GPU().DSModAux();
        aux.width.store(kDefaultW);
        aux.height.store(kDefaultH);
        aux.present.store(true);
        aux.panel_is_inline.store(false);
    }

    g_helper_pid = MaybeStartThorBottomHelper(g_path);
    if (g_helper_pid < 0) {
        LOG_ERROR(Frontend, "Thor bottom: runner ended with code {} (output in {})", -1,
                  "/duo-bottom-runner.log");
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
    if (w == 0 || h == 0 || g_ui_scratch.size() != static_cast<std::size_t>(w) * h) {
        return;
    }
    if (!ResizeMap(w, h)) {
        return;
    }

    auto* header = static_cast<DuoBottomHeader*>(g_map);
    auto* pixels = reinterpret_cast<u32*>(static_cast<std::uint8_t*>(g_map) + DuoBottomPixelsOffset());
    std::memcpy(pixels, g_ui_scratch.data(), g_ui_scratch.size() * sizeof(u32));
    header->width = w;
    header->height = h;
    header->stride_bytes = w * 4u;
    header->ready = 1;
    // seq last so readers never see a half-written frame with a new seq.
    header->seq += 1;

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
        munmap(g_map, g_map_bytes);
        g_map = nullptr;
        g_map_bytes = 0;
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
