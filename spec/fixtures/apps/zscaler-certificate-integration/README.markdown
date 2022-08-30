# Test to verify the integration of Zscaler certificates with node and electron

This test audits the integration of Zscaler certificates read from the windows system store with electron node. To do so, we are starting a server with a certificate that is signed by the self signed root certificate and running a client that tries to communicate with the server.

The client application then reads the certificates from the root Zscaler certificate from the windows system store to verify the SSL handshake with the server.

To obtain a certificates involved in this test we can run the following commands:


To obtain a root ca certificate
---------------------------------------------------------

**Generate a private key**

```
openssl genrsa -out CA.key -des3 2048

```

**Generate a root CA certificate using the key generated above**

```
openssl req -x509 -sha256 -new -nodes -days 3650 -key CA.key -out CA.pem

```

To obtain a certificate signed by root ca certificate generated above
---------------------------------------------------------------------

**Create a file localhost.ext and paste the following content**

```
authorityKeyIdentifier = keyid,issuer
basicConstraints = CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
IP.1 = 127.0.0.1

```

**Generate a key using the command below**

```
openssl genrsa -out localhost.key -des3 2048
openssl rsa -in localhost.key -out localhost.decrypted.key

```

**Use the above generated key to generate a CSR (Certificate Signing Request)**

```
openssl req -new -key localhost.key -out localhost.csr

```

**_Note:_** We need to answer the `Common Name` question with `Zscaler`, in order to read the certs from the windows system store.


**Now with the above CSR, we can request the root CA generated to sign a certificate as below**

```
openssl x509 -req -in localhost.csr -CA CA.pem -CAkey CA.key -CAcreateserial -days 3650 -sha256 -extfile localhost.ext -out localhost.crt

```

Above command takes in the CSR (localhost.csr), the CA certificate (CA.pem and CA.key), and the certificate extensions file (localhost.ext). Those inputs generate a localhost.crt certificate file, valid for ten years.

We can use localhost.decrypted.key and localhost.crt to start a server
