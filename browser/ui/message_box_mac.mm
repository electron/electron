// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/ui/message_box.h"

#import <Cocoa/Cocoa.h>

#include "base/callback.h"
#include "base/strings/sys_string_conversions.h"
#include "browser/native_window.h"
#include "browser/ui/nsalert_synchronous_sheet_mac.h"

namespace atom {

int ShowMessageBox(NativeWindow* parent_window,
                   MessageBoxType type,
                   const std::vector<std::string>& buttons,
                   const std::string& title,
                   const std::string& message,
                   const std::string& detail) {
  // Ignore the title; it's the window title on other platforms and ignorable.
  NSAlert* alert = [[[NSAlert alloc] init] autorelease];
  [alert setMessageText:base::SysUTF8ToNSString(message)];
  [alert setInformativeText:base::SysUTF8ToNSString(detail)];

  switch (type) {
    case MESSAGE_BOX_TYPE_INFORMATION:
      [alert setAlertStyle:NSInformationalAlertStyle];
      break;
    case MESSAGE_BOX_TYPE_WARNING:
      [alert setAlertStyle:NSWarningAlertStyle];
      break;
    default:
      break;
  }

  for (size_t i = 0; i < buttons.size(); ++i) {
    NSString* title = base::SysUTF8ToNSString(buttons[i]);
    NSButton* button = [alert addButtonWithTitle:title];
    [button setTag:i];
  }

  if (parent_window)
    return [alert runModalSheetForWindow:parent_window->GetNativeWindow()];
  else
    return [alert runModal];
}

void ShowMessageBox(NativeWindow* parent_window,
                    MessageBoxType type,
                    const std::vector<std::string>& buttons,
                    const std::string& title,
                    const std::string& message,
                    const std::string& detail,
                    const MessageBoxCallback& callback) {
  callback.Run(ShowMessageBox(
      parent_window, type, buttons, title, message, detail));
}

}  // namespace atom
