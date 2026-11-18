# memvault

Christian’s agent memory plus the **Fritchii Thor** AppImage packaging kit for ArmadaOS on the AYN Thor.

## Memory

`memory/` holds Claude’s imported notes. `memory/MEMORY.md` is the index. `AGENTS.md` tells an agent to load that index, the profile, and the preferences before other work.

## Fritchii for the Thor

`packaging/thor/` is the recipe taken from Claude’s custom aarch64 AppImage:

- Eden Duo companion install hooks
- Clock-max (CPU + GPU held at max while the emulator runs)
- Turnip / Game Mode tweaks

See `packaging/thor/BUILD.md` for rebuild steps onto Fritchii **v6.1**.

### Origin clone

Browse: https://cursor.com/codebase/christian-ball/memvault (private)

```bash
curl -fsSL https://downloads.cursor.com/origin/install.sh | sh
origin auth login
origin repo clone christian-ball/memvault
```

If `origin` is not found:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Docs: https://cursor.com/docs/origin/cli
