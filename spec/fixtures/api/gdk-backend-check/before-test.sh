#!/bin/sh

set -o errexit
set -o nounset

cleanup () {
  exec "./after-test.sh"
}

trap cleanup HUP INT

# Install the application Groot
cp ./groot /tmp/

# Make sure the applications directory exists.
mkdir -p ~/.local/share/applications

# Register Groot as desktop application
cp ./groot.desktop ~/.local/share/applications

# Update database of desktop entries
update-desktop-database ~/.local/share/applications

# Make sure the packages directory exists.
mkdir -p ~/.local/share/mime/packages

# Associate 'application/x-groot' mime type with '*.groot' files
cp ./groot.xml ~/.local/share/mime/packages/

# update the MIME database
update-mime-database ~/.local/share/mime
