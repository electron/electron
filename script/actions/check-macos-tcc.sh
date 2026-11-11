#!/bin/bash
# Checks, before the specs run, that processes started by this job may capture
# the screen and post synthetic input (TCC kTCCServiceScreenCapture and
# kTCCServicePostEvent). The specs need both: desktopCapturer, getDisplayMedia
# and the color-sampling specs capture the screen, and the real-input specs
# post mouse events.
#
# TCC checks these against the *responsible* process of the caller, which for
# everything in a job is the runner agent that started it, not Electron. The
# runner image grants them to the agents it knows about (see
# actions/runner-images images/macos/scripts/build/configure-tccdb-macos.sh).
# On a small share of hosted macOS VMs none of these grants apply to the job:
# screen capture, posting events and AXIsProcessTrusted() are all denied from
# the first spec on.
#
# When access is missing this script prints what is needed to explain why (the
# process chain, the TCC.db rows for these services and tccd's attribution of
# the request) and warns. It never changes TCC state and never fails the step.

set -uo pipefail

work_dir="${RUNNER_TEMP:-${TMPDIR:-/tmp}}/tcc-preflight"
mkdir -p "$work_dir"
checker="$work_dir/tcc-preflight"

cat > "$work_dir/tcc-preflight.c" <<'EOF'
#include <ApplicationServices/ApplicationServices.h>
#include <stdio.h>

int main(void) {
  int screen_capture = CGPreflightScreenCaptureAccess();
  int post_event = CGPreflightPostEventAccess();
  int accessibility = AXIsProcessTrusted();
  printf("screenCaptureAccess=%d postEventAccess=%d accessibilityTrusted=%d\n",
         screen_capture, post_event, accessibility);
  return screen_capture && post_event ? 0 : 1;
}
EOF

if ! clang -framework ApplicationServices -o "$checker" "$work_dir/tcc-preflight.c"; then
  echo "::warning::Could not build the TCC preflight check; skipping it."
  exit 0
fi

if "$checker"; then
  exit 0
fi

system_db="/Library/Application Support/com.apple.TCC/TCC.db"
user_db="$HOME/Library/Application Support/com.apple.TCC/TCC.db"
services="'kTCCServiceScreenCapture','kTCCServicePostEvent','kTCCServiceAccessibility','kTCCServiceListenEvent'"

echo "::group::TCC diagnostics"
sw_vers
csrutil status || true
echo "Process chain (pid ppid user executable), innermost first:"
pid=$$
while [ -n "$pid" ] && [ "$pid" -gt 1 ]; do
  ps -o pid=,ppid=,user=,comm= -p "$pid"
  pid=$(ps -o ppid= -p "$pid" | tr -d ' ')
done
for db in "$system_db" "$user_db"; do
  echo "Rows in $db:"
  sudo sqlite3 -header "$db" \
    "SELECT service, client, client_type, auth_value, auth_reason, csreq IS NOT NULL AS has_csreq, last_modified FROM access WHERE service IN ($services) ORDER BY service, client;" || true
done
echo "tccd attribution of recent requests:"
sudo log show --last 2m --info --style compact \
  --predicate 'subsystem == "com.apple.TCC" AND eventMessage CONTAINS "ATTRIBUTION"' 2>/dev/null | tail -n 40
echo "::endgroup::"

echo "::warning::This macOS runner denies screen capture and/or synthetic input to the job (TCC ScreenCapture/PostEvent). The desktopCapturer, getDisplayMedia, color-sampling and real-input specs will fail on it; that is a runner host problem, not a test failure. See the TCC diagnostics group in the 'Check TCC access on macOS' step."
exit 0
