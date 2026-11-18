#!/bin/sh
# One-time setup (runs as root): installs the clock helper where only root can change it,
# and lets USER run just that helper through sudo without a password.
set -e
SRC="$1"; U="$2"
case "$U" in ''|*[!A-Za-z0-9._-]*) echo "bad user name" >&2; exit 1 ;; esac
install -d -m 755 /usr/local/libexec
install -m 755 -o root -g root "$SRC" /usr/local/libexec/eden-maxclock
T=/etc/sudoers.d/.eden-maxclock.tmp
printf '%s ALL=(root) NOPASSWD: /usr/local/libexec/eden-maxclock\n' "$U" > "$T"
chmod 440 "$T"
if visudo -cf "$T" >/dev/null; then mv "$T" /etc/sudoers.d/eden-maxclock; else rm -f "$T"; exit 1; fi
