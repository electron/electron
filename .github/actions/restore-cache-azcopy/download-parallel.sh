#!/bin/bash
# Downloads a single large file with parallel HTTP range requests.
#
# Usage: download-parallel.sh <url> <output-file> [parts]
#
# azcopy always auto-tunes its connection count when the source is an Azure
# Files share (AZCOPY_CONCURRENCY_VALUE is ignored for that case), and the
# tuning rounds keep it running for about a minute after the data has arrived.
# A fixed number of range requests avoids that. 16 connections is below what
# azcopy itself probes (it tries up to 256), so the share sees no more load.
#
# The URL carries a SAS token, so it is never echoed.

set -euo pipefail

URL=$1
OUTPUT=$2
PARTS=${3:-16}

SIZE=$(curl -sSfIL --retry 5 --connect-timeout 30 "$URL" | tr -d '\r' |
  awk 'tolower($1) == "content-length:" { size = $2 } END { print size }')
if ! [[ "$SIZE" =~ ^[0-9]+$ ]] || [ "$SIZE" -eq 0 ]; then
  echo "Could not read the size of the src cache"
  exit 1
fi

PART_DIR=$(mktemp -d "${OUTPUT}.parts.XXXXXX")
trap 'rm -rf "$PART_DIR"' EXIT

CHUNK=$(((SIZE + PARTS - 1) / PARTS))
ARGS=(--parallel --parallel-max "$PARTS")
PART_FILES=()
for ((i = 0; i < PARTS; i++)); do
  START=$((i * CHUNK))
  if [ "$START" -ge "$SIZE" ]; then
    break
  fi
  END=$((START + CHUNK - 1))
  if [ "$END" -ge "$SIZE" ]; then
    END=$((SIZE - 1))
  fi
  PART_FILE=$(printf '%s/part.%03d' "$PART_DIR" "$i")
  PART_FILES+=("$PART_FILE")
  if [ "$i" -gt 0 ]; then
    ARGS+=(--next)
  fi
  ARGS+=(-sSfL --retry 5 --retry-all-errors --connect-timeout 30
    -r "$START-$END" -o "$PART_FILE" "$URL")
done

echo "Downloading $SIZE bytes in ${#PART_FILES[@]} parts"
START_TIME=$(date +%s)
curl "${ARGS[@]}"

TOTAL=0
for PART_FILE in "${PART_FILES[@]}"; do
  TOTAL=$((TOTAL + $(wc -c < "$PART_FILE")))
done
if [ "$TOTAL" -ne "$SIZE" ]; then
  echo "Downloaded $TOTAL bytes but expected $SIZE"
  exit 1
fi

: > "$OUTPUT"
for PART_FILE in "${PART_FILES[@]}"; do
  cat "$PART_FILE" >> "$OUTPUT"
  rm -f "$PART_FILE"
done
echo "Downloaded in $(($(date +%s) - START_TIME))s"
