#!/bin/sh

set -e

cd "${BUILT_PRODUCTS_DIR}/${1}.framework"
shift

while [ ! -z "${1}" ]; do
  ln -sf Versions/Current/"${1}" "${1}"
  shift
done
