// SPDX-FileCopyrightText: Copyright 2026 Claude / Christian Ball (Thor Fritchii build)
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Recreated from Claude's Thor AppImage strings (OnMenuInstallDuoCompanion,
// OnMenuRemoveDuoCompanion, OnGameListDumpDuoFiles). Stock eden-duo has no Qt
// frontend for this; Android owns install UI. The AppImage also ships
// packaging/thor/duo-companion.py for CLI install.

#pragma once

#include <QString>
#include <QStringList>
#include <QWidget>

#include "common/common_types.h"

namespace DuoQt {

/// Resolve APPDIR/bin/duo-companion.py or a sibling next to the executable.
[[nodiscard]] QString CompanionToolPath();

/// Install one or more .dsmod.zip packages via duo-companion.py.
bool InstallCompanions(QWidget* parent, const QStringList& paths);

/// Prompt for a title id / installed companion and remove it.
bool RemoveCompanionInteractive(QWidget* parent);

/// List installed companions (title id + name) for a message box / log.
QString ListCompanionsText();

/// Dump dual-screen package files for the selected game into a folder the user
/// can send for companion development (OnGameListDumpDuoFiles).
bool DumpCompanionFiles(QWidget* parent, u64 title_id, const QString& title_name);

} // namespace DuoQt
