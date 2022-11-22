#!/bin/bash

set -eo pipefail

buildtools=$HOME/.electron_build_tools

export PATH="$PATH:$buildtools/src"

# Sync latest
e d gclient sync --with_branch_heads --with_tags
