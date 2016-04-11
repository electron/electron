// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_ATOM_CONSTANTS_H_
#define ATOM_COMMON_ATOM_CONSTANTS_H_

namespace atom {

// Header to ignore CORS.
extern const char kCORSHeader[];

// Strings describing Chrome security policy for DevTools security panel.
extern const char kSHA1Certificate[];
extern const char kSHA1MajorDescription[];
extern const char kSHA1MinorDescription[];
extern const char kCertificateError[];
extern const char kValidCertificate[];
extern const char kValidCertificateDescription[];
extern const char kSecureProtocol[];
extern const char kSecureProtocolDescription[];

}  // namespace atom

#endif  // ATOM_COMMON_ATOM_CONSTANTS_H_
