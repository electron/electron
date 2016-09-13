// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/atom_constants.h"

namespace atom {

const char kCORSHeader[] = "Access-Control-Allow-Origin: *";

const char kSHA1Certificate[] = "SHA-1 Certificate";
const char kSHA1MajorDescription[] =
    "The certificate for this site expires in 2017 or later, "
    "and the certificate chain contains a certificate signed using SHA-1.";
const char kSHA1MinorDescription[] =
    "The certificate for this site expires in 2016, "
    "and the certificate chain contains a certificate signed using SHA-1.";
const char kCertificateError[] = "Certificate Error";
const char kValidCertificate[] = "Valid Certificate";
const char kValidCertificateDescription[] =
    "The connection to this site is using a valid, trusted server certificate.";
const char kSecureProtocol[] = "Secure TLS connection";
const char kSecureProtocolDescription[] =
    "The connection to this site is using a strong protocol version "
    "and cipher suite.";

}  // namespace atom
