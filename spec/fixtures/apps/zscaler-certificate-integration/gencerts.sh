#!/bin/sh
set -o errexit
set -o nounset

mkdir -p certificates

# Generate root CA
openssl genrsa -out certificates/CA.key 2048

openssl req -x509 -sha256 -new -nodes -days 3650 -key certificates/CA.key -out certificates/CA.pem -subj "/C=IN/ST=Rajasthan/L=Jaipur/O=Global Security/OU=IT Department/CN=Zscaler"

# Generate server certificates
touch certificates/localhost.ext

cat << EOF > certificates/localhost.ext
authorityKeyIdentifier = keyid,issuer
basicConstraints = CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
IP.1 = 127.0.0.1
EOF

openssl genrsa -out certificates/localhost.key 2048

openssl rsa -in certificates/localhost.key \
        -out certificates/localhost.decrypted.key

openssl req -new -key certificates/localhost.key \
      -out certificates/localhost.csr \
      -subj "/C=IN/ST=Rajasthan/L=Jaipur/O=Global Security/OU=IT Department/CN=Zscaler"

openssl x509 -req -in certificates/localhost.csr \
      -CA certificates/CA.pem \
      -CAkey certificates/CA.key \
      -CAcreateserial -days 3650 \
      -sha256 \
      -extfile certificates/localhost.ext \
      -out certificates/localhost.crt

