// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/message_box.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#import <Cocoa/Cocoa.h>

#include "base/callback.h"
#include "base/containers/contains.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/no_destructor.h"
#include "base/strings/sys_string_conversions.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/native_window.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/image/image_skia.h"

namespace electron {

MessageBoxSettings::MessageBoxSettings() = default;
MessageBoxSettings::MessageBoxSettings(const MessageBoxSettings&) = default;
MessageBoxSettings::~MessageBoxSettings() = default;

namespace {

// <ID, messageBox> map
std::map<int, NSAlert*>& GetDialogsMap() {
  static base::NoDestructor<std::map<int, NSAlert*>> dialogs;
  return *dialogs;
}

NSAlert* CreateNSAlert(const MessageBoxSettings& settings) {
  // Ignore the title; it's the window title on other platforms and ignorable.
  NSAlert* alert = [[NSAlert alloc] init];
  [alert setMessageText:base::SysUTF8ToNSString(settings.message)];
  [alert setInformativeText:base::SysUTF8ToNSString(settings.detail)];

  switch (settings.type) {
    case MessageBoxType::kInformation:
      alert.alertStyle = NSAlertStyleInformational;
      break;
    case MessageBoxType::kWarning:
    case MessageBoxType::kError:
      // NSWarningAlertStyle shows the app icon while NSAlertStyleCritical
      // shows a warning icon with an app icon badge. Since there is no
      // error variant, lets just use NSAlertStyleCritical.
      alert.alertStyle = NSAlertStyleCritical;
      break;
    default:
      break;
  }

  for (size_t i = 0; i < settings.buttons.size(); ++i) {
    NSString* title = base::SysUTF8ToNSString(settings.buttons[i]);
    // An empty title causes crash on macOS.
    if (settings.buttons[i].empty())
      title = @"(empty)";
    NSButton* button = [alert addButtonWithTitle:title];
    [button setTag:i];
  }

  NSArray* ns_buttons = [alert buttons];
  int button_count = static_cast<int>([ns_buttons count]);

  if (settings.default_id >= 0 && settings.default_id < button_count) {
    // Highlight the button at default_id
    [[ns_buttons objectAtIndex:settings.default_id] highlight:YES];

    // The first button added gets set as the default selected, so remove
    // that and set the button @ default_id to be default.
    [[ns_buttons objectAtIndex:0] setKeyEquivalent:@""];
    [[ns_buttons objectAtIndex:settings.default_id] setKeyEquivalent:@"\r"];
  }

  // Bind cancel id button to escape key if there is more than one button
  if (button_count > 1 && settings.cancel_id >= 0 &&
      settings.cancel_id < button_count) {
    [[ns_buttons objectAtIndex:settings.cancel_id] setKeyEquivalent:@"\e"];
  }

  if (!settings.checkbox_label.empty()) {
    alert.showsSuppressionButton = YES;
    alert.suppressionButton.title =
        base::SysUTF8ToNSString(settings.checkbox_label);
    alert.suppressionButton.state =
        settings.checkbox_checked ? NSOnState : NSOffState;
  }

  if (!settings.icon.isNull()) {
    NSImage* image = skia::SkBitmapToNSImageWithColorSpace(
        *settings.icon.bitmap(), base::mac::GetGenericRGBColorSpace());
    [alert setIcon:image];
  }

  if (settings.text_width > 0) {
    NSRect rect = NSMakeRect(0, 0, settings.text_width, 0);
    NSView* accessoryView = [[NSView alloc] initWithFrame:rect];
    [alert setAccessoryView:[accessoryView autorelease]];
  }

  return alert;
}

}  // namespace

int ShowMessageBoxSync(const MessageBoxSettings& settings) {
  base::scoped_nsobject<NSAlert> alert(CreateNSAlert(settings));

  // Use runModal for synchronous alert without parent, since we don't have a
  // window to wait for. Also use it when window is provided but it is not
  // shown as it would be impossible to dismiss the alert if it is connected
  // to invisible window (see #22671).
  if (!settings.parent_window || !settings.parent_window->IsVisible())
    return [alert runModal];

  __block int ret_code = -1;

  NSWindow* window =
      settings.parent_window->GetNativeWindow().GetNativeNSWindow();
  [alert beginSheetModalForWindow:window
                completionHandler:^(NSModalResponse response) {
                  ret_code = response;
                  [NSApp stopModal];
                }];

  [NSApp runModalForWindow:window];
  return ret_code;
}

void ShowMessageBox(const MessageBoxSettings& settings,
                    MessageBoxCallback callback) {
  NSAlert* alert = CreateNSAlert(settings);

  // Use runModal for synchronous alert without parent, since we don't have a
  // window to wait for.
  if (!settings.parent_window) {
    int ret = [[alert autorelease] runModal];
    std::move(callback).Run(ret, alert.suppressionButton.state == NSOnState);
  } else {
    if (settings.id) {
      if (base::Contains(GetDialogsMap(), *settings.id))
        CloseMessageBox(*settings.id);
      GetDialogsMap()[*settings.id] = alert;
    }

    NSWindow* window =
        settings.parent_window
            ? settings.parent_window->GetNativeWindow().GetNativeNSWindow()
            : nil;

    // Duplicate the callback object here since c is a reference and gcd would
    // only store the pointer, by duplication we can force gcd to store a copy.
    __block MessageBoxCallback callback_ = std::move(callback);
    __block absl::optional<int> id = std::move(settings.id);
    __block int cancel_id = settings.cancel_id;

    auto handler = ^(NSModalResponse response) {
      if (id)
        GetDialogsMap().erase(*id);
      // When the alert is cancelled programmatically, the response would be
      // something like -1000. This currently only happens when users call
      // CloseMessageBox API, and we should return cancelId as result.
      if (response < 0)
        response = cancel_id;
      bool suppressed = alert.suppressionButton.state == NSOnState;
      [alert release];
      // The completionHandler runs inside a transaction commit, and we should
      // not do any runModal inside it. However since we can not control what
      // users will run in the callback, we have to delay running the callback
      // until next tick, otherwise crash like this may happen:
      // https://github.com/electron/electron/issues/26884
      content::GetUIThreadTaskRunner({})->PostTask(
          FROM_HERE,
          base::BindOnce(std::move(callback_), response, suppressed));
    };
    [alert beginSheetModalForWindow:window completionHandler:handler];
  }
}

void CloseMessageBox(int id) {
  auto it = GetDialogsMap().find(id);
  if (it == GetDialogsMap().end()) {
    LOG(ERROR) << "CloseMessageBox called with nonexistent ID";
    return;
  }
  [NSApp endSheet:it->second.window];
}

void ShowErrorBox(const std::u16string& title, const std::u16string& content) {
  NSAlert* alert = [[NSAlert alloc] init];
  [alert setMessageText:base::SysUTF16ToNSString(title)];
  [alert setInformativeText:base::SysUTF16ToNSString(content)];
  [alert setAlertStyle:NSAlertStyleCritical];
  [alert runModal];
  [alert release];
}

}  // namespace electron
