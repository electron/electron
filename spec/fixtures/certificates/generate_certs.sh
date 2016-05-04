#!/bin/bash
# This script generates certificates that can be used to test SSL client
# authentication.
#
#   1. A (end-entity) -> B -> C (self-signed root)
#   2. D (end-entity) -> B -> C (self-signed root)

try () {
  echo "$@"
  "$@" || exit 1
}

try mkdir out

echo Create the serial number files and indices.
serial=1000
for i in B C
do
  try /bin/sh -c "echo $serial > out/$i-serial"
  serial=$(expr $serial + 1)
  touch out/$i-index.txt
  touch out/$i-index.txt.attr
done

echo Generate the keys.
for i in A B C D
do
  try openssl genrsa -out out/$i.key 2048
done

echo Generate the C CSR
COMMON_NAME="Root CA" \
  CA_DIR=out \
  ID=C \
  try openssl req \
    -new \
    -key out/C.key \
    -out out/C.csr \
    -config certs.cnf

echo C signs itself.
COMMON_NAME="Root CA" \
  CA_DIR=out \
  ID=C \
  try openssl x509 \
    -req -days 3650 \
    -in out/C.csr \
    -extensions ca_cert \
    -extfile certs.cnf \
    -signkey out/C.key \
    -out out/C.pem

echo Generate the intermediates
COMMON_NAME="Intermediate CA" \
  CA_DIR=out \
  ID=B \
  try openssl req \
    -new \
    -key out/B.key \
    -out out/B.csr \
    -config certs.cnf

COMMON_NAME="Root CA" \
  CA_DIR=out \
  ID=C \
  try openssl ca \
    -batch \
    -extensions ca_intermediate_cert \
    -in out/B.csr \
    -out out/B.pem \
    -config certs.cnf

echo Generate the leaf certs
COMMON_NAME="Client Cert" \
  ID=A \
  try openssl req \
    -new \
    -key out/A.key \
    -out out/A.csr \
    -config certs.cnf

echo B signs A
COMMON_NAME="Intermediate CA" \
  CA_DIR=out \
  ID=B \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -in out/A.csr \
    -out out/A.pem \
    -config certs.cnf

COMMON_NAME="localhost" \
  ID=D \
  try openssl req \
    -new \
    -key out/D.key \
    -out out/D.csr \
    -config certs.cnf

echo B signs D
COMMON_NAME="Intermediate CA" \
  CA_DIR=out \
  ID=B \
  try openssl ca \
    -batch \
    -extensions server_cert \
    -in out/D.csr \
    -out out/D.pem \
    -config certs.cnf

echo Package the client cert and private key into PKCS12 file
try /bin/sh -c "cat out/A.pem out/A.key out/B.pem out/C.pem > out/A-chain.pem"

try openssl pkcs12 \
  -in out/A-chain.pem \
  -out client.p12 \
  -export \
  -passout pass:electron

echo Package the certs
try cp out/C.pem rootCA.pem
try cp out/B.pem intermediateCA.pem
try cp out/D.key server.key
try cp out/D.pem server.pem

try rm -rf out
