// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_PLATFORM_UTIL_H_
#define ATOM_COMMON_PLATFORM_UTIL_H_

#include <string>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/strings/string16.h"
#endif

class GURL;

namespace platform_util {

typedef base::Callback<void(const std::string&)> OpenExternalCallback;

// Show the given file in a file manager. If possible, select the file.
// Must be called from the UI thread.
bool ShowItemInFolder(const base::FilePath& full_path);

// Open the given file in the desktop's default manner.
// Must be called from the UI thread.
bool OpenItem(const base::FilePath& full_path);

struct OpenExternalOptions {
  bool activate = true;
  base::FilePath working_dir;
};

// Open the given external protocol URL in the desktop's default manner.
// (For example, mailto: URLs in the default mail user agent.)
bool OpenExternal(
#if defined(OS_WIN)
    const base::string16& url,
#else
    const GURL& url,
#endif
    const OpenExternalOptions& options);

// The asynchronous version of OpenExternal.
void OpenExternal(
#if defined(OS_WIN)
    const base::string16& url,
#else
    const GURL& url,
#endif
    const OpenExternalOptions& options,
    const OpenExternalCallback& callback);

// Move a file to trash.
bool MoveItemToTrash(const base::FilePath& full_path);

void Beep();

#if defined(OS_MACOSX)
bool GetLoginItemEnabled();
bool SetLoginItemEnabled(bool enabled);
#endif

#if defined(OS_LINUX)
// Returns a success flag.
// Unlike libgtkui, does *not* use "chromium-browser.desktop" as a fallback.
bool GetDesktopName(std::string* setme);
#endif

}  // namespace platform_util

#endif  // ATOM_COMMON_PLATFORM_UTIL_H_
