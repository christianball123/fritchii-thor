// SPDX-FileCopyrightText: Copyright 2026 Claude / Christian Ball (Thor Fritchii build)
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/duo/duo_companion_menus.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>

#include "common/logging.h"

namespace DuoQt {
namespace {

QString EdenDataDir() {
    // Companions live under ~/.local/share/eden/load/ (same as packaging/thor hooks).
    const QString xdg = qEnvironmentVariable("XDG_DATA_HOME");
    if (!xdg.isEmpty()) {
        return xdg + QStringLiteral("/eden");
    }
    return QDir::homePath() + QStringLiteral("/.local/share/eden");
}

QString ModulesDir() {
    // AppImage: $APPDIR/share/duo-modules ; otherwise next to the binary.
    const QString appdir = qEnvironmentVariable("APPDIR");
    if (!appdir.isEmpty()) {
        return appdir + QStringLiteral("/share/duo-modules");
    }
    return QCoreApplication::applicationDirPath() + QStringLiteral("/../share/duo-modules");
}

int RunCompanionTool(const QStringList& args, QString* stdout_text = nullptr,
                     QString* stderr_text = nullptr) {
    const QString tool = CompanionToolPath();
    if (tool.isEmpty() || !QFileInfo::exists(tool)) {
        if (stderr_text) {
            *stderr_text = QStringLiteral("The companion installer is missing from this build.");
        }
        return 127;
    }
    QProcess proc;
    proc.setProgram(QStringLiteral("python3"));
    QStringList full{QStringLiteral("-I"), tool};
    full.append(args);
    proc.setArguments(full);
    proc.start();
    if (!proc.waitForFinished(/*msecs=*/120000)) {
        proc.kill();
        if (stderr_text) {
            *stderr_text = QStringLiteral("Could not run python3, which the companion installer needs.");
        }
        return 124;
    }
    if (stdout_text) {
        *stdout_text = QString::fromLocal8Bit(proc.readAllStandardOutput());
    }
    if (stderr_text) {
        *stderr_text = QString::fromLocal8Bit(proc.readAllStandardError());
    }
    return proc.exitCode();
}

} // namespace

QString CompanionToolPath() {
    const QString appdir = qEnvironmentVariable("APPDIR");
    if (!appdir.isEmpty()) {
        const QString p = appdir + QStringLiteral("/bin/duo-companion.py");
        if (QFileInfo::exists(p)) {
            return p;
        }
    }
    const QString beside = QCoreApplication::applicationDirPath() + QStringLiteral("/duo-companion.py");
    if (QFileInfo::exists(beside)) {
        return beside;
    }
    return {};
}

bool InstallCompanions(QWidget* parent, const QStringList& paths) {
    if (paths.isEmpty()) {
        return false;
    }
    const QString modules = ModulesDir();
    const QString data = EdenDataDir();
    QString out, err;
    QStringList args{QStringLiteral("install"), modules, data};
    args.append(paths);
    const int rc = RunCompanionTool(args, &out, &err);
    if (!out.isEmpty()) {
        LOG_INFO(Frontend, "Duo companion install:\n{}", out.toStdString());
    }
    if (rc != 0) {
        QMessageBox::warning(parent, QObject::tr("Install Duo Companion"),
                             err.isEmpty() ? QObject::tr("Install failed.") : err);
        return false;
    }
    QMessageBox::information(
        parent, QObject::tr("Install Duo Companion"),
        QObject::tr("Games with a companion now start with it on the second screen.\n"
                    "Restart the game to use its companion."));
    return true;
}

bool RemoveCompanionInteractive(QWidget* parent) {
    QString listed;
    RunCompanionTool({QStringLiteral("list"), ModulesDir(), EdenDataDir()}, &listed, nullptr);
    if (listed.trimmed().isEmpty() || listed.contains(QStringLiteral("No companions"))) {
        QMessageBox::information(parent, QObject::tr("Remove Duo Companion"),
                                 QObject::tr("No companions are installed."));
        return false;
    }
    bool ok = false;
    const QString tid = QInputDialog::getText(
        parent, QObject::tr("Remove Duo Companion"),
        QObject::tr("Companion to remove:\n\n%1\nTitle ID (16 hex digits):").arg(listed),
        QLineEdit::Normal, {}, &ok);
    if (!ok || tid.trimmed().isEmpty()) {
        return false;
    }
    QString out, err;
    const int rc = RunCompanionTool(
        {QStringLiteral("remove"), ModulesDir(), EdenDataDir(), tid.trimmed().toUpper()}, &out,
        &err);
    if (rc != 0) {
        QMessageBox::warning(parent, QObject::tr("Remove Duo Companion"),
                             err.isEmpty() ? QObject::tr("Remove failed.") : err);
        return false;
    }
    QMessageBox::information(parent, QObject::tr("Remove Duo Companion"),
                             out.isEmpty() ? QObject::tr("Removed.") : out);
    return true;
}

QString ListCompanionsText() {
    QString out;
    RunCompanionTool({QStringLiteral("list"), ModulesDir(), EdenDataDir()}, &out, nullptr);
    return out.isEmpty() ? QObject::tr("No companions are installed.") : out;
}

bool DumpCompanionFiles(QWidget* parent, u64 title_id, const QString& title_name) {
    const QString dest = QFileDialog::getExistingDirectory(
        parent, QObject::tr("Extract Duo Companion Files"));
    if (dest.isEmpty()) {
        return false;
    }
    // Best-effort: copy any installed package for this title into dest.
    const QString load = EdenDataDir() + QStringLiteral("/load/") +
                         QString::asprintf("%016llX", static_cast<unsigned long long>(title_id));
    QDir src(load);
    if (!src.exists()) {
        QMessageBox::information(
            parent, QObject::tr("Extract Duo Companion Files"),
            QObject::tr("No dual-screen package is installed for this title yet."));
        return false;
    }
    const QString target =
        dest + QChar('/') +
        (title_name.isEmpty() ? QString::asprintf("%016llX", static_cast<unsigned long long>(title_id))
                              : title_name);
    QDir().mkpath(target);
    for (const QFileInfo& fi : src.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString to = target + QChar('/') + fi.fileName();
        QDir(to).removeRecursively();
        // Qt 6.3+: QDir::copyRecursively; fall back to process cp -a on Linux.
        QProcess::execute(QStringLiteral("cp"), {QStringLiteral("-a"), fi.absoluteFilePath(), to});
    }
    QMessageBox::information(
        parent, QObject::tr("Extract Duo Companion Files"),
        QObject::tr("Send the files in that folder (%1) to make a companion for %2, version dump.")
            .arg(target, title_name.isEmpty() ? QString::asprintf("%016llX",
                                                                  static_cast<unsigned long long>(
                                                                      title_id))
                                              : title_name));
    return true;
}

} // namespace DuoQt
