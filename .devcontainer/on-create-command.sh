#!/bin/bash

set -eo pipefail

e auto-update disable

buildtools=$HOME/.electron_build_tools
gclient_root=/workspaces/gclient
buildtools_configs=/workspaces/buildtools-configs

export PATH="$PATH:$buildtools/src"

# Create the persisted buildtools config folder
mkdir -p $buildtools_configs
mkdir -p $gclient_root/.git-cache
rm -f $buildtools/configs
ln -s $buildtools_configs $buildtools/configs

# Set the git cookie from chromium.googlesource.com.
if [[ -z "$CHROMIUM_GIT_COOKIE" ]]; then
  echo "CHROMIUM_GIT_COOKIE is not set - cannot authenticate."
  exit 1
fi

eval 'set +o history' 2>/dev/null || setopt HIST_IGNORE_SPACE 2>/dev/null
touch ~/.gitcookies
chmod 0600 ~/.gitcookies

git config --global http.cookiefile ~/.gitcookies

tr , \\t <<__END__ >>~/.gitcookies
$CHROMIUM_GIT_COOKIE
__END__
eval 'set -o history' 2>/dev/null || unsetopt HIST_IGNORE_SPACE 2>/dev/null

RESPONSE=$(curl -s -b ~/.gitcookies https://chromium-review.googlesource.com/a/accounts/self)
if [[ $RESPONSE == ")]}'"* ]]; then
  EMAIL=$(echo "$RESPONSE" | tail -c +5 | jq -r '.email // "No email found"')
  echo "Cookie authentication successful - authenticated as: $EMAIL"
else
  echo "Cookie authentication failed - ensure CHROMIUM_GIT_COOKIE is set correctly"
  echo $RESPONSE
  exit 1
fi

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
            \"root\": \"/workspaces/gclient\",
            \"remotes\": {
                \"electron\": {
                    \"origin\": \"https://github.com/electron/electron.git\"
                }
            },
            \"gen\": {
                \"args\": [
                    \"import(\\\"//electron/build/args/testing.gn\\\")\",
                    \"use_remoteexec = true\",
                    \"use_siso=true\"
                ],
                \"out\": \"Testing\"
            },
            \"env\": {
                \"CHROMIUM_BUILDTOOLS_PATH\": \"/workspaces/gclient/src/buildtools\",
                \"GIT_CACHE_PATH\": \"/workspaces/gclient/.git-cache\"
            },
            \"\$schema\": \"file:///home/builduser/.electron_build_tools/evm-config.schema.json\",
            \"configValidationLevel\": \"strict\",
            \"remoteBuild\": \"siso\",
            \"preserveSDK\": 5
        }
    " >$buildtools/configs/evm.testing.json
  }

  write_config

  e use testing 
else
  echo "build-tools testing config already exists"
fi
