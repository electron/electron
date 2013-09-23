// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/ui/message_box.h"

#import <Cocoa/Cocoa.h>

#include "base/callback.h"
#include "base/strings/sys_string_conversions.h"
#include "browser/native_window.h"
#include "browser/ui/nsalert_synchronous_sheet_mac.h"

@interface ModalDelegate : NSObject {
 @private
  atom::MessageBoxCallback callback_;
  NSAlert* alert_;
}
- (id)initWithCallback:(const atom::MessageBoxCallback&)callback
              andAlert:(NSAlert*)alert;
@end

@implementation ModalDelegate

- (id)initWithCallback:(const atom::MessageBoxCallback&)callback
              andAlert:(NSAlert*)alert {
  if ((self = [super init])) {
    callback_ = callback;
    alert_ = alert;
  }
  return self;
}

- (void)alertDidEnd:(NSAlert*)alert
         returnCode:(NSInteger)returnCode
        contextInfo:(void*)contextInfo {
  callback_.Run(returnCode);
  [alert_ release];
  [self release];
}

@end

namespace atom {

namespace {

NSAlert* CreateNSAlert(NativeWindow* parent_window,
                       MessageBoxType type,
                       const std::vector<std::string>& buttons,
                       const std::string& title,
                       const std::string& message,
                       const std::string& detail) {
  // Ignore the title; it's the window title on other platforms and ignorable.
  NSAlert* alert = [[NSAlert alloc] init];
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

  return alert;
}

}  // namespace

int ShowMessageBox(NativeWindow* parent_window,
                   MessageBoxType type,
                   const std::vector<std::string>& buttons,
                   const std::string& title,
                   const std::string& message,
                   const std::string& detail) {
  NSAlert* alert = CreateNSAlert(
      parent_window, type, buttons, title, message, detail);
  [alert autorelease];

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
  NSAlert* alert = CreateNSAlert(
      parent_window, type, buttons, title, message, detail);
  ModalDelegate* delegate = [[ModalDelegate alloc] initWithCallback:callback
                                                           andAlert:alert];

  NSWindow* window = parent_window ? parent_window->GetNativeWindow() : nil;
  [alert beginSheetModalForWindow:window
                    modalDelegate:delegate
                   didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:)
                      contextInfo:nil];
}

}  // namespace atom
