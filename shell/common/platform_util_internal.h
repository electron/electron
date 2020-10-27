// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_PLATFORM_UTIL_INTERNAL_H_
#define SHELL_COMMON_PLATFORM_UTIL_INTERNAL_H_

#include "shell/common/platform_util.h"

#include <string>

namespace base {
class FilePath;
}

namespace platform_util {
namespace internal {

// Called by platform_util.cc on to invoke platform specific logic to move
// |path| to trash using a suitable handler.
bool PlatformTrashItem(const base::FilePath& path, std::string* error);

}  // namespace internal
}  // namespace platform_util

#endif  // SHELL_COMMON_PLATFORM_UTIL_INTERNAL_H_
