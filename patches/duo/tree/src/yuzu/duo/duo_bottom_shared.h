// SPDX-FileCopyrightText: Copyright 2026 Claude / Christian Ball (Thor Fritchii build)
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Shared companion framebuffer between the main Fritchii process (top gamescope
// DISPLAY) and the --duo-bottom-helper child (bottom Xwayland from
// /armada-bottom-env). Reconstructed from Claude's working Thor AppImage.

#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>

namespace DuoQt {

inline constexpr std::uint32_t kDuoBottomMagic = 0x424F5544u; // 'DUOB' LE
inline constexpr std::uint32_t kDuoBottomVersion = 1;
inline constexpr std::uint32_t kDuoBottomFormatRGBA8 = 0;

#pragma pack(push, 1)
struct DuoBottomHeader {
    std::uint32_t magic{kDuoBottomMagic};
    std::uint32_t version{kDuoBottomVersion};
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t stride_bytes{0}; // width * 4
    std::uint32_t format{kDuoBottomFormatRGBA8};
    // Writers bump seq after pixels are fully written; readers present on change.
    std::uint64_t seq{0};
    std::uint32_t ready{0}; // 1 once header+pixels are valid
    std::uint32_t pad{0};
};
#pragma pack(pop)

inline constexpr std::size_t DuoBottomPixelsOffset() {
    return sizeof(DuoBottomHeader);
}

inline std::size_t DuoBottomMapBytes(std::uint32_t width, std::uint32_t height) {
    return DuoBottomPixelsOffset() + static_cast<std::size_t>(width) * height * 4u;
}

inline bool DuoBottomHeaderOk(const DuoBottomHeader& h) {
    return h.magic == kDuoBottomMagic && h.version == kDuoBottomVersion && h.ready != 0 &&
           h.width > 0 && h.height > 0 && h.stride_bytes >= h.width * 4u;
}

} // namespace DuoQt
