// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_APPLICATION_INFO_H_
#define ATOM_COMMON_APPLICATION_INFO_H_

#if defined(OS_WIN)
#include "atom/browser/win/scoped_hstring.h"
#include "base/strings/string16.h"
#endif

#include <string>

namespace atom {

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

}  // namespace atom

#endif  // ATOM_COMMON_APPLICATION_INFO_H_
