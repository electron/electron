// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/file_dialog.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "base/apple/foundation_util.h"
#include "base/apple/scoped_cftyperef.h"
#include "base/files/file_util.h"
#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "electron/mas.h"
#include "shell/browser/native_window.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/thread_restrictions.h"

@interface PopUpButtonHandler : NSObject

@property(nonatomic, assign) NSSavePanel* savePanel;
@property(nonatomic, strong) NSArray* contentTypesList;

- (instancetype)initWithPanel:(NSSavePanel*)panel
                 andTypesList:(NSArray*)typesList;
- (void)selectFormat:(id)sender;

@end

@implementation PopUpButtonHandler

@synthesize savePanel;
@synthesize contentTypesList;

- (instancetype)initWithPanel:(NSSavePanel*)panel
                 andTypesList:(NSArray*)typesList {
  self = [super init];
  if (self) {
    [self setSavePanel:panel];
    [self setContentTypesList:typesList];
  }
  return self;
}

- (void)selectFormat:(id)sender {
  NSPopUpButton* button = (NSPopUpButton*)sender;
  NSInteger selectedItemIndex = [button indexOfSelectedItem];
  NSArray* list = [self contentTypesList];
  NSArray* content_types = [list objectAtIndex:selectedItemIndex];

  __block BOOL allowAllFiles = NO;
  [content_types
      enumerateObjectsUsingBlock:^(UTType* type, NSUInteger idx, BOOL* stop) {
        if ([[type preferredFilenameExtension] isEqual:@"*"]) {
          allowAllFiles = YES;
          *stop = YES;
        }
      }];

  [[self savePanel] setAllowedContentTypes:allowAllFiles ? @[] : content_types];
}

@end

// Manages the PopUpButtonHandler.
@interface ElectronAccessoryView : NSView
@property(nonatomic, strong) PopUpButtonHandler* popUpButtonHandler;
@end

@implementation ElectronAccessoryView

@synthesize popUpButtonHandler;

- (void)dealloc {
  auto* popupButton =
      static_cast<NSPopUpButton*>([[self subviews] objectAtIndex:1]);
  popupButton.target = nil;
  popUpButtonHandler = nil;
}

@end

namespace file_dialog {

DialogSettings::DialogSettings() = default;
DialogSettings::DialogSettings(const DialogSettings&) = default;
DialogSettings::~DialogSettings() = default;

namespace {

void SetAllowedFileTypes(NSSavePanel* dialog, const Filters& filters) {
  NSMutableArray* file_types_list = [NSMutableArray array];
  NSMutableArray* filter_names = [NSMutableArray array];

  // Create array to keep file types and their name.
  for (const Filter& filter : filters) {
    NSMutableOrderedSet* content_types_set =
        [NSMutableOrderedSet orderedSetWithCapacity:filters.size()];
    [filter_names addObject:@(filter.first.c_str())];

    for (std::string ext : filter.second) {
      // macOS is incapable of understanding multiple file extensions,
      // so we need to tokenize the extension that's been passed in.
      // We want to err on the side of allowing files, so we pass
      // along only the final extension ('tar.gz' => 'gz').
      auto pos = ext.rfind('.');
      if (pos != std::string::npos) {
        ext.erase(0, pos + 1);
      }

      if (ext == "*") {
        [content_types_set addObject:[UTType typeWithFilenameExtension:@"*"]];
        break;
      } else {
        if (UTType* utt = [UTType typeWithFilenameExtension:@(ext.c_str())])
          [content_types_set addObject:utt];
      }
    }

    [file_types_list addObject:content_types_set];
  }

  NSArray* content_types = [file_types_list objectAtIndex:0];

  __block BOOL allowAllFiles = NO;
  [content_types
      enumerateObjectsUsingBlock:^(UTType* type, NSUInteger idx, BOOL* stop) {
        if ([[type preferredFilenameExtension] isEqual:@"*"]) {
          allowAllFiles = YES;
          *stop = YES;
        }
      }];

  [dialog setAllowedContentTypes:allowAllFiles ? @[] : content_types];

  // Don't add file format picker.
  if ([file_types_list count] <= 1)
    return;

  // Add file format picker.
  ElectronAccessoryView* accessoryView = [[ElectronAccessoryView alloc]
      initWithFrame:NSMakeRect(0.0, 0.0, 200, 32.0)];
  NSTextField* label =
      [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 60, 22)];

