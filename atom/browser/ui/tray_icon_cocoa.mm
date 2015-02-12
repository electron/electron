// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/tray_icon_cocoa.h"

#include "atom/browser/ui/cocoa/atom_menu_controller.h"
#include "base/strings/sys_string_conversions.h"
#include "ui/gfx/image/image.h"

@interface StatusItemController : NSObject {
  atom::TrayIconCocoa* trayIcon_; // weak
}
- (id)initWithIcon:(atom::TrayIconCocoa*)icon;
- (void)handleClick:(id)sender;
- (void)handleDoubleClick:(id)sender;

@end // @interface StatusItemController

@implementation StatusItemController

- (id)initWithIcon:(atom::TrayIconCocoa*)icon {
  trayIcon_ = icon;
  return self;
}

- (void)handleClick:(id)sender {
  trayIcon_->NotifyClicked();
}

- (void)handleDoubleClick:(id)sender {
  trayIcon_->NotifyDoubleClicked();
}

@end

namespace atom {

TrayIconCocoa::TrayIconCocoa() {
  controller_.reset([[StatusItemController alloc] initWithIcon:this]);

  item_.reset([[[NSStatusBar systemStatusBar]
                statusItemWithLength:NSVariableStatusItemLength] retain]);
  [item_ setEnabled:YES];
  [item_ setTarget:controller_];
  [item_ setAction:@selector(handleClick:)];
  [item_ setDoubleAction:@selector(handleDoubleClick:)];
  [item_ setHighlightMode:YES];
}

TrayIconCocoa::~TrayIconCocoa() {
  // Remove the status item from the status bar.
  [[NSStatusBar systemStatusBar] removeStatusItem:item_];
}

void TrayIconCocoa::SetImage(const gfx::Image& image) {
  [item_ setImage:image.AsNSImage()];
}

void TrayIconCocoa::SetPressedImage(const gfx::Image& image) {
  [item_ setAlternateImage:image.AsNSImage()];
}

void TrayIconCocoa::SetToolTip(const std::string& tool_tip) {
  [item_ setToolTip:base::SysUTF8ToNSString(tool_tip)];
}

void TrayIconCocoa::SetTitle(const std::string& title) {
  [item_ setTitle:base::SysUTF8ToNSString(title)];
}

void TrayIconCocoa::SetHighlightMode(bool highlight) {
  [item_ setHighlightMode:highlight];
}

void TrayIconCocoa::SetContextMenu(ui::SimpleMenuModel* menu_model) {
  menu_.reset([[AtomMenuController alloc] initWithModel:menu_model]);
  [item_ setMenu:[menu_ menu]];
}

// static
TrayIcon* TrayIcon::Create() {
  return new TrayIconCocoa;
}

}  // namespace atom
