// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/api/atom_api_menu_mac.h"

#include "atom/browser/native_window.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "content/public/browser/web_contents.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

MenuMac::MenuMac() {
}

void MenuMac::Destroy() {
  menu_controller_.reset();
  Menu::Destroy();
}

void MenuMac::Popup(Window* window) {
  NativeWindow* native_window = window->window();
  if (!native_window)
    return;
  content::WebContents* web_contents = native_window->web_contents();
  if (!web_contents)
    return;

  NSWindow* nswindow = native_window->GetNativeWindow();
  base::scoped_nsobject<AtomMenuController> menu_controller(
      [[AtomMenuController alloc] initWithModel:model_.get()]);

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
                   forView:web_contents->GetContentNativeView()];
}

void MenuMac::PopupAt(Window* window, int x, int y) {
  NativeWindow* native_window = window->window();
  if (!native_window)
    return;
  content::WebContents* web_contents = native_window->web_contents();
  if (!web_contents)
    return;

  base::scoped_nsobject<AtomMenuController> menu_controller(
      [[AtomMenuController alloc] initWithModel:model_.get()]);
  NSMenu* menu = [menu_controller menu];
  NSView* view = web_contents->GetContentNativeView();

  // Show the menu.
  [menu popUpMenuPositioningItem:[menu itemAtIndex:0]
                      atLocation:NSMakePoint(x, [view frame].size.height - y)
                          inView:view];
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
