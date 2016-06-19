// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/message_box.h"

#import <Cocoa/Cocoa.h>

#include "atom/browser/native_window.h"
#include "base/callback.h"
#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "skia/ext/skia_utils_mac.h"

@interface ModalDelegate : NSObject {
 @private
  atom::MessageBoxCallback callback_;
  NSAlert* alert_;
  bool callEndModal_;
}
- (id)initWithCallback:(const atom::MessageBoxCallback&)callback
              andAlert:(NSAlert*)alert
          callEndModal:(bool)flag;
@end

@implementation ModalDelegate

- (id)initWithCallback:(const atom::MessageBoxCallback&)callback
              andAlert:(NSAlert*)alert
          callEndModal:(bool)flag {
  if ((self = [super init])) {
    callback_ = callback;
    alert_ = alert;
    callEndModal_ = flag;
  }
  return self;
}

- (void)alertDidEnd:(NSAlert*)alert
         returnCode:(NSInteger)returnCode
        contextInfo:(void*)contextInfo {
  callback_.Run(returnCode);
  [alert_ release];
  [self release];

  if (callEndModal_)
    [NSApp stopModal];
}

@end

namespace atom {

namespace {

NSAlert* CreateNSAlert(NativeWindow* parent_window,
                       MessageBoxType type,
                       const std::vector<std::string>& buttons,
                       int default_id,
                       const std::string& title,
                       const std::string& message,
                       const std::string& detail,
                       const gfx::ImageSkia& icon) {
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
    // An empty title causes crash on macOS.
    if (buttons[i].empty())
      title = @"(empty)";
    NSButton* button = [alert addButtonWithTitle:title];
    [button setTag:i];
  }

  NSArray* ns_buttons = [alert buttons];
  if (default_id >= 0 && default_id < static_cast<int>([ns_buttons count])) {
    // Focus the button at default_id if the user opted to do so.
    // The first button added gets set as the default selected.
    // So remove that default, and make the requested button the default.
    [[ns_buttons objectAtIndex:0] setKeyEquivalent:@""];
    [[ns_buttons objectAtIndex:default_id] setKeyEquivalent:@"\r"];
  }

  if (!icon.isNull()) {
    NSImage* image = skia::SkBitmapToNSImageWithColorSpace(
        *icon.bitmap(), base::mac::GetGenericRGBColorSpace());
    [alert setIcon:image];
  }

  return alert;
}

void SetReturnCode(int* ret_code, int result) {
  *ret_code = result;
}

}  // namespace

int ShowMessageBox(NativeWindow* parent_window,
                   MessageBoxType type,
                   const std::vector<std::string>& buttons,
                   int default_id,
                   int cancel_id,
                   int options,
                   const std::string& title,
                   const std::string& message,
                   const std::string& detail,
                   const gfx::ImageSkia& icon) {
  NSAlert* alert = CreateNSAlert(
      parent_window, type, buttons, default_id, title, message,
      detail, icon);

  // Use runModal for synchronous alert without parent, since we don't have a
  // window to wait for.
  if (!parent_window || !parent_window->GetNativeWindow())
    return [[alert autorelease] runModal];

  int ret_code = -1;
  ModalDelegate* delegate = [[ModalDelegate alloc]
      initWithCallback:base::Bind(&SetReturnCode, &ret_code)
              andAlert:alert
          callEndModal:true];

  NSWindow* window = parent_window->GetNativeWindow();
  [alert beginSheetModalForWindow:window
                    modalDelegate:delegate
                   didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:)
                      contextInfo:nil];

  [NSApp runModalForWindow:window];
  return ret_code;
}

void ShowMessageBox(NativeWindow* parent_window,
                    MessageBoxType type,
                    const std::vector<std::string>& buttons,
                    int default_id,
                    int cancel_id,
                    int options,
                    const std::string& title,
                    const std::string& message,
                    const std::string& detail,
                    const gfx::ImageSkia& icon,
                    const MessageBoxCallback& callback) {
  NSAlert* alert = CreateNSAlert(
      parent_window, type, buttons, default_id, title, message,
      detail, icon);
  ModalDelegate* delegate = [[ModalDelegate alloc] initWithCallback:callback
                                                           andAlert:alert
                                                       callEndModal:false];

  NSWindow* window = parent_window ? parent_window->GetNativeWindow() : nil;
  [alert beginSheetModalForWindow:window
                    modalDelegate:delegate
                   didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:)
                      contextInfo:nil];
}

void ShowErrorBox(const base::string16& title, const base::string16& content) {
  NSAlert* alert = [[NSAlert alloc] init];
  [alert setMessageText:base::SysUTF16ToNSString(title)];
  [alert setInformativeText:base::SysUTF16ToNSString(content)];
  [alert setAlertStyle:NSWarningAlertStyle];
  [alert runModal];
  [alert release];
}

}  // namespace atom
