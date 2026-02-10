#!/bin/sh

# Removes the codesigning keychain created by generate-identity.sh.
# Safe to run even if generate-identity.sh was never run (each step
# is guarded).

set -eo pipefail

KEYCHAIN="electron-codesign.keychain-db"

# delete-keychain also removes it from the search list
if security list-keychains -d user | grep -q "$KEYCHAIN"; then
  security delete-keychain "$KEYCHAIN"
  echo "Deleted keychain: $KEYCHAIN"
else
  echo "Keychain not found, nothing to delete"
fi

# Clean up working directory
rm -rf "$(dirname $0)"/.working
echo "Cleanup complete"
