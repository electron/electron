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

#include "base/apple/foundation_util.h"
#include "base/apple/scoped_cftyperef.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/mac/mac_util.h"
#include "base/run_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "electron/mas.h"
#include "shell/browser/native_window.h"
#include "shell/browser/native_window_mac_remote.h"
#include "shell/common/electron_paths.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/thread_restrictions.h"

@interface PopUpButtonHandler : NSObject

@property(nonatomic, weak) NSSavePanel* savePanel;
@property(nonatomic, strong) NSArray* fileTypesList;

- (instancetype)initWithPanel:(NSSavePanel*)panel
                 andTypesList:(NSArray*)typesList;
- (void)selectFormat:(id)sender;

@end

@implementation PopUpButtonHandler

@synthesize savePanel;
@synthesize fileTypesList;

- (instancetype)initWithPanel:(NSSavePanel*)panel
                 andTypesList:(NSArray*)typesList {
  self = [super init];
  if (self) {
    [self setSavePanel:panel];
    [self setFileTypesList:typesList];
  }
  return self;
}

- (void)selectFormat:(id)sender {
  NSPopUpButton* button = (NSPopUpButton*)sender;
  NSInteger selectedItemIndex = [button indexOfSelectedItem];
  NSArray* list = [self fileTypesList];
  NSArray* fileTypes = [list objectAtIndex:selectedItemIndex];

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  // If we meet a '*' file extension, we allow all file types.
  if ([fileTypes count] == 0 || [fileTypes containsObject:@"*"])
    [[self savePanel] setAllowedFileTypes:nil];
  else
    [[self savePanel] setAllowedFileTypes:fileTypes];
#pragma clang diagnostic pop
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
    NSMutableOrderedSet* file_type_set =
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

      [file_type_set addObject:@(ext.c_str())];
    }

    [file_types_list addObject:[file_type_set array]];
  }

  // Passing empty array to setAllowedFileTypes will cause exception.
  NSArray* file_types = nil;
  NSUInteger count = [file_types_list count];
  if (count > 0) {
    file_types = [[file_types_list objectAtIndex:0] allObjects];
    // If we meet a '*' file extension, we allow all the file types and no
    // need to set the specified file types.
    if ([file_types count] == 0 || [file_types containsObject:@"*"])
      file_types = nil;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  [dialog setAllowedFileTypes:file_types];
#pragma clang diagnostic pop

  if (count <= 1)
    return;  // don't add file format picker

  // Add file format picker.
  ElectronAccessoryView* accessoryView = [[ElectronAccessoryView alloc]
      initWithFrame:NSMakeRect(0.0, 0.0, 200, 32.0)];
  NSRect labelFrame = NSMakeRect(0, 0, 60, 22);
  NSRect popupButtonFrame = NSMakeRect(50.0, 2, 140, 22.0);
  if (@available(macOS 26.0, *)) {
    labelFrame.origin.y = 4;
    popupButtonFrame.origin.y = 6;
  }
  NSTextField* label = [[NSTextField alloc] initWithFrame:labelFrame];

  [label setEditable:NO];
  [label setStringValue:@"Format:"];
  [label setBordered:NO];
  [label setBezeled:NO];
  [label setDrawsBackground:NO];

  NSPopUpButton* popupButton =
      [[NSPopUpButton alloc] initWithFrame:popupButtonFrame pullsDown:NO];
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

  base::FilePath default_path = settings.default_path.empty()
                                    ? electron::GetDefaultPath()
                                    : settings.default_path;

  NSString* default_dir = nil;
  NSString* default_filename = nil;
  if (!default_path.empty()) {
    electron::ScopedAllowBlockingForElectron allow_blocking;
    if (base::DirectoryExists(default_path)) {
      default_dir = base::SysUTF8ToNSString(default_path.value());
    } else {
      if (default_path.IsAbsolute()) {
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

// The window the dialog is a sheet of, if any. A window in an app shim has no
// NSWindow in this process that could take one.
NSWindow* SheetParent(const DialogSettings& settings) {
  if (!settings.parent_window || settings.force_detached ||
      settings.parent_window->IsInAppShim()) {
    return nil;
  }
  return settings.parent_window->GetNativeWindow().GetNativeNSWindow();
}

// The window in an app shim that shows the dialog as its sheet, if any.
electron::NativeWindowMacRemote* ShimParent(const DialogSettings& settings) {
  if (!settings.parent_window || settings.force_detached ||
      !settings.parent_window->IsInAppShim()) {
    return nullptr;
  }
  return static_cast<electron::NativeWindowMacRemote*>(settings.parent_window);
}

// Run modal dialog with parent window and return user's choice.
int RunModalDialog(NSSavePanel* dialog, const DialogSettings& settings) {
  __block int chosen = NSModalResponseCancel;
  NSWindow* window = SheetParent(settings);
  if (!window) {
    chosen = [dialog runModal];
  } else {
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
  NSString* path = [url path];
  if (!path)
    return "";

  // Create the file if it doesn't exist (necessary for NSSavePanel options).
  NSFileManager* defaultManager = [NSFileManager defaultManager];
  if (![defaultManager fileExistsAtPath:path]) {
    [defaultManager createFileAtPath:path contents:nil attributes:nil];
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
    if (!path)
      continue;

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
      if (!exists || !(is_directory || is_package_as_directory))
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

// Waits, in a nested loop, for a dialog that a shim's process is showing.
template <typename Result>
Result WaitForShimDialog(
    base::OnceCallback<void(base::OnceCallback<void(bool, const Result&)>)>
        show) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  Result result;
  std::move(show).Run(base::BindOnce(
      [](base::OnceClosure quit, Result* out, bool canceled,
         const Result& value) {
        if (!canceled)
          *out = value;
        std::move(quit).Run();
      },
      run_loop.QuitClosure(), &result));
  run_loop.Run();
  return result;
}

gin_helper::Dictionary OpenDialogResult(
    v8::Isolate* isolate,
    bool canceled,
    const std::vector<base::FilePath>& paths,
    const std::vector<std::string>& bookmarks) {
  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
  dict.Set("canceled", canceled);
  dict.Set("filePaths", paths);
#if IS_MAS_BUILD()
  dict.Set("bookmarks", bookmarks);
#endif
  return dict;
}

gin_helper::Dictionary SaveDialogResult(v8::Isolate* isolate,
                                        bool canceled,
                                        const base::FilePath& path) {
  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
  dict.Set("canceled", canceled);
  dict.Set("filePath", path);
  return dict;
}

}  // namespace

bool ShowOpenDialogSync(const DialogSettings& settings,
                        std::vector<base::FilePath>* paths) {
  DCHECK(paths);
  if (auto* shim_parent = ShimParent(settings)) {
    *paths = WaitForShimDialog<std::vector<base::FilePath>>(
        base::BindOnce(&electron::NativeWindowMacRemote::ShowOpenDialog,
                       base::Unretained(shim_parent), settings));
    return !paths->empty();
  }

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
  std::vector<base::FilePath> paths;
  std::vector<std::string> bookmarks;
  if (chosen != NSModalResponseCancel) {
#if IS_MAS_BUILD()
    if (security_scoped_bookmarks)
      ReadDialogPathsWithBookmarks(dialog, &paths, &bookmarks);
    else
      ReadDialogPaths(dialog, &paths);
#else
    ReadDialogPaths(dialog, &paths);
#endif
  }
  // Built before the promise is converted: As() moves it, isolate and all.
  v8::Local<v8::Value> result =
      OpenDialogResult(promise.isolate(), chosen == NSModalResponseCancel,
                       paths, bookmarks)
          .GetHandle();
  ResolvePromiseInNextTick(promise.As<v8::Local<v8::Value>>(), result);
}

void ShowOpenDialog(const DialogSettings& settings,
                    gin_helper::Promise<gin_helper::Dictionary> promise) {
  if (auto* shim_parent = ShimParent(settings)) {
    shim_parent->ShowOpenDialog(
        settings,
        base::BindOnce(
            [](gin_helper::Promise<gin_helper::Dictionary> promise,
               bool canceled, const std::vector<base::FilePath>& paths) {
              v8::HandleScope scope(promise.isolate());
              promise.Resolve(
                  OpenDialogResult(promise.isolate(), canceled, paths, {}));
            },
            std::move(promise)));
    return;
  }

  NSOpenPanel* dialog = [NSOpenPanel openPanel];

  SetupDialog(dialog, settings);
  SetupOpenDialogForProperties(dialog, settings.properties);

  // Capture the value of the security_scoped_bookmarks settings flag
  // and pass it to the completion handler.
  bool security_scoped_bookmarks = settings.security_scoped_bookmarks;

  __block gin_helper::Promise<gin_helper::Dictionary> p = std::move(promise);

  if (NSWindow* window = SheetParent(settings)) {
    [dialog
        beginSheetModalForWindow:window
               completionHandler:^(NSInteger chosen) {
                 OpenDialogCompletion(chosen, dialog, security_scoped_bookmarks,
                                      std::move(p));
               }];
  } else {
    [dialog beginWithCompletionHandler:^(NSInteger chosen) {
      OpenDialogCompletion(chosen, dialog, security_scoped_bookmarks,
                           std::move(p));
    }];
  }
}

void ShowOpenDialogForNSWindow(const DialogSettings& settings,
                               gfx::NativeWindow window,
                               OpenDialogCallback callback) {
  NSOpenPanel* dialog = [NSOpenPanel openPanel];

  SetupDialog(dialog, settings);
  SetupOpenDialogForProperties(dialog, settings.properties);

  __block OpenDialogCallback cb = std::move(callback);
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::SequencedTaskRunner::GetCurrentDefault();
  [dialog
      beginSheetModalForWindow:window.GetNativeNSWindow()
             completionHandler:^(NSInteger chosen) {
               std::vector<base::FilePath> paths;
               if (chosen != NSModalResponseCancel)
                 ReadDialogPaths(dialog, &paths);
               // Not from inside the panel's own transaction.
               task_runner->PostTask(
                   FROM_HERE, base::BindOnce(std::move(cb),
                                             chosen == NSModalResponseCancel,
                                             std::move(paths)));
             }];
}

std::optional<base::FilePath> ShowSaveDialogSync(
    const DialogSettings& settings) {
  if (auto* shim_parent = ShimParent(settings)) {
    base::FilePath path = WaitForShimDialog<base::FilePath>(
        base::BindOnce(&electron::NativeWindowMacRemote::ShowSaveDialog,
                       base::Unretained(shim_parent), settings));
    if (path.empty())
      return {};
    return path;
  }

  NSSavePanel* dialog = [NSSavePanel savePanel];

  SetupDialog(dialog, settings);
  SetupSaveDialogForProperties(dialog, settings.properties);

  const int chosen = RunModalDialog(dialog, settings);
  if (chosen == NSModalResponseCancel || ![[dialog URL] isFileURL])
    return {};

  return base::FilePath{base::SysNSStringToUTF8([[dialog URL] path])};
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
  if (auto* shim_parent = ShimParent(settings)) {
    shim_parent->ShowSaveDialog(
        settings, base::BindOnce(
                      [](gin_helper::Promise<gin_helper::Dictionary> promise,
                         bool canceled, const base::FilePath& path) {
                        v8::HandleScope scope(promise.isolate());
                        promise.Resolve(SaveDialogResult(promise.isolate(),
                                                         canceled, path));
                      },
                      std::move(promise)));
    return;
  }

  NSSavePanel* dialog = [NSSavePanel savePanel];

  SetupDialog(dialog, settings);
  SetupSaveDialogForProperties(dialog, settings.properties);
  [dialog setCanSelectHiddenExtension:YES];

  // Capture the value of the security_scoped_bookmarks settings flag
  // and pass it to the completion handler.
  bool security_scoped_bookmarks = settings.security_scoped_bookmarks;

  __block gin_helper::Promise<gin_helper::Dictionary> p = std::move(promise);

  if (NSWindow* window = SheetParent(settings)) {
    [dialog
        beginSheetModalForWindow:window
               completionHandler:^(NSInteger chosen) {
                 SaveDialogCompletion(chosen, dialog, security_scoped_bookmarks,
                                      std::move(p));
               }];
  } else {
    [dialog beginWithCompletionHandler:^(NSInteger chosen) {
      SaveDialogCompletion(chosen, dialog, security_scoped_bookmarks,
                           std::move(p));
    }];
  }
}

void ShowSaveDialogForNSWindow(const DialogSettings& settings,
                               gfx::NativeWindow window,
                               SaveDialogCallback callback) {
  NSSavePanel* dialog = [NSSavePanel savePanel];

  SetupDialog(dialog, settings);
  SetupSaveDialogForProperties(dialog, settings.properties);
  [dialog setCanSelectHiddenExtension:YES];

  __block SaveDialogCallback cb = std::move(callback);
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::SequencedTaskRunner::GetCurrentDefault();
  [dialog
      beginSheetModalForWindow:window.GetNativeNSWindow()
             completionHandler:^(NSInteger chosen) {
               base::FilePath path;
               if (chosen != NSModalResponseCancel && [[dialog URL] isFileURL])
                 path = base::FilePath(
                     base::SysNSStringToUTF8([[dialog URL] path]));
               task_runner->PostTask(FROM_HERE,
                                     base::BindOnce(std::move(cb), path.empty(),
                                                    std::move(path)));
             }];
}

}  // namespace file_dialog
