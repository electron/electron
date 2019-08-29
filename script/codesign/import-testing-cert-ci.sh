#!/bin/sh

KEY_CHAIN=mac-build.keychain
KEYCHAIN_PASSWORD=unsafe_keychain_pass
security create-keychain -p $KEYCHAIN_PASSWORD $KEY_CHAIN
# Make the keychain the default so identities are found
security default-keychain -s $KEY_CHAIN
# Unlock the keychain
security unlock-keychain -p $KEYCHAIN_PASSWORD $KEY_CHAIN

# Add certificates to keychain and allow codesign to access them
security import "$(dirname $0)"/signing.cer -k $KEY_CHAIN -A /usr/bin/codesign
security import "$(dirname $0)"/signing.pem -k $KEY_CHAIN -A /usr/bin/codesign
security import "$(dirname $0)"/signing.p12 -k $KEY_CHAIN -P $SPEC_KEY_PASSWORD -A /usr/bin/codesign

echo "Add keychain to keychain-list"
security list-keychains -s $KEY_CHAIN

echo "Setting key partition list"
security set-key-partition-list -S apple-tool:,apple: -s -k $KEYCHAIN_PASSWORD $KEY_CHAIN

echo "Trusting self-signed certificate"
sudo security trust-settings-import -d "$(dirname $0)"/trust-settings.plist