  [label setEditable:NO];
  [label setStringValue:@"Format:"];
  [label setBordered:NO];
  [label setBezeled:NO];
  [label setDrawsBackground:NO];

  NSPopUpButton* popupButton =
      [[NSPopUpButton alloc] initWithFrame:NSMakeRect(50.0, 2, 140, 22.0)
                                 pullsDown:NO];
  PopUpButtonHandler* popUpButtonHandler =
      [[PopUpButtonHandler alloc] initWithPanel:dialog
                                   andTypesList:file_types_list];
  [popupButton addItemsWithTitles:filter_names];
  [popupButton setTarget:popUpButtonHandler];
  [popupButton setAction:@selector(selectFormat:)];

  [accessoryView addSubview:label];
  [accessoryView addSubview:popupButton];
  [accessoryView setPopUpButtonHandler:popUpButtonHandler];

  [dialog setAccessoryView:accessoryView];
}

void SetupDialog(NSSavePanel* dialog, const DialogSettings& settings) {
  if (!settings.title.empty())
    [dialog setTitle:base::SysUTF8ToNSString(settings.title)];

  if (!settings.button_label.empty())
    [dialog setPrompt:base::SysUTF8ToNSString(settings.button_label)];

  if (!settings.message.empty())
    [dialog setMessage:base::SysUTF8ToNSString(settings.message)];

  if (!settings.name_field_label.empty())
    [dialog
        setNameFieldLabel:base::SysUTF8ToNSString(settings.name_field_label)];

  [dialog setShowsTagField:settings.shows_tag_field];

  NSString* default_dir = nil;
  NSString* default_filename = nil;
  if (!settings.default_path.empty()) {
    electron::ScopedAllowBlockingForElectron allow_blocking;
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

void SetupOpenDialogForProperties(NSOpenPanel* dialog, int properties) {
  [dialog setCanChooseFiles:(properties & OPEN_DIALOG_OPEN_FILE)];
  if (properties & OPEN_DIALOG_OPEN_DIRECTORY)
    [dialog setCanChooseDirectories:YES];
  if (properties & OPEN_DIALOG_CREATE_DIRECTORY)
    [dialog setCanCreateDirectories:YES];
  if (properties & OPEN_DIALOG_MULTI_SELECTIONS)
    [dialog setAllowsMultipleSelection:YES];
  if (properties & OPEN_DIALOG_SHOW_HIDDEN_FILES)
    [dialog setShowsHiddenFiles:YES];
  if (properties & OPEN_DIALOG_NO_RESOLVE_ALIASES)
    [dialog setResolvesAliases:NO];
  if (properties & OPEN_DIALOG_TREAT_PACKAGE_APP_AS_DIRECTORY)
    [dialog setTreatsFilePackagesAsDirectories:YES];
}

void SetupSaveDialogForProperties(NSSavePanel* dialog, int properties) {
  if (properties & SAVE_DIALOG_CREATE_DIRECTORY)
    [dialog setCanCreateDirectories:YES];
  if (properties & SAVE_DIALOG_SHOW_HIDDEN_FILES)
    [dialog setShowsHiddenFiles:YES];
  if (properties & SAVE_DIALOG_TREAT_PACKAGE_APP_AS_DIRECTORY)
    [dialog setTreatsFilePackagesAsDirectories:YES];
}

// Run modal dialog with parent window and return user's choice.
int RunModalDialog(NSSavePanel* dialog, const DialogSettings& settings) {
  __block int chosen = NSModalResponseCancel;
  if (!settings.parent_window || !settings.parent_window->GetNativeWindow() ||
      settings.force_detached) {
    chosen = [dialog runModal];
  } else {
    NSWindow* window =
        settings.parent_window->GetNativeWindow().GetNativeNSWindow();

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
  NSFileManager* defaultManager = [NSFileManager defaultManager];
  if (![defaultManager fileExistsAtPath:[url path]]) {
    [defaultManager createFileAtPath:[url path] contents:nil attributes:nil];
  }

  NSError* error = nil;
  NSData* bookmarkData =
      [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
          includingResourceValuesForKeys:nil
                           relativeToURL:nil
                                   error:&error];
  if (error != nil) {
    // Send back an empty string if there was an error.
    return "";
  } else {
    // Encode NSData in base64 then convert to NSString.
    NSString* base64data = [[NSString alloc]
        initWithData:[bookmarkData base64EncodedDataWithOptions:0]
            encoding:NSUTF8StringEncoding];
    return base::SysNSStringToUTF8(base64data);
  }
}

void ReadDialogPathsWithBookmarks(NSOpenPanel* dialog,
                                  std::vector<base::FilePath>* paths,
                                  std::vector<std::string>* bookmarks) {
  NSArray* urls = [dialog URLs];
  for (NSURL* url in urls) {
    if (![url isFileURL])
      continue;

    NSString* path = [url path];

    // There's a bug in macOS where despite a request to disallow file
    // selection, files/packages can be selected. If file selection
    // was disallowed, drop any files selected. See crbug.com/1357523.
    if (![dialog canChooseFiles]) {
      BOOL is_directory;
      BOOL exists =
          [[NSFileManager defaultManager] fileExistsAtPath:path
                                               isDirectory:&is_directory];
      BOOL is_package_as_directory =
          [[NSWorkspace sharedWorkspace] isFilePackageAtPath:path] &&
          [dialog treatsFilePackagesAsDirectories];
      if (!exists || !is_directory || !is_package_as_directory)
        continue;
    }

    paths->emplace_back(base::SysNSStringToUTF8(path));
    bookmarks->push_back(GetBookmarkDataFromNSURL(url));
  }
}

void ReadDialogPaths(NSOpenPanel* dialog, std::vector<base::FilePath>* paths) {
  std::vector<std::string> ignored_bookmarks;
  ReadDialogPathsWithBookmarks(dialog, paths, &ignored_bookmarks);
}

void ResolvePromiseInNextTick(gin_helper::Promise<v8::Local<v8::Value>> promise,
                              v8::Local<v8::Value> value) {
  // The completionHandler runs inside a transaction commit, and we should
  // not do any runModal inside it. However since we can not control what
  // users will run in the microtask, we have to delay the resolution until
  // next tick, otherwise crash like this may happen:
  // https://github.com/electron/electron/issues/26884
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](gin_helper::Promise<v8::Local<v8::Value>> promise,
             v8::Global<v8::Value> global) {
            v8::Isolate* isolate = promise.isolate();
            v8::HandleScope handle_scope(isolate);
            v8::Local<v8::Value> value = global.Get(isolate);
            promise.Resolve(value);
          },
          std::move(promise), v8::Global<v8::Value>(promise.isolate(), value)));
}

}  // namespace

