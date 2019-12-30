// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/message_box.h"

#include <string>
#include <utility>
#include <vector>

#import <Cocoa/Cocoa.h>

#include "base/callback.h"
#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "shell/browser/native_window.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/image/image_skia.h"

namespace electron {

MessageBoxSettings::MessageBoxSettings() = default;
MessageBoxSettings::MessageBoxSettings(const MessageBoxSettings&) = default;
MessageBoxSettings::~MessageBoxSettings() = default;

namespace {

NSAlert* CreateNSAlert(const MessageBoxSettings& settings) {
  // Ignore the title; it's the window title on other platforms and ignorable.
  NSAlert* alert = [[NSAlert alloc] init];
  [alert setMessageText:base::SysUTF8ToNSString(settings.message)];
  [alert setInformativeText:base::SysUTF8ToNSString(settings.detail)];

  switch (settings.type) {
    case MessageBoxType::kInformation:
      alert.alertStyle = NSInformationalAlertStyle;
      break;
    case MessageBoxType::kWarning:
    case MessageBoxType::kError:
      // NSWarningAlertStyle shows the app icon while NSCriticalAlertStyle
      // shows a warning icon with an app icon badge. Since there is no
      // error variant, lets just use NSCriticalAlertStyle.
      alert.alertStyle = NSCriticalAlertStyle;
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

  return alert;
}

}  // namespace

int ShowMessageBoxSync(const MessageBoxSettings& settings) {
  NSAlert* alert = CreateNSAlert(settings);

  // Use runModal for synchronous alert without parent, since we don't have a
  // window to wait for.
  if (!settings.parent_window)
    return [[alert autorelease] runModal];

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
    NSWindow* window =
        settings.parent_window
            ? settings.parent_window->GetNativeWindow().GetNativeNSWindow()
            : nil;

    // Duplicate the callback object here since c is a reference and gcd would
    // only store the pointer, by duplication we can force gcd to store a copy.
    __block MessageBoxCallback callback_ = std::move(callback);

    [alert beginSheetModalForWindow:window
                  completionHandler:^(NSModalResponse response) {
                    std::move(callback_).Run(
                        response, alert.suppressionButton.state == NSOnState);
                  }];
  }
}

void ShowErrorBox(const base::string16& title, const base::string16& content) {
  NSAlert* alert = [[NSAlert alloc] init];
  [alert setMessageText:base::SysUTF16ToNSString(title)];
  [alert setInformativeText:base::SysUTF16ToNSString(content)];
  [alert setAlertStyle:NSCriticalAlertStyle];
  [alert runModal];
  [alert release];
}

}  // namespace electron
