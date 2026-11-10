// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_LIB_LOGIND_UTILS_H_
#define ELECTRON_SHELL_BROWSER_LIB_LOGIND_UTILS_H_

#include "base/command_line.h"
#include "base/files/file_path.h"

namespace electron::logind {

inline constexpr char kServiceName[] = "org.freedesktop.login1";
inline constexpr char kObjectPath[] = "/org/freedesktop/login1";
inline constexpr char kManagerInterface[] = "org.freedesktop.login1.Manager";

// Used as the lock owner, which lists rebrands of the electron executable as
// separate entities.
inline base::FilePath::StringType GetExecutableBaseName() {
  return base::CommandLine::ForCurrentProcess()
      ->GetProgram()
      .BaseName()
      .value();
}

}  // namespace electron::logind

#endif  // ELECTRON_SHELL_BROWSER_LIB_LOGIND_UTILS_H_
