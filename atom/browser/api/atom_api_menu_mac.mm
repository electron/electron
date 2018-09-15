// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/api/atom_api_menu_mac.h"

#include "atom/browser/native_window.h"
#include "atom/browser/unresponsive_suppressor.h"
#include "base/mac/scoped_sending_event.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"

#include "atom/common/node_includes.h"

using content::BrowserThread;

namespace {

static scoped_nsobject<NSMenu> applicationMenu_;

}  // namespace

namespace atom {

namespace api {

MenuMac::MenuMac(v8::Isolate* isolate, v8::Local<v8::Object> wrapper)
    : Menu(isolate, wrapper), weak_factory_(this) {}

MenuMac::~MenuMac() = default;

void MenuMac::PopupAt(TopLevelWindow* window,
                      int x,
                      int y,
                      int positioning_item,
                      const base::Closure& callback) {
  NativeWindow* native_window = window->window();
  if (!native_window)
    return;

  auto popup = base::Bind(&MenuMac::PopupOnUI, weak_factory_.GetWeakPtr(),
                          native_window->GetWeakPtr(), window->weak_map_id(), x,
                          y, positioning_item, callback);
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, popup);
}

void MenuMac::PopupOnUI(const base::WeakPtr<NativeWindow>& native_window,
                        int32_t window_id,
                        int x,
                        int y,
                        int positioning_item,
                        base::Closure callback) {
  if (!native_window)
    return;
  NSWindow* nswindow = native_window->GetNativeWindow();

  auto close_callback = base::Bind(
      &MenuMac::OnClosed, weak_factory_.GetWeakPtr(), window_id, callback);
  popup_controllers_[window_id] = base::scoped_nsobject<AtomMenuController>([
      [AtomMenuController alloc] initWithModel:model()
                         useDefaultAccelerator:NO]);
  NSMenu* menu = [popup_controllers_[window_id] menu];
  NSView* view = [nswindow contentView];

  // Which menu item to show.
  NSMenuItem* item = nil;
  if (positioning_item < [menu numberOfItems] && positioning_item >= 0)
    item = [menu itemAtIndex:positioning_item];

  // (-1, -1) means showing on mouse location.
  NSPoint position;
  if (x == -1 || y == -1) {
    position = [view convertPoint:[nswindow mouseLocationOutsideOfEventStream]
                         fromView:nil];
  } else {
    position = NSMakePoint(x, [view frame].size.height - y);
  }

  // If no preferred item is specified, try to show all of the menu items.
  if (!positioning_item) {
    CGFloat windowBottom = CGRectGetMinY([view window].frame);
    CGFloat lowestMenuPoint = windowBottom + position.y - [menu size].height;
    CGFloat screenBottom = CGRectGetMinY([view window].screen.frame);
    CGFloat distanceFromBottom = lowestMenuPoint - screenBottom;
    if (distanceFromBottom < 0)
      position.y = position.y - distanceFromBottom + 4;
  }

  // Place the menu left of cursor if it is overflowing off right of screen.
  CGFloat windowLeft = CGRectGetMinX([view window].frame);
  CGFloat rightmostMenuPoint = windowLeft + position.x + [menu size].width;
  CGFloat screenRight = CGRectGetMaxX([view window].screen.frame);
  if (rightmostMenuPoint > screenRight)
    position.x = position.x - [menu size].width;

  [popup_controllers_[window_id] setCloseCallback:close_callback];
  // Make sure events can be pumped while the menu is up.
  base::MessageLoop::ScopedNestableTaskAllower allow;

  // One of the events that could be pumped is |window.close()|.
  // User-initiated event-tracking loops protect against this by
  // setting flags in -[CrApplication sendEvent:], but since
  // web-content menus are initiated by IPC message the setup has to
  // be done manually.
  base::mac::ScopedSendingEvent sendingEventScoper;

  // Don't emit unresponsive event when showing menu.
  atom::UnresponsiveSuppressor suppressor;
  [menu popUpMenuPositioningItem:item atLocation:position inView:view];
}

void MenuMac::ClosePopupAt(int32_t window_id) {
  auto controller = popup_controllers_.find(window_id);
  if (controller != popup_controllers_.end()) {
    // Close the controller for the window.
    [controller->second cancel];
  } else if (window_id == -1) {
    // Or just close all opened controllers.
    for (auto it = popup_controllers_.begin();
         it != popup_controllers_.end();) {
      // The iterator is invalidated after the call.
      [(it++)->second cancel];
    }
  }
}

void MenuMac::OnClosed(int32_t window_id, base::Closure callback) {
  popup_controllers_.erase(window_id);
  callback.Run();
}

// static
void Menu::SetApplicationMenu(Menu* base_menu) {
  MenuMac* menu = static_cast<MenuMac*>(base_menu);
  base::scoped_nsobject<AtomMenuController> menu_controller([
      [AtomMenuController alloc] initWithModel:menu->model_.get()
                         useDefaultAccelerator:YES]);

  NSRunLoop* currentRunLoop = [NSRunLoop currentRunLoop];
  [currentRunLoop cancelPerformSelector:@selector(setMainMenu:)
                                 target:NSApp
                               argument:applicationMenu_];
  applicationMenu_.reset([[menu_controller menu] retain]);
  [[NSRunLoop currentRunLoop]
      performSelector:@selector(setMainMenu:)
               target:NSApp
             argument:applicationMenu_
                order:0
                modes:[NSArray arrayWithObject:NSDefaultRunLoopMode]];

  // Ensure the menu_controller_ is destroyed after main menu is set.
  menu_controller.swap(menu->menu_controller_);
}

// static
void Menu::SendActionToFirstResponder(const std::string& action) {
  SEL selector = NSSelectorFromString(base::SysUTF8ToNSString(action));
  [NSApp sendAction:selector to:nil from:[NSApp mainMenu]];
}

// static
mate::WrappableBase* Menu::New(mate::Arguments* args) {
  return new MenuMac(args->isolate(), args->GetThis());
}

}  // namespace api

}  // namespace atom
