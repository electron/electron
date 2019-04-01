#!/usr/bin/env bash

set -e

valid_certs=$(security find-identity -p codesigning -v)
if [[ $valid_certs == *"1)"* ]]; then
  first_valid_cert=$(echo $valid_certs | sed 's/ \".*//' | sed 's/.* //')
  echo $first_valid_cert
  exit 0
else
  # No Certificate
  exit 0
fi
