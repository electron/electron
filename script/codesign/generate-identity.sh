#!/bin/sh

set -eo pipefail

dir="$(dirname $0)"/.working

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
sudo security add-trusted-cert -d -r trustRoot -k /Library/Keychains/System.keychain "$dir"/certificate.cer
sudo security import "$dir"/certificate.key -A -k /Library/Keychains/System.keychain

# restart(reload) taskgated daemon
sudo pkill -f /usr/libexec/taskgated

# need once
sudo security authorizationdb write system.privilege.taskport allow
# need once
DevToolsSecurity -enable

# openssl req -newkey rsa:2048 -nodes -keyout "$dir"/private.pem -x509 -days 1 -out "$dir"/certificate.pem -extensions extended -config "$(dirname $0)"/codesign.cnf
# openssl x509 -inform PEM -in "$dir"/certificate.pem -outform DER -out "$dir"/certificate.cer
# openssl x509 -pubkey -noout -in "$dir"/certificate.pem > "$dir"/public.key
# rm -f "$dir"/certificate.pem

# Import Certs
# security import "$dir"/certificate.cer -k $KEY_CHAIN
# security import "$dir"/private.pem -k $KEY_CHAIN
# security import "$dir"/public.key -k $KEY_CHAIN

# Generate Trust Settings
npm_config_yes=true npx ts-node "$(dirname $0)"/gen-trust.ts "$dir"/certificate.cer "$dir"/trust.xml

# Import Trust Settings
sudo security trust-settings-import -d "$dir/trust.xml"
