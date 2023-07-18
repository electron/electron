#!/bin/bash

set -eo pipefail

buildtools=$HOME/.electron_build_tools
gclient_root=/workspaces/gclient
buildtools_configs=/workspaces/buildtools-configs

export PATH="$PATH:$buildtools/src"

# Create the persisted buildtools config folder
mkdir -p $buildtools_configs
mkdir -p $gclient_root/.git-cache
rm -f $buildtools/configs
ln -s $buildtools_configs $buildtools/configs

# Write the gclient config if it does not already exist
if [ ! -f $gclient_root/.gclient ]; then
  echo "Creating gclient config"

  echo "solutions = [
      { \"name\"        : \"src/electron\",
          \"url\"         : \"https://github.com/electron/electron\",
          \"deps_file\"   : \"DEPS\",
          \"managed\"     : False,
          \"custom_deps\" : {
          },
          \"custom_vars\": {},
      },
    ]
  " >$gclient_root/.gclient
fi

# Write the default buildtools config file if it does
# not already exist
if [ ! -f $buildtools/configs/evm.testing.json ]; then
  echo "Creating build-tools testing config"

  write_config() {
    echo "
        {
            \"goma\": \"$1\",
            \"root\": \"/workspaces/gclient\",
            \"remotes\": {
                \"electron\": {
                    \"origin\": \"https://github.com/electron/electron.git\"
                }
            },
            \"gen\": {
                \"args\": [
                    \"import(\\\"//electron/build/args/testing.gn\\\")\",
                    \"import(\\\"/home/builduser/.electron_build_tools/third_party/goma.gn\\\")\"
                ],
                \"out\": \"Testing\"
            },
            \"env\": {
                \"CHROMIUM_BUILDTOOLS_PATH\": \"/workspaces/gclient/src/buildtools\",
                \"GIT_CACHE_PATH\": \"/workspaces/gclient/.git-cache\"
            },
            \"\$schema\": \"file:///home/builduser/.electron_build_tools/evm-config.schema.json\"
        }
    " >$buildtools/configs/evm.testing.json
  }

  # Start out as cache only
  write_config cache-only

  e use testing

  # Attempt to auth to the goma service via codespaces tokens
  # if it works we can use the goma cluster
  export NOTGOMA_CODESPACES_TOKEN=$GITHUB_TOKEN
  if e d goma_auth login; then
    echo "$GITHUB_USER has GOMA access - switching to cluster mode"
    write_config cluster
  fi
else
  echo "build-tools testing config already exists"

  # Re-auth with the goma cluster regardless.
  NOTGOMA_CODESPACES_TOKEN=$GITHUB_TOKEN e d goma_auth login || true
fi
