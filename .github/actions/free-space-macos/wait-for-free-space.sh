#!/bin/bash
# Waits until the data volume has at least $1 GiB free (default 80).
#
# free-space-macos renames the directories it frees into $TMPDIR/del-target-*
# and deletes them in a detached process, so the space comes back gradually.
# If that process is gone and there still is not enough space, delete what is
# left synchronously; if the volume is still short after the timeout, carry on
# and let the step that needs the space report the failure.

set -uo pipefail

REQUIRED_GB=${1:-80}
TIMEOUT_SECONDS=${2:-600}

free_gb() {
  df -g /System/Volumes/Data | awk 'NR==2 {print $4}'
}

start=$(date +%s)
while true; do
  free=$(free_gb)
  if [ "$free" -ge "$REQUIRED_GB" ]; then
    echo "${free}GiB free (need ${REQUIRED_GB}GiB) after $(($(date +%s) - start))s"
    exit 0
  fi

  if ! pgrep -f 'rm -rf .*del-target-' > /dev/null; then
    echo "Background delete is not running and only ${free}GiB is free; deleting synchronously"
    sudo rm -rf "$TMPDIR"/del-target-*
    echo "$(free_gb)GiB free after the synchronous delete"
    exit 0
  fi

  if [ $(($(date +%s) - start)) -ge "$TIMEOUT_SECONDS" ]; then
    echo "::warning::Only ${free}GiB free after ${TIMEOUT_SECONDS}s (wanted ${REQUIRED_GB}GiB); continuing"
    df -h
    exit 0
  fi

  sleep 2
done
