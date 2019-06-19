// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_COCOA_ATOM_BUNDLE_MOVER_H_
#define ATOM_BROWSER_UI_COCOA_ATOM_BUNDLE_MOVER_H_

#include <string>

#include "native_mate/persistent_dictionary.h"

namespace atom {

namespace ui {

namespace cocoa {

class AtomBundleMover {
 public:
  static bool Move(mate::Arguments* args);
  static bool IsCurrentAppInApplicationsFolder();

 private:
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

}  // namespace cocoa

}  // namespace ui

}  // namespace atom

#endif  // ATOM_BROWSER_UI_COCOA_ATOM_BUNDLE_MOVER_H_
