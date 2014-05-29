#!/bin/bash

# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# usage: make_locale_paks locale_dir [...]
#
# This script creates the .pak files under locales directory, it is used to fool
# the ResourcesBundle that the locale is available.

set -eu

if [[ ${#} -eq 0 ]]; then
  echo "usage: ${0} build_dir [locale_pak...]" >& 2
  exit 1
fi

PRODUCT_DIR="$1"
shift

LOCALES_DIR="${PRODUCT_DIR}/locales"
if [[ ! -d "${LOCALES_DIR}" ]]; then
  mkdir "${LOCALES_DIR}"
fi

cd "${LOCALES_DIR}"

for pak in "${@}"; do
  if [[ ! -f "${pak}.pak" ]]; then
    touch "${pak}.pak"
  fi
done
