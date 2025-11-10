// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_ELECTRON_CONSTANTS_H_
#define ELECTRON_SHELL_COMMON_ELECTRON_CONSTANTS_H_

#include <string_view>

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

// Per-profile UUID to distinguish global shortcut sessions for
// org.freedesktop.portal.GlobalShortcuts. This is a counterpart to
// extensions::pref_names::kGlobalShortcutsUuid, which may be not defined
// if extensions are disabled.
inline constexpr char kElectronGlobalShortcutsUuid[] =
    "electron.global_shortcuts.uuid";

#if BUILDFLAG(ENABLE_PDF_VIEWER)
inline constexpr std::string_view kPDFExtensionPluginName =
    "Chromium PDF Viewer";
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_ELECTRON_CONSTANTS_H_
