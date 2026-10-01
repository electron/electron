# Certificate Object

* `data` string - PEM encoded data
* `issuer` [CertificatePrincipal](certificate-principal.md) - Issuer principal
* `issuerName` string - Issuer's Common Name
* `issuerCert` Certificate - Issuer certificate, if supplied and parseable.
  The chain stops at the first issuer certificate that cannot be parsed.
* `subject` [CertificatePrincipal](certificate-principal.md) - Subject principal
* `subjectName` string - Subject's Common Name
* `serialNumber` string - Hex value represented string
* `validStart` number - Start date of the certificate being valid in seconds
* `validExpiry` number - End date of the certificate being valid in seconds
* `fingerprint` string - Fingerprint of the certificate
