#!/usr/bin/env bash
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script will check out llvm and clang into third_party/llvm and build it.

# Do NOT CHANGE this if you don't know what you're doing -- see
# https://code.google.com/p/chromium/wiki/UpdatingClang
# Reverting problematic clang rolls is safe, though.
CLANG_REVISION=269902

# This is incremented when pushing a new build of Clang at the same revision.
CLANG_SUB_REVISION=1

PACKAGE_VERSION="${CLANG_REVISION}-${CLANG_SUB_REVISION}"

THIS_DIR="$(dirname "${0}")"
LLVM_DIR="${THIS_DIR}/../vendor/llvm"
LLVM_BUILD_DIR="${LLVM_DIR}/../llvm-build/Release+Asserts"
STAMP_FILE="${LLVM_DIR}/../llvm-build/cr_build_revision"

# ${A:-a} returns $A if it's set, a else.
LLVM_REPO_URL=${LLVM_URL:-https://llvm.org/svn/llvm-project}

CDS_URL=https://commondatastorage.googleapis.com/chromium-browser-clang


# Die if any command dies, error on undefined variable expansions.
set -eu


OS="$(uname -s)"

# Check if there's anything to be done, exit early if not.
if [[ -f "${STAMP_FILE}" ]]; then
  PREVIOUSLY_BUILT_REVISON=$(cat "${STAMP_FILE}")
  if [[ "${PREVIOUSLY_BUILT_REVISON}" = "${PACKAGE_VERSION}" ]]; then
    echo "Clang already at ${PACKAGE_VERSION}"
    exit 0
  fi
fi
# To always force a new build if someone interrupts their build half way.
rm -f "${STAMP_FILE}"

# Check if there's a prebuilt binary and if so just fetch that. That's faster,
# and goma relies on having matching binary hashes on client and server too.
CDS_FILE="clang-${PACKAGE_VERSION}.tgz"
CDS_OUT_DIR=$(mktemp -d -t clang_download.XXXXXX)
CDS_OUTPUT="${CDS_OUT_DIR}/${CDS_FILE}"
if [ "${OS}" = "Linux" ]; then
  CDS_FULL_URL="${CDS_URL}/Linux_x64/${CDS_FILE}"
elif [ "${OS}" = "Darwin" ]; then
  CDS_FULL_URL="${CDS_URL}/Mac/${CDS_FILE}"
fi
echo Trying to download prebuilt clang
if which curl > /dev/null; then
  curl -L --fail "${CDS_FULL_URL}" -o "${CDS_OUTPUT}" || \
      rm -rf "${CDS_OUT_DIR}"
elif which wget > /dev/null; then
  wget "${CDS_FULL_URL}" -O "${CDS_OUTPUT}" || rm -rf "${CDS_OUT_DIR}"
else
  echo "Neither curl nor wget found. Please install one of these."
  exit 1
fi
if [ -f "${CDS_OUTPUT}" ]; then
  rm -rf "${LLVM_BUILD_DIR}"
  mkdir -p "${LLVM_BUILD_DIR}"
  tar -xzf "${CDS_OUTPUT}" -C "${LLVM_BUILD_DIR}"
  echo clang "${PACKAGE_VERSION}" unpacked
  echo "${PACKAGE_VERSION}" > "${STAMP_FILE}"
  rm -rf "${CDS_OUT_DIR}"
  exit 0
else
  echo Did not find prebuilt clang "${PACKAGE_VERSION}", building
fi
