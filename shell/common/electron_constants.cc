// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/electron_constants.h"

namespace electron {

const char kBrowserForward[] = "browser-forward";
const char kBrowserBackward[] = "browser-backward";

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

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
const char kRunAsNode[] = "ELECTRON_RUN_AS_NODE";
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
const char kPDFExtensionPluginName[] = "Chromium PDF Viewer";
const char kPDFInternalPluginName[] = "Chromium PDF Plugin";
const base::FilePath::CharType kPdfPluginPath[] =
    FILE_PATH_LITERAL("internal-pdf-viewer");
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

}  // namespace electron
