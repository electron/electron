#!/bin/sh

set -eo pipefail

dir="$(dirname $0)"/.working
KEYCHAIN="electron-codesign.keychain-db"
KEYCHAIN_TEMP="$(openssl rand -hex 12)"

cleanup() {
    rm -rf "$dir"
}

# trap cleanup EXIT

# Clean Up
cleanup

# Create Working Dir
mkdir -p "$dir"

# Generate Certs
openssl req -new -newkey rsa:2048 -x509 -days 7300 -nodes -config "$(dirname $0)"/codesign.cnf -extensions extended -batch -out "$dir"/certificate.cer -keyout "$dir"/certificate.key

# macOS 15+ blocks modifications to the system keychain via SIP/TCC,
# so we use a custom user-scoped keychain instead.
# Refs https://github.com/electron/electron/issues/48182
security create-keychain -p "$KEYCHAIN_TEMP" "$KEYCHAIN"
security set-keychain-settings -t 3600 -u "$KEYCHAIN"
security unlock-keychain -p "$KEYCHAIN_TEMP" "$KEYCHAIN"

security list-keychains -d user -s "$KEYCHAIN" $(security list-keychains -d user | tr -d '"')
security import "$dir"/certificate.cer -k "$KEYCHAIN" -T /usr/bin/codesign
security import "$dir"/certificate.key -k "$KEYCHAIN" -T /usr/bin/codesign -A

security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$KEYCHAIN_TEMP" "$KEYCHAIN"
