// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_PLATFORM_UTIL_H_
#define ATOM_COMMON_PLATFORM_UTIL_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/strings/string16.h"
#endif

class GURL;

namespace base {
class FilePath;
}

namespace platform_util {

// Show the given file in a file manager. If possible, select the file.
// Must be called from the UI thread.
void ShowItemInFolder(const base::FilePath& full_path);

// Open the given file in the desktop's default manner.
// Must be called from the UI thread.
void OpenItem(const base::FilePath& full_path);

// Open the given external protocol URL in the desktop's default manner.
// (For example, mailto: URLs in the default mail user agent.)
bool OpenExternal(
#if defined(OS_WIN)
    const base::string16& url,
#else
    const GURL& url,
#endif
    bool activate);

// Move a file to trash.
bool MoveItemToTrash(const base::FilePath& full_path);

void Beep();

#if defined(OS_MACOSX)
// On 10.7+, back and forward swipe gestures can be triggered using a scroll
// gesture, if enabled in System Preferences. This function returns true if
// the feature is supported and enabled, and false otherwise.
bool IsSwipeTrackingFromScrollEventsEnabled();
#endif

}  // namespace platform_util

#endif  // ATOM_COMMON_PLATFORM_UTIL_H_
