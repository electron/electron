// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "atom/browser/api/atom_api_menu_mac.h"

#include "atom/browser/native_window.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

MenuMac::MenuMac() {
}

void MenuMac::Popup(Window* window) {
  base::scoped_nsobject<AtomMenuController> menu_controller(
      [[AtomMenuController alloc] initWithModel:model_.get()]);

  NativeWindow* native_window = window->window();
  NSWindow* nswindow = native_window->GetNativeWindow();
  content::WebContents* web_contents = native_window->GetWebContents();

  // Fake out a context menu event.
  NSEvent* currentEvent = [NSApp currentEvent];
  NSPoint position = [nswindow mouseLocationOutsideOfEventStream];
  NSTimeInterval eventTime = [currentEvent timestamp];
  NSEvent* clickEvent = [NSEvent mouseEventWithType:NSRightMouseDown
                                           location:position
                                      modifierFlags:NSRightMouseDownMask
                                          timestamp:eventTime
                                       windowNumber:[nswindow windowNumber]
                                            context:nil
                                        eventNumber:0
                                         clickCount:1
                                           pressure:1.0];

  // Show the menu.
  [NSMenu popUpContextMenu:[menu_controller menu]
                 withEvent:clickEvent
                   forView:web_contents->GetView()->GetContentNativeView()];
}

// static
void Menu::SetApplicationMenu(Menu* base_menu) {
  MenuMac* menu = static_cast<MenuMac*>(base_menu);
  base::scoped_nsobject<AtomMenuController> menu_controller(
      [[AtomMenuController alloc] initWithModel:menu->model_.get()]);
  [NSApp setMainMenu:[menu_controller menu]];

  // Ensure the menu_controller_ is destroyed after main menu is set.
  menu_controller.swap(menu->menu_controller_);
}

// static
void Menu::SendActionToFirstResponder(const std::string& action) {
  SEL selector = NSSelectorFromString(base::SysUTF8ToNSString(action));
  [NSApp sendAction:selector to:nil from:[NSApp mainMenu]];
}

// static
mate::Wrappable* Menu::Create() {
  return new MenuMac();
}

}  // namespace api

}  // namespace atom
