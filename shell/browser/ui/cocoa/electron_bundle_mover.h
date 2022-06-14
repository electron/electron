// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_BUNDLE_MOVER_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_BUNDLE_MOVER_H_

#include <string>

#include "base/mac/foundation_util.h"
#include "shell/common/gin_helper/error_thrower.h"

namespace gin {
class Arguments;
}

namespace electron {

// Possible bundle movement conflicts
enum class BundlerMoverConflictType { kExists, kExistsAndRunning };

class ElectronBundleMover {
 public:
  static bool Move(gin_helper::ErrorThrower thrower, gin::Arguments* args);
  static bool IsCurrentAppInApplicationsFolder();

 private:
  static bool ShouldContinueMove(gin_helper::ErrorThrower thrower,
                                 BundlerMoverConflictType type,
                                 gin::Arguments* args);
  static bool IsInApplicationsFolder(NSString* bundlePath);
  static NSString* ContainingDiskImageDevice(NSString* bundlePath);
  static void Relaunch(NSString* destinationPath);
  static NSString* ShellQuotedString(NSString* string);
  static bool CopyBundle(NSString* srcPath, NSString* dstPath);
  static bool AuthorizedInstall(NSString* srcPath,
                                NSString* dstPath,
                                bool* canceled);
  static bool IsApplicationAtPathRunning(NSString* bundlePath);
  static bool DeleteOrTrash(NSString* path);
  static bool Trash(NSString* path);
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_BUNDLE_MOVER_H_
