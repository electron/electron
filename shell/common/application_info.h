// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_APPLICATION_INFO_H_
#define SHELL_COMMON_APPLICATION_INFO_H_

#if defined(OS_WIN)
#include "base/strings/string16.h"
#include "shell/browser/win/scoped_hstring.h"
#endif

#include <string>

namespace electron {

void OverrideApplicationName(const std::string& name);
std::string GetOverriddenApplicationName();

void OverrideApplicationVersion(const std::string& version);
std::string GetOverriddenApplicationVersion();

std::string GetApplicationName();
std::string GetApplicationVersion();
// Returns the user agent of Electron.
std::string GetApplicationUserAgent();

#if defined(OS_WIN)
PCWSTR GetRawAppUserModelID();
bool GetAppUserModelID(ScopedHString* app_id);
void SetAppUserModelID(const base::string16& name);
bool IsRunningInDesktopBridge();
#endif

}  // namespace electron

#endif  // SHELL_COMMON_APPLICATION_INFO_H_
