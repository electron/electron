#!/bin/bash
set -euo pipefail

export XDG_SESSION_TYPE=wayland
# On a Wayland desktop, the tests will use your active display and compositor.
# To run headlessly in weston like in CI, set WAYLAND_DISPLAY=wayland-99.
export WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-99}"

# Chromium's bundled Weston (//electron:electron_weston_zip, unzipped here by
# CI) comes with the ui-controls module, which lets the mouse-input spec addon
# inject pointer input as a Wayland client. Without it the distro weston is
# used and real input specs cannot run.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WESTON_DIR="${WESTON_DIR:-$SCRIPT_DIR/../../../out/weston}"

if [[ -z "${XDG_RUNTIME_DIR:-}" ]]; then
	XDG_RUNTIME_DIR="$(mktemp -d)"
	chmod 700 "$XDG_RUNTIME_DIR"
	export XDG_RUNTIME_DIR
	trap 'kill "$WESTON_PID" >/dev/null 2>&1 || true; rm -rf "$XDG_RUNTIME_DIR"' EXIT
else
	trap 'kill "$WESTON_PID" >/dev/null 2>&1 || true' EXIT
fi

# Matches what Chromium's testing/xvfb.py does for its own Wayland tests.
#
# panel-position=none: no panel, so the work area is the whole output.
# desktop-shell places a new window at a random spot inside the work area
# unless the window fills it, so the input specs make their windows exactly
# output sized to know where they are.
#
# --width/--height: a fixed output size, which the specs read back with
# screen.getPrimaryDisplay().
WESTON_INI="$XDG_RUNTIME_DIR/weston.ini"
printf '[shell]\npanel-position=none\n' >"$WESTON_INI"

if [[ -x "$WESTON_DIR/weston" ]]; then
	echo "Using bundled weston with ui-controls from $WESTON_DIR"
	# Bundled weston loads its modules and helper clients relative to the
	# working directory (MODULEDIR and LIBEXECDIR are "." in its build).
	(
		cd "$WESTON_DIR"
		exec ./weston \
			--backend=headless-backend.so \
			--modules=ui-controls.so \
			--socket="$WAYLAND_DISPLAY" \
			--idle-time=0 \
			--width=1280 \
			--height=800 \
			--config="$WESTON_INI"
	) >/tmp/weston-headless.log 2>&1 &
else
	echo "No bundled weston in $WESTON_DIR, using $(command -v weston || echo 'weston from PATH')"
	weston \
		--backend=headless-backend.so \
		--socket="$WAYLAND_DISPLAY" \
		--idle-time=0 \
		--width=1280 \
		--height=800 \
		--config="$WESTON_INI" \
		>/tmp/weston-headless.log 2>&1 &
fi
WESTON_PID=$!

for _ in {1..100}; do
	if [[ -S "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY" ]]; then
		break
	fi
	sleep 0.1
done

node "$@" --ozone-platform=wayland
