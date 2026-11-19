# SPDX-FileCopyrightText: Copyright 2026 Claude / Christian Ball (Thor Fritchii build)
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Include from src/yuzu/CMakeLists.txt:
#   include(duo/duo_qt.cmake) # Thor: Duo companion menus + bottom helper

target_sources(yuzu PRIVATE
    duo/duo_companion_menus.cpp
    duo/duo_companion_menus.h
    duo/duo_bottom_shared.h
    duo/duo_bottom_helper.cpp
    duo/duo_bottom_helper.h
    duo/duo_bottom_bridge.cpp
    duo/duo_bottom_bridge.h
    duo/duo_aux_window.cpp
    duo/duo_aux_window.h
)