bool ShowOpenDialogSync(const DialogSettings& settings,
                        std::vector<base::FilePath>* paths) {
  DCHECK(paths);
  NSOpenPanel* dialog = [NSOpenPanel openPanel];

  SetupDialog(dialog, settings);
  SetupOpenDialogForProperties(dialog, settings.properties);

  int chosen = RunModalDialog(dialog, settings);
  if (chosen == NSModalResponseCancel)
    return false;

  ReadDialogPaths(dialog, paths);
  return true;
}

void OpenDialogCompletion(int chosen,
                          NSOpenPanel* dialog,
                          bool security_scoped_bookmarks,
                          gin_helper::Promise<gin_helper::Dictionary> promise) {
  v8::HandleScope scope(promise.isolate());
  auto dict = gin_helper::Dictionary::CreateEmpty(promise.isolate());
  if (chosen == NSModalResponseCancel) {
    dict.Set("canceled", true);
    dict.Set("filePaths", std::vector<base::FilePath>());
#if IS_MAS_BUILD()
    dict.Set("bookmarks", std::vector<std::string>());
#endif
  } else {
    std::vector<base::FilePath> paths;
    dict.Set("canceled", false);
#if IS_MAS_BUILD()
    std::vector<std::string> bookmarks;
    if (security_scoped_bookmarks)
      ReadDialogPathsWithBookmarks(dialog, &paths, &bookmarks);
    else
      ReadDialogPaths(dialog, &paths);
    dict.Set("filePaths", paths);
    dict.Set("bookmarks", bookmarks);
#else
    ReadDialogPaths(dialog, &paths);
    dict.Set("filePaths", paths);
#endif
  }
  ResolvePromiseInNextTick(promise.As<v8::Local<v8::Value>>(),
                           dict.GetHandle());
}

