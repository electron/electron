// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_ELECTRON_CONSTANTS_H_
#define ELECTRON_SHELL_COMMON_ELECTRON_CONSTANTS_H_

#include "base/files/file_path.h"
#include "build/build_config.h"
#include "electron/buildflags/buildflags.h"

namespace electron {

// The app-command in NativeWindow.
extern const char kBrowserForward[];
extern const char kBrowserBackward[];

// Strings describing Chrome security policy for DevTools security panel.
extern const char kSHA1Certificate[];
extern const char kSHA1MajorDescription[];
extern const char kSHA1MinorDescription[];
extern const char kCertificateError[];
extern const char kValidCertificate[];
extern const char kValidCertificateDescription[];
extern const char kSecureProtocol[];
extern const char kSecureProtocolDescription[];

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
extern const char kRunAsNode[];
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
// The MIME type used for the PDF plugin.
extern const char kPdfPluginMimeType[];
extern const base::FilePath::CharType kPdfPluginPath[];
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_ELECTRON_CONSTANTS_H_
