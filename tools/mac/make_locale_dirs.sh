#!/bin/bash

# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# usage: make_locale_dirs.sh locale_dir [...]
#
# This script creates the Resources directory for the bundle being built by
# the Xcode target that calls it if the directory does not yet exist. It then
# changes to that directory and creates subdirectories for each locale_dir
# passed on the command line.
#
# This script is intended to create empty locale directories (.lproj) in a
# Cocoa .app bundle. The presence of these empty directories is sufficient to
# convince Cocoa that the application supports the named localization, even if
# an InfoPlist.strings file is not provided. Chrome uses these empty locale
# directoires for its helper executable bundles, which do not otherwise
# require any direct Cocoa locale support.

set -eu

if [[ ${#} -eq 0 ]]; then
  echo "usage: ${0} locale_dir [...]" >& 2
  exit 1
fi

RESOURCES_DIR="${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}"
if [[ ! -d "${RESOURCES_DIR}" ]]; then
  mkdir "${RESOURCES_DIR}"
fi

cd "${RESOURCES_DIR}"

for dir in "${@}"; do
  if [[ ! -d "${dir}" ]]; then
    mkdir "${dir}"
  fi
done
