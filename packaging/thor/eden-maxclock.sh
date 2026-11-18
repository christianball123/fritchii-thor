#!/bin/sh
# Runs as root. Puts CPU + GPU at max clocks while Eden (PID $1) runs,
# then restores the previous settings when Eden exits.
PID="$1"
[ "$PID" = "--check" ] && exit 0
case "$PID" in ''|*[!0-9]*) echo "usage: eden-maxclock PID" >&2; exit 1 ;; esac
mkdir -p /run/eden-maxclock && chmod 700 /run/eden-maxclock
STATE="/run/eden-maxclock/$PID"
: > "$STATE" || exit 1

save_set() { # file newvalue
	[ -w "$1" ] || return 0
	printf '%s\t%s\n' "$1" "$(cat "$1" 2>/dev/null)" >> "$STATE"
	echo "$2" > "$1" 2>/dev/null || :
}

# CPU: every core cluster -> performance governor, min freq = max freq
for p in /sys/devices/system/cpu/cpufreq/policy*; do
	[ -d "$p" ] || continue
	save_set "$p/scaling_governor" performance
	[ -r "$p/cpuinfo_max_freq" ] && save_set "$p/scaling_min_freq" "$(cat "$p/cpuinfo_max_freq")"
done

# GPU (Adreno, msm devfreq) -> performance, min freq = max freq
for g in /sys/class/devfreq/*gpu* /sys/class/devfreq/*kgsl*; do
	[ -d "$g" ] || continue
	save_set "$g/governor" performance
	[ -r "$g/max_freq" ] && save_set "$g/min_freq" "$(cat "$g/max_freq")"
done
# older kgsl-style driver, if present
for k in /sys/class/kgsl/kgsl-3d0; do
	[ -d "$k" ] || continue
	save_set "$k/force_clk_on" 1
	save_set "$k/force_bus_on" 1
	save_set "$k/force_rail_on" 1
	save_set "$k/idle_timer" 10000000
done

# Wait for Eden to close, then restore everything (in reverse order)
while kill -0 "$PID" 2>/dev/null; do sleep 3; done
tac "$STATE" 2>/dev/null | while IFS="$(printf '\t')" read -r f v; do
	[ -n "$f" ] && echo "$v" > "$f" 2>/dev/null || :
done
rm -f "$STATE"
