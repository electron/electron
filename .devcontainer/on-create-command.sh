#!/bin/bash

set -eo pipefail

buildtools=$HOME/.electron_build_tools
rm -rf $buildtools
git clone https://github.com/electron/build-tools.git $buildtools
pushd $buildtools
npx yarn
popd

echo export PATH=\"\$PATH:$buildtools/src\" >> ~/.bashrc

/workspaces/electron/.git
/workspaces/src/electron/.git
/workspaces/src/chrome/.git
/workspaces/src/.git