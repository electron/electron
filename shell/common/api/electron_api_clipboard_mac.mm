// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_clipboard.h"

#include <vector>

#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "ui/base/cocoa/find_pasteboard.h"

namespace electron {

namespace api {

void Clipboard::WriteFindText(const base::string16& text) {
  NSString* text_ns = base::SysUTF16ToNSString(text);
  [[FindPasteboard sharedInstance] setFindText:text_ns];
}

base::string16 Clipboard::ReadFindText() {
  return GetFindPboardText();
}

std::vector<base::FilePath> Clipboard::ReadFilePaths() {
  std::vector<base::FilePath> result;
  NSArray* paths = [[NSPasteboard generalPasteboard]
      propertyListForType:NSFilenamesPboardType];
  if (paths) {
    result.reserve([paths count]);
    for (NSString* path in paths)
      result.push_back(base::mac::NSStringToFilePath(path));
  }
  return result;
}

void Clipboard::WriteFilePaths(const std::vector<base::FilePath>& paths) {
  NSMutableArray* filePaths = [NSMutableArray array];
  for (const auto& path : paths)
    [filePaths addObject:base::SysUTF8ToNSString(path.value())];
  NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
  [pasteboard declareTypes:@[ NSFilenamesPboardType ] owner:nil];
  [pasteboard setPropertyList:filePaths forType:NSFilenamesPboardType];
}

}  // namespace api

}  // namespace electron
