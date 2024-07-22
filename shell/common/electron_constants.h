// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_ELECTRON_CONSTANTS_H_
#define ELECTRON_SHELL_COMMON_ELECTRON_CONSTANTS_H_

#include "base/files/file_path.h"
#include "electron/buildflags/buildflags.h"

namespace electron {

// The app-command in NativeWindow.
extern const char kBrowserForward[];
extern const char kBrowserBackward[];

// Keys for Device APIs
extern const char kDeviceVendorIdKey[];
extern const char kDeviceProductIdKey[];
extern const char kDeviceSerialNumberKey[];

extern const char kRunAsNode[];

#if BUILDFLAG(ENABLE_PDF_VIEWER)
extern const char kPDFExtensionPluginName[];
extern const char kPDFInternalPluginName[];
extern const base::FilePath::CharType kPdfPluginPath[];
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_ELECTRON_CONSTANTS_H_
