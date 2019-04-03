#!/bin/sh

export KEY_CHAIN=mac-build.keychain
KEYCHAIN_PASSWORD=unsafe_keychain_pass
security create-keychain -p $KEYCHAIN_PASSWORD $KEY_CHAIN
# Make the keychain the default so identities are found
security default-keychain -s $KEY_CHAIN
# Unlock the keychain
security unlock-keychain -p $KEYCHAIN_PASSWORD $KEY_CHAIN

echo "Add keychain to keychain-list"
security list-keychains -s $KEY_CHAIN

echo "Setting key partition list"
security set-key-partition-list -S apple-tool:,apple: -s -k $KEYCHAIN_PASSWORD $KEY_CHAIN

echo Generating Identity
"$(dirname $0)"/generate-identity.sh
