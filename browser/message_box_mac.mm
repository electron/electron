// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/message_box.h"

#import <Cocoa/Cocoa.h>

#include "base/strings/sys_string_conversions.h"

namespace atom {

int ShowMessageBox(MessageBoxType type,
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

  return [alert runModal];
}

}  // namespace atom
