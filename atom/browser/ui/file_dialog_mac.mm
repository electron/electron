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

@interface PopUpButtonHandler : NSObject
@property (nonatomic, strong) NSSavePanel *savePanel;
@property (nonatomic, strong) NSArray *fileTypes;
- (instancetype)initWithPanel:(NSSavePanel *)panel andTypes:(NSArray *)types;
- (void)selectFormat:(id)sender;
@end

@implementation PopUpButtonHandler
- (instancetype)initWithPanel:(NSSavePanel *)panel andTypes:(NSArray *)types {
  self = [super init];
  if (self) {
    _savePanel = panel;
    _fileTypes = types;
  }
  return self;
}

- (void)selectFormat:(id)sender {
  NSPopUpButton *button = (NSPopUpButton *)sender;
  NSInteger selectedItemIndex = [button indexOfSelectedItem];
  NSString *nameFieldString = [[self savePanel] nameFieldStringValue];
  NSString *trimmedNameFieldString = [nameFieldString stringByDeletingPathExtension];
  NSString *extension = [[self fileTypes] objectAtIndex: selectedItemIndex];

  NSString *nameFieldStringWithExt = [NSString stringWithFormat:@"%@.%@", trimmedNameFieldString, extension];
  [[self savePanel] setNameFieldStringValue:nameFieldStringWithExt];
  [[self savePanel] setAllowedFileTypes:@[extension]];
}
@end

namespace file_dialog {

namespace {

static PopUpButtonHandler *popUpButtonHandler;

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

  if (!popUpButtonHandler)
    popUpButtonHandler = [[PopUpButtonHandler alloc] initWithPanel:dialog andTypes:file_types];

  // add file format picker
  NSView  *accessoryView = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 200, 32.0)];
  NSTextField *label = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 60, 22)];

  [label setEditable:NO];
  [label setStringValue:@"Format:"];
  [label setBordered:NO];
  [label setBezeled:NO];
  [label setDrawsBackground:NO];

  NSPopUpButton *popupButton = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(50.0, 2, 140, 22.0) pullsDown:NO];
  [popupButton addItemsWithTitles:file_types];
  [popupButton setTarget:popUpButtonHandler];
  [popupButton setAction:@selector(selectFormat:)];

  [accessoryView addSubview:label];
  [accessoryView addSubview:popupButton];

  [dialog setAccessoryView:accessoryView];
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

// Create bookmark data and serialise it into a base64 string.
std::string GetBookmarkDataFromNSURL(NSURL* url) {
  // Create the file if it doesn't exist (necessary for NSSavePanel options).
  NSFileManager *defaultManager = [NSFileManager defaultManager];
  if (![defaultManager fileExistsAtPath: [url path]]) {
    [defaultManager createFileAtPath: [url path] contents: nil attributes: nil];
  }

  NSError *error = nil;
  NSData *bookmarkData = [url bookmarkDataWithOptions: NSURLBookmarkCreationWithSecurityScope
                       includingResourceValuesForKeys: nil
                                        relativeToURL: nil
                                                error: &error];
  if (error != nil) {
    // Send back an empty string if there was an error.
    return "";
  } else {
    // Encode NSData in base64 then convert to NSString.
    NSString *base64data = [[NSString alloc] initWithData: [bookmarkData base64EncodedDataWithOptions: 0]
                                                 encoding: NSUTF8StringEncoding];
    return base::SysNSStringToUTF8(base64data);
  }
}

void ReadDialogPathsWithBookmarks(NSOpenPanel* dialog,
                                  std::vector<base::FilePath>* paths,
                                  std::vector<std::string>* bookmarks) {
  NSArray* urls = [dialog URLs];
  for (NSURL* url in urls)
    if ([url isFileURL]) {
      paths->push_back(base::FilePath(base::SysNSStringToUTF8([url path])));
      bookmarks->push_back(GetBookmarkDataFromNSURL(url));
    }
}

void ReadDialogPaths(NSOpenPanel* dialog, std::vector<base::FilePath>* paths) {
  std::vector<std::string> ignored_bookmarks;
  ReadDialogPathsWithBookmarks(dialog, paths, &ignored_bookmarks);
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

void OpenDialogCompletion(int chosen, NSOpenPanel* dialog,
                          const DialogSettings& settings,
                          const OpenDialogCallback& callback) {
  if (chosen == NSFileHandlingPanelCancelButton) {
    #if defined(MAS_BUILD)
      callback.Run(false, std::vector<base::FilePath>(),
                   std::vector<std::string>());
    #else
      callback.Run(false, std::vector<base::FilePath>());
    #endif
  } else {
    std::vector<base::FilePath> paths;
    #if defined(MAS_BUILD)
      std::vector<std::string> bookmarks;
      if (settings.security_scoped_bookmarks) {
        ReadDialogPathsWithBookmarks(dialog, &paths, &bookmarks);
      } else {
        ReadDialogPaths(dialog, &paths);
      }
      callback.Run(true, paths, bookmarks);
    #else
      ReadDialogPaths(dialog, &paths);
      callback.Run(true, paths);
    #endif
  }
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
    OpenDialogCompletion(chosen, dialog, settings, callback);
  } else {
    NSWindow* window = settings.parent_window->GetNativeWindow();
    [dialog beginSheetModalForWindow:window
                   completionHandler:^(NSInteger chosen) {
      OpenDialogCompletion(chosen, dialog, settings, callback);
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

void SaveDialogCompletion(int chosen, NSSavePanel* dialog,
                          const DialogSettings& settings,
                          const SaveDialogCallback& callback) {
  if (chosen == NSFileHandlingPanelCancelButton) {
    #if defined(MAS_BUILD)
      callback.Run(false, base::FilePath(), "");
    #else
      callback.Run(false, base::FilePath());
    #endif
  } else {
    std::string path = base::SysNSStringToUTF8([[dialog URL] path]);
    #if defined(MAS_BUILD)
      std::string bookmark;
      if (settings.security_scoped_bookmarks) {
        bookmark = GetBookmarkDataFromNSURL([dialog URL]);
      }
      callback.Run(true, base::FilePath(path), bookmark);
    #else
      callback.Run(true, base::FilePath(path));
    #endif
  }
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
    SaveDialogCompletion(chosen, dialog, settings, callback);
  } else {
    NSWindow* window = settings.parent_window->GetNativeWindow();
    [dialog beginSheetModalForWindow:window
                   completionHandler:^(NSInteger chosen) {
      SaveDialogCompletion(chosen, dialog, settings, callback);
    }];
  }
}

}  // namespace file_dialog
