# Certificate Object

* `data` String - PEM encoded data
* `issuer` [CertificatePrincipal](structures/certificate-principal.md) - Issuer principal
* `issuerName` String - Issuer's Common Name
* `issuerCert` Certificate - Issuer certificate (if not self-signed)
* `subject` [CertificatePrincipal](structures/certificate-principal.md) - Subject principal
* `subjectName` String - Subject's Common Name
* `serialNumber` String - Hex value represented string
* `validStart` Number - Start date of the certificate being valid in seconds
* `validExpiry` Number - End date of the certificate being valid in seconds
* `fingerprint` String - Fingerprint of the certificate
