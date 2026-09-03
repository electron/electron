#!/usr/bin/env bash
# Runs a command and, if it fails, runs it again with exponential backoff.
#
#   bash src/electron/script/actions/retry.sh <command> [args...]
#
# For CI steps that fetch over the network and fail on transient errors (a
# truncated pack from googlesource.com, a GitHub 5xx, a connection reset). Only
# wrap commands that are safe to run twice: a failed `git clone` removes its
# partial directory and a failed `git fetch` leaves the repository as it was.
#
# By default it makes 4 attempts, waiting 1s, 4s and 16s between them. Override
# with RETRY_ATTEMPTS (total attempts), RETRY_DELAY (first wait, in seconds) and
# RETRY_FACTOR (multiplier for each later wait). The exit status is the
# command's own status from the last attempt.

set -uo pipefail

if [ "$#" -eq 0 ]; then
  echo "usage: $0 <command> [args...]" >&2
  exit 2
fi

attempts="${RETRY_ATTEMPTS:-4}"
delay="${RETRY_DELAY:-1}"
factor="${RETRY_FACTOR:-4}"

attempt=1
while true; do
  "$@" && exit 0
  status=$?
  if [ "$attempt" -ge "$attempts" ]; then
    echo "::error::'$*' failed with exit code $status after $attempt attempts"
    exit "$status"
  fi
  echo "::warning::'$*' failed with exit code $status (attempt $attempt of $attempts); retrying in ${delay}s"
  sleep "$delay"
  delay=$((delay * factor))
  attempt=$((attempt + 1))
done
