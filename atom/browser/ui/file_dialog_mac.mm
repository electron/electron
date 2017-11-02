// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/file_dialog.h"

#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

#include "atom/browser/native_window.h"
#include "base/files/file_util.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/strings/sys_string_conversions.h"

namespace file_dialog {

namespace {

void SetAllowedFileTypes(NSSavePanel* dialog, const Filters& filters) {
  NSMutableSet* file_type_set = [NSMutableSet set];
  for (size_t i = 0; i < filters.size(); ++i) {
    const Filter& filter = filters[i];
    for (size_t j = 0; j < filter.second.size(); ++j) {
      // If we meet a '*' file extension, we allow all the file types and no
      // need to set the specified file types.
      if (filter.second[j] == "*") {
        [dialog setAllowsOtherFileTypes:YES];
        return;
      }
      base::ScopedCFTypeRef<CFStringRef> ext_cf(
          base::SysUTF8ToCFStringRef(filter.second[j]));
      [file_type_set addObject:base::mac::CFToNSCast(ext_cf.get())];
    }
  }

  // Passing empty array to setAllowedFileTypes will cause exception.
  NSArray* file_types = nil;
  if ([file_type_set count])
    file_types = [file_type_set allObjects];

  [dialog setAllowedFileTypes:file_types];
}

void SetupDialog(NSSavePanel* dialog,
                 const DialogSettings& settings) {
  if (!settings.title.empty())
    [dialog setTitle:base::SysUTF8ToNSString(settings.title)];

  if (!settings.button_label.empty())
    [dialog setPrompt:base::SysUTF8ToNSString(settings.button_label)];

  if (!settings.message.empty())
    [dialog setMessage:base::SysUTF8ToNSString(settings.message)];

  if (!settings.name_field_label.empty())
    [dialog setNameFieldLabel:base::SysUTF8ToNSString(settings.name_field_label)];

  [dialog setShowsTagField:settings.shows_tag_field];

  NSString* default_dir = nil;
  NSString* default_filename = nil;
  if (!settings.default_path.empty()) {
    if (base::DirectoryExists(settings.default_path)) {
      default_dir = base::SysUTF8ToNSString(settings.default_path.value());
    } else {
      if (settings.default_path.IsAbsolute()) {
        default_dir =
            base::SysUTF8ToNSString(settings.default_path.DirName().value());
      }

      default_filename =
          base::SysUTF8ToNSString(settings.default_path.BaseName().value());
    }
  }

  if (settings.filters.empty()) {
    [dialog setAllowsOtherFileTypes:YES];
  } else {
    // Set setAllowedFileTypes before setNameFieldStringValue as it might
    // override the extension set using setNameFieldStringValue
    SetAllowedFileTypes(dialog, settings.filters);
  }

  // Make sure the extension is always visible. Without this, the extension in
  // the default filename will not be used in the saved file.
  [dialog setExtensionHidden:NO];

  if (default_dir)
    [dialog setDirectoryURL:[NSURL fileURLWithPath:default_dir]];
  if (default_filename)
    [dialog setNameFieldStringValue:default_filename];
}

void SetupDialogForProperties(NSOpenPanel* dialog, int properties) {
  [dialog setCanChooseFiles:(properties & FILE_DIALOG_OPEN_FILE)];
  if (properties & FILE_DIALOG_OPEN_DIRECTORY)
    [dialog setCanChooseDirectories:YES];
  if (properties & FILE_DIALOG_CREATE_DIRECTORY)
    [dialog setCanCreateDirectories:YES];
  if (properties & FILE_DIALOG_MULTI_SELECTIONS)
    [dialog setAllowsMultipleSelection:YES];
  if (properties & FILE_DIALOG_SHOW_HIDDEN_FILES)
    [dialog setShowsHiddenFiles:YES];
  if (properties & FILE_DIALOG_NO_RESOLVE_ALIASES)
    [dialog setResolvesAliases:NO];
  if (properties & FILE_DIALOG_TREAT_PACKAGE_APP_AS_DIRECTORY)
    [dialog setTreatsFilePackagesAsDirectories:YES];
}

// Run modal dialog with parent window and return user's choice.
int RunModalDialog(NSSavePanel* dialog, const DialogSettings& settings) {
  __block int chosen = NSFileHandlingPanelCancelButton;
  if (!settings.parent_window || !settings.parent_window->GetNativeWindow() ||
      settings.force_detached) {
    chosen = [dialog runModal];
  } else {
    NSWindow* window = settings.parent_window->GetNativeWindow();

    [dialog beginSheetModalForWindow:window
                   completionHandler:^(NSInteger c) {
      chosen = c;
      [NSApp stopModal];
    }];
    [NSApp runModalForWindow:window];
  }

  return chosen;
}

void ReadDialogPaths(NSOpenPanel* dialog, std::vector<base::FilePath>* paths) {
  NSArray* urls = [dialog URLs];
  for (NSURL* url in urls)
    if ([url isFileURL])
      paths->push_back(base::FilePath(base::SysNSStringToUTF8([url path])));
}

}  // namespace

bool ShowOpenDialog(const DialogSettings& settings,
                    std::vector<base::FilePath>* paths) {
  DCHECK(paths);
  NSOpenPanel* dialog = [NSOpenPanel openPanel];

  SetupDialog(dialog, settings);
  SetupDialogForProperties(dialog, settings.properties);

  int chosen = RunModalDialog(dialog, settings);
  if (chosen == NSFileHandlingPanelCancelButton)
    return false;

  ReadDialogPaths(dialog, paths);
  return true;
}

void ShowOpenDialog(const DialogSettings& settings,
                    const OpenDialogCallback& c) {
  NSOpenPanel* dialog = [NSOpenPanel openPanel];

  SetupDialog(dialog, settings);
  SetupDialogForProperties(dialog, settings.properties);

  // Duplicate the callback object here since c is a reference and gcd would
  // only store the pointer, by duplication we can force gcd to store a copy.
  __block OpenDialogCallback callback = c;

  if (!settings.parent_window || !settings.parent_window->GetNativeWindow() ||
      settings.force_detached) {
    int chosen = [dialog runModal];
    if (chosen == NSFileHandlingPanelCancelButton) {
      callback.Run(false, std::vector<base::FilePath>());
    } else {
      std::vector<base::FilePath> paths;
      ReadDialogPaths(dialog, &paths);
      callback.Run(true, paths);
    }
  } else {
    NSWindow* window = settings.parent_window->GetNativeWindow();
    [dialog beginSheetModalForWindow:window
                   completionHandler:^(NSInteger chosen) {
      if (chosen == NSFileHandlingPanelCancelButton) {
        callback.Run(false, std::vector<base::FilePath>());
      } else {
        std::vector<base::FilePath> paths;
        ReadDialogPaths(dialog, &paths);
        callback.Run(true, paths);
      }
    }];
  }
}

bool ShowSaveDialog(const DialogSettings& settings,
                    base::FilePath* path) {
  DCHECK(path);
  NSSavePanel* dialog = [NSSavePanel savePanel];

  SetupDialog(dialog, settings);

  int chosen = RunModalDialog(dialog, settings);
  if (chosen == NSFileHandlingPanelCancelButton || ![[dialog URL] isFileURL])
    return false;

  *path = base::FilePath(base::SysNSStringToUTF8([[dialog URL] path]));
  return true;
}

void ShowSaveDialog(const DialogSettings& settings,
                    const SaveDialogCallback& c) {
  NSSavePanel* dialog = [NSSavePanel savePanel];

  SetupDialog(dialog, settings);
  [dialog setCanSelectHiddenExtension:YES];

  __block SaveDialogCallback callback = c;

  if (!settings.parent_window || !settings.parent_window->GetNativeWindow() ||
      settings.force_detached) {
    int chosen = [dialog runModal];
    if (chosen == NSFileHandlingPanelCancelButton) {
      callback.Run(false, base::FilePath());
    } else {
      std::string path = base::SysNSStringToUTF8([[dialog URL] path]);
      callback.Run(true, base::FilePath(path));
    }
  } else {
    NSWindow* window = settings.parent_window->GetNativeWindow();
    [dialog beginSheetModalForWindow:window
                   completionHandler:^(NSInteger chosen) {
      if (chosen == NSFileHandlingPanelCancelButton) {
        callback.Run(false, base::FilePath());
      } else {
        std::string path = base::SysNSStringToUTF8([[dialog URL] path]);
        callback.Run(true, base::FilePath(path));
      }
    }];
  }
}

}  // namespace file_dialog
