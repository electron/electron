#!/bin/bash

set -eo pipefail

buildtools=$HOME/.electron_build_tools
gclient_root=/workspaces/gclient
buildtools_configs=/workspaces/buildtools-configs

export PATH="$PATH:$buildtools/src"

# Sync latest gclient
e d gclient sync --with_branch_heads --with_tags
