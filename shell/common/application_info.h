// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_APPLICATION_INFO_H_
#define ELECTRON_SHELL_COMMON_APPLICATION_INFO_H_

#if defined(OS_WIN)
#include "shell/browser/win/scoped_hstring.h"
#endif

#include <string>

namespace electron {

std::string& OverriddenApplicationName();
std::string& OverriddenApplicationVersion();

std::string GetPossiblyOverriddenApplicationName();

std::string GetApplicationName();
std::string GetApplicationVersion();
// Returns the user agent of Electron.
std::string GetApplicationUserAgent();

#if defined(OS_WIN)
PCWSTR GetRawAppUserModelID();
bool GetAppUserModelID(ScopedHString* app_id);
void SetAppUserModelID(const std::wstring& name);
bool IsRunningInDesktopBridge();
#endif

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_APPLICATION_INFO_H_
