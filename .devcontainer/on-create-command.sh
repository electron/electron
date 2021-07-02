#!/bin/bash

set -eo pipefail

buildtools=$HOME/.electron_build_tools
rm -rf $buildtools
git clone https://github.com/electron/build-tools.git $buildtools
pushd $buildtools
npx yarn
popd

echo export PATH=\"\$PATH:$buildtools/src\" >> ~/.bashrc

gclient_root=/workspaces/pizza

echo "
solutions = [
  { \"name\"        : \"src/electron\",
    \"url\"         : \"https://github.com/electron/electron\",
    \"deps_file\"   : \"DEPS\",
    \"managed\"     : False,
    \"custom_deps\" : {
    },
    \"custom_vars\": {},
  },
]
" > $gclient_root/.gclient

mkdir -p $buildtools/configs

echo "
{
    \"root\": \"/workspaces/pizza\",
    \"goma\": \"cache-only\",
    \"gen\": {
        \"args\": [
            \"import(\\\"//electron/build/args/testing.gn\\\")\",
            \"import(\\\"/home/samuel/projects/electron/gn-scripts/third_party/goma.gn\\\")\"
        ],
        \"out\": \"Testing\"
    },
    \"env\": {
        \"CHROMIUM_BUILDTOOLS_PATH\": \"/workspaces/pizza/src/buildtools\",
        \"GIT_CACHE_PATH\": \"/workspaces/pizza/.git-cache\"
    },
    \"remotes\": {
        \"electron\": {
            \"origin\": \"https://github.com/electron/electron.git\"
        }
    }
}
" > $buildtools/configs/evm.testing.json
