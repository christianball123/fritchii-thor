#!/usr/bin/env python3
# Thor build: install Eden Duo companion packages (.dsmod.zip) for Fritchii-Thor.
#
#   duo-companion.py install <modules dir> <fritchii data dir> FILE.dsmod.zip [...]
#   duo-companion.py list    <modules dir> <fritchii data dir>
#   duo-companion.py remove  <modules dir> <fritchii data dir> TITLEID
#
# A package unpacks to <fritchii data>/load/<TITLEID>/<Name>-<version>/ (package.json + dualscreen/),
# e.g. ~/.local/share/fritchii/load/… — same LoadDir Fritchii already uses for mods.
# Published packages carry no Linux ARM64 reader module, so the one built for the Thor
# (<modules dir>/<TITLEID>.so) is added as modules/linux-aarch64/<TITLEID>.so and listed in
# dualscreen/manifest.json with its checksum.

import hashlib
import json
import os
import re
import shutil
import sys
import tempfile
import zipfile

PLATFORM = "linux-aarch64"
TITLE_RE = re.compile(r"^[0-9A-F]{16}$")


def safe_name(text):
    return re.sub(r"[^A-Za-z0-9._-]", "", text.replace(" ", "")) or "Companion"


def companions(load_dir):
    """(title_id, folder path, package.json) for every installed companion."""
    out = []
    if not os.path.isdir(load_dir):
        return out
    for tid in sorted(os.listdir(load_dir)):
        tdir = os.path.join(load_dir, tid)
        if not os.path.isdir(tdir):
            continue
        for folder in sorted(os.listdir(tdir)):
            root = os.path.join(tdir, folder)
            pkg = os.path.join(root, "package.json")
            if os.path.isfile(pkg) and os.path.isfile(os.path.join(root, "dualscreen", "manifest.json")):
                try:
                    with open(pkg, encoding="utf-8") as f:
                        out.append((tid, root, json.load(f)))
                except (OSError, ValueError):
                    pass
    return out


def install(modules_dir, data_dir, path):
    name = os.path.basename(path)
    if not zipfile.is_zipfile(path):
        print(f"{name}: not a .dsmod.zip package")
        return False
    load_dir = os.path.join(data_dir, "load")
    with zipfile.ZipFile(path) as z:
        entries = z.infolist()
        for e in entries:
            parts = e.filename.split("/")
            if e.filename.startswith("/") or ".." in parts or parts[0] not in ("package.json", "dualscreen"):
                print(f"{name}: unexpected file in package: {e.filename}")
                return False
        try:
            pkg = json.loads(z.read("package.json").decode("utf-8"))
            manifest = json.loads(z.read("dualscreen/manifest.json").decode("utf-8"))
        except KeyError as missing:
            print(f"{name}: package is missing {missing}")
            return False
        tid = str(pkg.get("title_id", "")).upper()
        if not TITLE_RE.match(tid):
            print(f"{name}: package.json has no valid title_id")
            return False
        version = str(pkg.get("version", "0"))
        folder = f"{safe_name(str(pkg.get('name') or manifest.get('name') or 'Companion'))}-{version}"

        staging = tempfile.mkdtemp(prefix=".duo-install-", dir=data_dir)
        try:
            z.extractall(staging)
            module = manifest.get("module")
            own = (module or {}).get("libraries", {}).get(PLATFORM)
            own_file = os.path.join(staging, "dualscreen", own["path"]) if own and own.get("path") else ""
            if own_file and os.path.isfile(own_file):
                # The package brings its own Thor module (made for this build): keep it as is
                with open(own_file, "rb") as f:
                    if hashlib.sha256(f.read()).hexdigest().lower() != str(own.get("sha256", "")).lower():
                        print(f"{name}: its {PLATFORM} module is damaged (checksum mismatch)")
                        return False
            elif module is not None:
                built = os.path.join(modules_dir, f"{tid}.so")
                if not os.path.isfile(built):
                    print(f"{name}: needs a reader module, and this build has none for {tid} yet")
                    return False
                rel = f"modules/{PLATFORM}/{tid}.so"
                dest = os.path.join(staging, "dualscreen", rel)
                os.makedirs(os.path.dirname(dest), exist_ok=True)
                shutil.copyfile(built, dest)
                with open(dest, "rb") as f:
                    digest = hashlib.sha256(f.read()).hexdigest()
                module.setdefault("libraries", {})[PLATFORM] = {"path": rel, "sha256": digest}
                with open(os.path.join(staging, "dualscreen", "manifest.json"), "w", encoding="utf-8") as f:
                    json.dump(manifest, f, indent=1)
            # Replace any older install of this game's companion
            for other_tid, root, _ in companions(load_dir):
                if other_tid == tid:
                    shutil.rmtree(root, ignore_errors=True)
            target = os.path.join(load_dir, tid, folder)
            os.makedirs(os.path.dirname(target), exist_ok=True)
            shutil.move(staging, target)
            staging = None
        finally:
            if staging:
                shutil.rmtree(staging, ignore_errors=True)
    print(f"Installed {folder} for {tid}" + (" (with the Thor reader module)" if manifest.get("module") is not None else ""))
    return True


def main(argv):
    if len(argv) < 4:
        print(__doc__ if __doc__ else "usage")
        return 2
    cmd, modules_dir, data_dir = argv[1], argv[2], argv[3]
    load_dir = os.path.join(data_dir, "load")
    if cmd == "install":
        if len(argv) < 5:
            print("Give the .dsmod.zip file(s) to install")
            return 2
        ok = all([install(modules_dir, data_dir, p) for p in argv[4:]])
        return 0 if ok else 1
    if cmd == "list":
        found = companions(load_dir)
        if not found:
            print("No companions installed")
        for tid, root, pkg in found:
            print(f"{tid}  {os.path.basename(root)}")
        return 0
    if cmd == "remove":
        tid = argv[4].upper() if len(argv) > 4 else ""
        gone = [root for t, root, _ in companions(load_dir) if t == tid]
        for root in gone:
            shutil.rmtree(root, ignore_errors=True)
        print(f"Removed {len(gone)} companion(s) for {tid}")
        return 0
    print(f"unknown command {cmd}")
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
