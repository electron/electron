#!/bin/bash
# Downloads a single large file with parallel HTTP range requests.
#
# Usage: download-parallel.sh <url> <output-file> [connections]
#
# azcopy always auto-tunes its connection count when the source is an Azure
# Files share (AZCOPY_CONCURRENCY_VALUE is ignored for that case), and the
# tuning rounds keep it running for about a minute after the data has arrived.
# This fetches the file the way azcopy does once tuned, as 8 MiB ranges over a
# fixed number of connections, which is below the 64 azcopy settles on.
#
# The URL carries a SAS token, so it is never echoed and only passed to curl
# through a config file that only this user can read.

set -euo pipefail

URL=$1
OUTPUT=$2
CONNECTIONS=${3:-32}
CHUNK_BYTES=${CHUNK_BYTES:-8388608}

PART_DIR=$(mktemp -d "${OUTPUT}.parts.XXXXXX")
trap 'rm -rf "$PART_DIR"' EXIT
CONFIG="$PART_DIR/curl.config"
umask 077

# Options that every transfer repeats: curl resets them after each `next`.
COMMON='fail
silent
show-error
location
retry = 5
retry-all-errors
connect-timeout = 30'

printf '%s\nhead\nurl = "%s"\n' "$COMMON" "$URL" > "$CONFIG"
SIZE=$(curl --config "$CONFIG" | tr -d '\r' |
  awk 'tolower($1) == "content-length:" { size = $2 } END { print size }')
if ! [[ "$SIZE" =~ ^[0-9]+$ ]] || [ "$SIZE" -eq 0 ]; then
  echo "Could not read the size of the src cache"
  exit 1
fi

PARTS=$(((SIZE + CHUNK_BYTES - 1) / CHUNK_BYTES))
{
  echo "parallel"
  echo "parallel-max = $CONNECTIONS"
  for ((i = 0; i < PARTS; i++)); do
    START=$((i * CHUNK_BYTES))
    END=$((START + CHUNK_BYTES - 1))
    if [ "$END" -ge "$SIZE" ]; then
      END=$((SIZE - 1))
    fi
    if [ "$i" -gt 0 ]; then
      echo "next"
    fi
    printf '%s\nrange = "%d-%d"\noutput = "%s/part.%06d"\nurl = "%s"\n' \
      "$COMMON" "$START" "$END" "$PART_DIR" "$i" "$URL"
  done
} > "$CONFIG"

echo "Downloading $SIZE bytes as $PARTS ranges over $CONNECTIONS connections"
START_TIME=$(date +%s)
curl --config "$CONFIG"
rm -f "$CONFIG"

TOTAL=0
for ((i = 0; i < PARTS; i++)); do
  TOTAL=$((TOTAL + $(wc -c < "$(printf '%s/part.%06d' "$PART_DIR" "$i")")))
done
if [ "$TOTAL" -ne "$SIZE" ]; then
  echo "Downloaded $TOTAL bytes but expected $SIZE"
  exit 1
fi

: > "$OUTPUT"
for ((i = 0; i < PARTS; i++)); do
  PART_FILE=$(printf '%s/part.%06d' "$PART_DIR" "$i")
  cat "$PART_FILE" >> "$OUTPUT"
  rm -f "$PART_FILE"
done
echo "Downloaded in $(($(date +%s) - START_TIME))s"
