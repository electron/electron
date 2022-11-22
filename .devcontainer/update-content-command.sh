#!/bin/bash

set -eo pipefail

buildtools=$HOME/.electron_build_tools

export PATH="$PATH:$buildtools/src"

# Sync latest gclient, but only in a prebuild
if [ ! -z "$CODESPACES_PREBUILD_TOKEN" ]; then
  e d gclient sync --with_branch_heads --with_tags
fi