void ShowOpenDialog(const DialogSettings& settings,
                    gin_helper::Promise<gin_helper::Dictionary> promise) {
  NSOpenPanel* dialog = [NSOpenPanel openPanel];

  SetupDialog(dialog, settings);
  SetupOpenDialogForProperties(dialog, settings.properties);

  // Capture the value of the security_scoped_bookmarks settings flag
  // and pass it to the completion handler.
  bool security_scoped_bookmarks = settings.security_scoped_bookmarks;

  __block gin_helper::Promise<gin_helper::Dictionary> p = std::move(promise);

  if (!settings.parent_window || !settings.parent_window->GetNativeWindow() ||
      settings.force_detached) {
    [dialog beginWithCompletionHandler:^(NSInteger chosen) {
      OpenDialogCompletion(chosen, dialog, security_scoped_bookmarks,
                           std::move(p));
    }];
  } else {
    NSWindow* window =
        settings.parent_window->GetNativeWindow().GetNativeNSWindow();
    [dialog
        beginSheetModalForWindow:window
               completionHandler:^(NSInteger chosen) {
                 OpenDialogCompletion(chosen, dialog, security_scoped_bookmarks,
                                      std::move(p));
               }];
  }
}

bool ShowSaveDialogSync(const DialogSettings& settings, base::FilePath* path) {
  DCHECK(path);
  NSSavePanel* dialog = [NSSavePanel savePanel];

  SetupDialog(dialog, settings);
  SetupSaveDialogForProperties(dialog, settings.properties);

  int chosen = RunModalDialog(dialog, settings);
  if (chosen == NSModalResponseCancel || ![[dialog URL] isFileURL])
    return false;

  *path = base::FilePath(base::SysNSStringToUTF8([[dialog URL] path]));
  return true;
}

void SaveDialogCompletion(int chosen,
                          NSSavePanel* dialog,
                          bool security_scoped_bookmarks,
                          gin_helper::Promise<gin_helper::Dictionary> promise) {
  v8::HandleScope scope(promise.isolate());
  auto dict = gin_helper::Dictionary::CreateEmpty(promise.isolate());
  if (chosen == NSModalResponseCancel) {
    dict.Set("canceled", true);
    dict.Set("filePath", base::FilePath());
#if IS_MAS_BUILD()
    dict.Set("bookmark", std::string_view{});
#endif
  } else {
    std::string path = base::SysNSStringToUTF8([[dialog URL] path]);
    dict.Set("filePath", base::FilePath(path));
    dict.Set("canceled", false);
#if IS_MAS_BUILD()
    std::string bookmark;
    if (security_scoped_bookmarks) {
      bookmark = GetBookmarkDataFromNSURL([dialog URL]);
      dict.Set("bookmark", bookmark);
    }
#endif
  }
  ResolvePromiseInNextTick(promise.As<v8::Local<v8::Value>>(),
                           dict.GetHandle());
}

void ShowSaveDialog(const DialogSettings& settings,
                    gin_helper::Promise<gin_helper::Dictionary> promise) {
  NSSavePanel* dialog = [NSSavePanel savePanel];

  SetupDialog(dialog, settings);
  SetupSaveDialogForProperties(dialog, settings.properties);
  [dialog setCanSelectHiddenExtension:YES];

  // Capture the value of the security_scoped_bookmarks settings flag
  // and pass it to the completion handler.
  bool security_scoped_bookmarks = settings.security_scoped_bookmarks;

  __block gin_helper::Promise<gin_helper::Dictionary> p = std::move(promise);

  if (!settings.parent_window || !settings.parent_window->GetNativeWindow() ||
      settings.force_detached) {
    [dialog beginWithCompletionHandler:^(NSInteger chosen) {
      SaveDialogCompletion(chosen, dialog, security_scoped_bookmarks,
                           std::move(p));
    }];
  } else {
    NSWindow* window =
        settings.parent_window->GetNativeWindow().GetNativeNSWindow();
    [dialog
        beginSheetModalForWindow:window
               completionHandler:^(NSInteger chosen) {
                 SaveDialogCompletion(chosen, dialog, security_scoped_bookmarks,
                                      std::move(p));
               }];
  }
}

}  // namespace file_dialog
