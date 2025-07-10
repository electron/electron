#!/bin/bash
set -euo pipefail

export XDG_SESSION_TYPE=wayland
export WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-99}"

if [[ -z "${XDG_RUNTIME_DIR:-}" ]]; then
	XDG_RUNTIME_DIR="$(mktemp -d)"
	chmod 700 "$XDG_RUNTIME_DIR"
	export XDG_RUNTIME_DIR
	trap 'kill "$WESTON_PID" >/dev/null 2>&1 || true; rm -rf "$XDG_RUNTIME_DIR"' EXIT
else
	trap 'kill "$WESTON_PID" >/dev/null 2>&1 || true' EXIT
fi

weston \
	--backend=headless-backend.so \
	--socket="$WAYLAND_DISPLAY" \
	--idle-time=0 \
	>/tmp/weston-headless.log 2>&1 &
WESTON_PID=$!

for _ in {1..100}; do
	if [[ -S "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY" ]]; then
		break
	fi
	sleep 0.1
done

node "$@" --ozone-platform=wayland
