# Duo companion reader modules (linux-aarch64)

These `.so` files are the Thor-built reader modules for Eden Duo `.dsmod.zip` companions.
They are installed next to each companion by `duo-companion.py` when a package has no
`linux-aarch64` library of its own.

Source: Claude's Fritchii Thor AppImage (Drive upload, Nov 2026).
Do not commit the `.so` binaries into git here — they are large and ABI-tied to the
emulator build. Rebuild them with the Duo modules tree when updating Fritchii.

When packaging a new AppImage, copy this directory to `share/duo-modules/` inside the AppDir.
