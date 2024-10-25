// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_ELECTRON_CONSTANTS_H_
#define ELECTRON_SHELL_COMMON_ELECTRON_CONSTANTS_H_

#include <string_view>

#include "base/files/file_path.h"
#include "base/strings/cstring_view.h"
#include "electron/buildflags/buildflags.h"

namespace electron {

// The app-command in NativeWindow.
inline constexpr std::string_view kBrowserForward = "browser-forward";
inline constexpr std::string_view kBrowserBackward = "browser-backward";

// Keys for Device APIs
inline constexpr std::string_view kDeviceVendorIdKey = "vendorId";
inline constexpr std::string_view kDeviceProductIdKey = "productId";
inline constexpr std::string_view kDeviceSerialNumberKey = "serialNumber";

inline constexpr base::cstring_view kRunAsNode = "ELECTRON_RUN_AS_NODE";

#if BUILDFLAG(ENABLE_PDF_VIEWER)
extern const char kPDFExtensionPluginName[];
extern const char kPDFInternalPluginName[];
extern const base::FilePath::CharType kPdfPluginPath[];
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_ELECTRON_CONSTANTS_H_
