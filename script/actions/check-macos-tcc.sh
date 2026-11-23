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
# On a small share of hosted macOS VMs tccd attributes the job to
# /opt/hca/hosted-compute-agent with an invalid code identity, and none of the
# grants apply: screen capture, posting events and AXIsProcessTrusted() are all
# denied from the first spec on.
#
# To avoid depending on the agent, this script also builds a small wrapper that
# spawns its command with responsibility disclaimed (as Chromium's
# base/process/launch_mac.cc does), so the command becomes its own responsible
# process. Run as `wrapper /bin/bash -c ...`, that is /bin/bash, which the
# "Add TCC permissions on macOS" step grants every service the specs use.
#
# The check runs directly first and prints the TCC diagnostics (also on healthy
# hosts, as a baseline). Then:
#   - when access is granted directly, it stops there and leaves the spec run
#     as it was. It does not try the wrapper: on healthy hosts access is
#     denied under it, and the preflight under it makes tccd record a denied
#     Accessibility row and show an Accessibility prompt;
#   - when access is denied directly, it tries again under the wrapper, and
#     when that is granted, writes MACOS_TCC_DISCLAIM to $GITHUB_ENV so the
#     spec run uses the wrapper too;
#   - when it is denied either way, fails, so the job stops before the specs
#     and can be re-run on another host.

set -uo pipefail

work_dir="${RUNNER_TEMP:-${TMPDIR:-/tmp}}/tcc-preflight"
mkdir -p "$work_dir"
checker="$work_dir/tcc-preflight"
disclaim="$work_dir/tcc-disclaim"

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

cat > "$work_dir/tcc-disclaim.c" <<'EOF'
// Runs argv[1..] with responsibility disclaimed, so that TCC treats the new
// process (not whoever started this one) as the responsible process for it
// and its descendants. Exits with the command's exit status.
#include <errno.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

extern char** environ;

// Private libSystem SPI; see Chromium's base/process/launch_mac.cc.
extern int responsibility_spawnattrs_setdisclaim(posix_spawnattr_t* attrs,
                                                 int disclaim);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s command [args...]\n", argv[0]);
    return 2;
  }
  posix_spawnattr_t attrs;
  int err = posix_spawnattr_init(&attrs);
  if (err == 0)
    err = responsibility_spawnattrs_setdisclaim(&attrs, 1);
  if (err != 0) {
    fprintf(stderr, "tcc-disclaim: setting up spawn attributes: %s\n",
            strerror(err));
    return 2;
  }
  pid_t pid;
  err = posix_spawnp(&pid, argv[1], NULL, &attrs, argv + 1, environ);
  posix_spawnattr_destroy(&attrs);
  if (err != 0) {
    fprintf(stderr, "tcc-disclaim: %s: %s\n", argv[1], strerror(err));
    return 127;
  }
  int status;
  while (waitpid(pid, &status, 0) == -1) {
    if (errno != EINTR) {
      perror("tcc-disclaim: waitpid");
      return 2;
    }
  }
  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  if (WIFSIGNALED(status))
    return 128 + WTERMSIG(status);
  return 1;
}
EOF

if ! clang -framework ApplicationServices -o "$checker" "$work_dir/tcc-preflight.c"; then
  echo "::warning::Could not build the TCC preflight check; skipping it."
  exit 0
fi

echo "Without the disclaim wrapper (responsible process: the runner agent):"
"$checker"
direct_status=$?

disclaimed_status=
if [ "$direct_status" -ne 0 ]; then
  if clang -o "$disclaim" "$work_dir/tcc-disclaim.c"; then
    echo "Under the disclaim wrapper (responsible process: /bin/bash):"
    # Keep /bin/bash running as the checker's parent: `; exit $?` stops bash
    # from exec'ing a lone command in place of itself.
    "$disclaim" /bin/bash -c '"$@"; exit $?' bash "$checker"
    disclaimed_status=$?
  else
    echo "::warning::Could not build the TCC disclaim wrapper; cannot retry under it."
  fi
fi

system_db="/Library/Application Support/com.apple.TCC/TCC.db"
user_db="$HOME/Library/Application Support/com.apple.TCC/TCC.db"
services="'kTCCServiceScreenCapture','kTCCServicePostEvent','kTCCServiceAccessibility','kTCCServiceListenEvent'"
hca=/opt/hca/hosted-compute-agent

if [ "$direct_status" -eq 0 ]; then
  echo "::group::TCC diagnostics (access granted; baseline for comparison)"
else
  echo "::group::TCC diagnostics"
fi
sw_vers
csrutil status || true
echo "Process chain (pid ppid user executable), innermost first:"
pid=$$
while [ -n "$pid" ] && [ "$pid" -gt 1 ]; do
  ps -o pid=,ppid=,user=,comm= -p "$pid"
  pid=$(ps -o ppid= -p "$pid" | tr -d ' ')
done
if [ -e "$hca" ]; then
  echo "Code signature of $hca:"
  codesign -dvvv "$hca" 2>&1 || true
  codesign --verify --verbose=2 "$hca" 2>&1 || true
fi
for db in "$system_db" "$user_db"; do
  echo "Rows in $db:"
  sudo sqlite3 -header "$db" \
    "SELECT service, client, client_type, auth_value, auth_reason, csreq IS NOT NULL AS has_csreq, last_modified FROM access WHERE service IN ($services) ORDER BY service, client;" || true
done
echo "tccd attribution of recent requests:"
sudo log show --last 2m --info --style compact \
  --predicate 'subsystem == "com.apple.TCC" AND eventMessage CONTAINS "ATTRIBUTION"' 2>/dev/null | tail -n 40
echo "::endgroup::"

if [ "$direct_status" -eq 0 ]; then
  echo "TCC access granted; the specs will run as usual."
  exit 0
fi

if [ "$disclaimed_status" = 0 ]; then
  echo "MACOS_TCC_DISCLAIM=$disclaim" >> "${GITHUB_ENV:-/dev/null}"
  echo "TCC access denied directly but granted under the disclaim wrapper; the specs will run under it."
  exit 0
fi

echo "::error::This macOS runner host denies screen capture and/or synthetic input to the job (TCC ScreenCapture/PostEvent), even after granting them and running as its own responsible process. The desktopCapturer, getDisplayMedia, color-sampling and real-input specs would fail on it; that is a runner host problem, not a test failure, so the job stops here. Re-run the job to get another host. See the TCC diagnostics group in this step."
exit 1
