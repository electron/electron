#!/bin/bash
# Clone libuv at the version Electron's Node ships (Node v24.22.1 -> libuv v1.52.1).
# usage: tools/fetch_libuv.sh [dest]   (default: <prototype>/.libuv, gitignored)
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
DEST=${1:-${LIBUV_DIR:-$HERE/.libuv}}
TAG=${LIBUV_TAG:-v1.52.1}
if [ -d "$DEST/.git" ]; then
  echo "$DEST already exists"
else
  git clone -q --depth 1 --branch "$TAG" https://github.com/libuv/libuv "$DEST"
fi
grep -E "define UV_VERSION_(MAJOR|MINOR|PATCH) " "$DEST/include/uv/version.h"
