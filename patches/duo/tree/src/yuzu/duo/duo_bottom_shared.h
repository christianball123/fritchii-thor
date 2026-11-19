// SPDX-FileCopyrightText: Copyright 2026 Claude / Christian Ball (Thor Fritchii build)
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Shared companion framebuffer layout matching Claude's working Thor AppImage
// (thor_bottom.cpp). Magic is 'DOUB' LE; pixels begin at offset 0x7c.

#pragma once

#include <cstddef>
#include <cstdint>

namespace DuoQt {

inline constexpr std::uint32_t kDuoBottomMagic = 0x42554F44u; // 'DOUB' LE
inline constexpr std::uint32_t kDuoBottomVersion = 1;
inline constexpr std::uint32_t kDuoBottomMaxW = 2560;
inline constexpr std::uint32_t kDuoBottomMaxH = 1440;
// Claude: ftruncate to 0xe10080 = 128 + 2560*1440*4 (header region padded).
inline constexpr std::size_t kDuoBottomHeaderBytes = 0x80;
inline constexpr std::size_t kDuoBottomPixelsOff = 0x7c;
inline constexpr std::size_t kDuoBottomMapBytes =
    kDuoBottomHeaderBytes + static_cast<std::size_t>(kDuoBottomMaxW) * kDuoBottomMaxH * 4u;

// Layout (little-endian), reconstructed from Claude's eden binary:
//   0x00 u32 magic (DOUB)
//   0x04 u32 version (1)
//   0x08 u32 parent_pid
//   0x0c u32 helper_pid
//   0x20 u32 present_state (3 while showing)
//   0x68 u64 seq
//   0x70 u32 width
//   0x74 u32 height
//   0x7c … RGBA8 pixels (stride = width*4)

inline void DuoBottomWriteU32(void* base, std::size_t off, std::uint32_t v) {
    *reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(base) + off) = v;
}

inline std::uint32_t DuoBottomReadU32(const void* base, std::size_t off) {
    return *reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(base) + off);
}

inline void DuoBottomWriteU64(void* base, std::size_t off, std::uint64_t v) {
    *reinterpret_cast<std::uint64_t*>(static_cast<std::uint8_t*>(base) + off) = v;
}

inline std::uint64_t DuoBottomReadU64(const void* base, std::size_t off) {
    return *reinterpret_cast<const std::uint64_t*>(static_cast<const std::uint8_t*>(base) + off);
}

inline bool DuoBottomHeaderOk(const void* base) {
    return DuoBottomReadU32(base, 0x00) == kDuoBottomMagic &&
           DuoBottomReadU32(base, 0x04) == kDuoBottomVersion &&
           DuoBottomReadU32(base, 0x70) > 0 && DuoBottomReadU32(base, 0x74) > 0 &&
           DuoBottomReadU32(base, 0x70) <= kDuoBottomMaxW &&
           DuoBottomReadU32(base, 0x74) <= kDuoBottomMaxH;
}

} // namespace DuoQt
