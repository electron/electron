#!/bin/sh

set -eo pipefail

dir="$(dirname $0)"/.working

cleanup() {
    rm -rf "$dir"
}

trap cleanup EXIT

# Clean Up
cleanup

# Create Working Dir
mkdir -p "$dir"

# Generate Certs
openssl req -newkey rsa:2048 -nodes -keyout "$dir"/private.pem -x509 -days 1 -out "$dir"/certificate.pem -extensions extended -config "$(dirname $0)"/codesign.cnf
openssl x509 -inform PEM -in "$dir"/certificate.pem -outform DER -out "$dir"/certificate.cer
rm -f "$dir"/certificate.pem

# Import Certs
security import "$dir"/certificate.cer -k $KEY_CHAIN
security import "$dir"/private.pem -k $KEY_CHAIN

# Generate Trust Settings
node "$(dirname $0)"/gen-trust.js "$dir"/certificate.cer "$dir"/trust.xml

# Import Trust Settings
sudo security trust-settings-import -d "$dir/trust.xml"
