// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/ui/file_dialog.h"

#import <Cocoa/Cocoa.h>
#include <CoreServices/CoreServices.h>

#include "base/file_util.h"
#include "base/strings/sys_string_conversions.h"
#include "browser/native_window.h"

namespace file_dialog {

namespace {

void SetupDialog(NSSavePanel* dialog,
                 const std::string& title,
                 const base::FilePath& default_path) {
  if (!title.empty())
    [dialog setTitle:base::SysUTF8ToNSString(title)];

  NSString* default_dir = nil;
  NSString* default_filename = nil;
  if (!default_path.empty()) {
    if (file_util::DirectoryExists(default_path)) {
      default_dir = base::SysUTF8ToNSString(default_path.value());
    } else {
      default_dir = base::SysUTF8ToNSString(default_path.DirName().value());
      default_filename =
          base::SysUTF8ToNSString(default_path.BaseName().value());
    }
  }

  if (default_dir)
    [dialog setDirectoryURL:[NSURL fileURLWithPath:default_dir]];
  if (default_filename)
    [dialog setNameFieldStringValue:default_filename];

  [dialog setAllowsOtherFileTypes:YES];
}

}  // namespace

bool ShowOpenDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    int properties,
                    std::vector<base::FilePath>* paths) {
  DCHECK(paths);
  NSOpenPanel* dialog = [NSOpenPanel openPanel];

  SetupDialog(dialog, title, default_path);

  [dialog setCanChooseFiles:(properties & FILE_DIALOG_OPEN_FILE)];
  if (properties & FILE_DIALOG_OPEN_DIRECTORY)
    [dialog setCanChooseDirectories:YES];
  if (properties & FILE_DIALOG_CREATE_DIRECTORY)
    [dialog setCanCreateDirectories:YES];
  if (properties & FILE_DIALOG_MULTI_SELECTIONS)
    [dialog setAllowsMultipleSelection:YES];

  __block int chosen = -1;

  if (parent_window == NULL) {
    chosen = [dialog runModal];
  } else {
    NSWindow* window = parent_window->GetNativeWindow();

    [dialog beginSheetModalForWindow:window
                   completionHandler:^(NSInteger c) {
      chosen = c;
      [NSApp stopModal];
    }];
    [NSApp runModalForWindow:window];
  }

  if (chosen == NSFileHandlingPanelCancelButton)
    return false;

  NSArray* urls = [dialog URLs];
  for (NSURL* url in urls)
    if ([url isFileURL])
      paths->push_back(base::FilePath(base::SysNSStringToUTF8([url path])));

  return true;
}

bool ShowSaveDialog(atom::NativeWindow* window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    base::FilePath* path) {
  DCHECK(window);
  DCHECK(path);
  NSSavePanel* dialog = [NSSavePanel savePanel];

  SetupDialog(dialog, title, default_path);

  [dialog setCanSelectHiddenExtension:YES];

  __block bool result = false;
  __block base::FilePath ret_path;
  [dialog beginSheetModalForWindow:window->GetNativeWindow()
                 completionHandler:^(NSInteger chosen) {
    if (chosen == NSFileHandlingPanelCancelButton ||
        ![[dialog URL] isFileURL]) {
      result = false;
    } else {
      result = true;
      ret_path = base::FilePath(base::SysNSStringToUTF8([[dialog URL] path]));
    }

    [NSApp stopModal];
  }];

  [NSApp runModalForWindow:window->GetNativeWindow()];

  *path = ret_path;
  return result;
}

}  // namespace file_dialog
