// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "shell/browser/api/electron_api_menu_mac.h"

#include <string>
#include <utility>

#include "base/mac/scoped_sending_event.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/current_thread.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/native_window.h"
#include "shell/browser/unresponsive_suppressor.h"
#include "shell/common/keyboard_util.h"
#include "shell/common/node_includes.h"

namespace {

static scoped_nsobject<NSMenu> applicationMenu_;

ui::Accelerator GetAcceleratorFromKeyEquivalentAndModifierMask(
    NSString* key_equivalent,
    NSUInteger modifier_mask) {
  absl::optional<char16_t> shifted_char;
  ui::KeyboardCode code = electron::KeyboardCodeFromStr(
      base::SysNSStringToUTF8(key_equivalent), &shifted_char);
  int modifiers = 0;
  if (modifier_mask & NSEventModifierFlagShift)
    modifiers |= ui::EF_SHIFT_DOWN;
  if (modifier_mask & NSEventModifierFlagControl)
    modifiers |= ui::EF_CONTROL_DOWN;
  if (modifier_mask & NSEventModifierFlagOption)
    modifiers |= ui::EF_ALT_DOWN;
  if (modifier_mask & NSEventModifierFlagCommand)
    modifiers |= ui::EF_COMMAND_DOWN;
  return ui::Accelerator(code, modifiers);
}

}  // namespace

namespace electron {

namespace api {

MenuMac::MenuMac(gin::Arguments* args) : Menu(args) {}

MenuMac::~MenuMac() = default;

void MenuMac::PopupAt(BaseWindow* window,
                      int x,
                      int y,
                      int positioning_item,
                      base::OnceClosure callback) {
  NativeWindow* native_window = window->window();
  if (!native_window)
    return;

  // Make sure the Menu object would not be garbage-collected until the callback
  // has run.
  base::OnceClosure callback_with_ref = BindSelfToClosure(std::move(callback));

  auto popup =
      base::BindOnce(&MenuMac::PopupOnUI, weak_factory_.GetWeakPtr(),
                     native_window->GetWeakPtr(), window->weak_map_id(), x, y,
                     positioning_item, std::move(callback_with_ref));
  base::SequencedTaskRunnerHandle::Get()->PostTask(FROM_HERE, std::move(popup));
}

v8::Local<v8::Value> Menu::GetUserAcceleratorAt(int command_id) const {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  if (![NSMenuItem usesUserKeyEquivalents])
    return v8::Null(isolate);

  auto controller = base::scoped_nsobject<ElectronMenuController>(
      [[ElectronMenuController alloc] initWithModel:model()
                              useDefaultAccelerator:NO]);

  int command_index = GetIndexOfCommandId(command_id);
  if (command_index == -1)
    return v8::Null(isolate);

  base::scoped_nsobject<NSMenuItem> item =
      [controller makeMenuItemForIndex:command_index fromModel:model()];
  if ([[item userKeyEquivalent] length] == 0)
    return v8::Null(isolate);

  NSString* user_key_equivalent = [item keyEquivalent];
  NSUInteger user_modifier_mask = [item keyEquivalentModifierMask];
  ui::Accelerator accelerator = GetAcceleratorFromKeyEquivalentAndModifierMask(
      user_key_equivalent, user_modifier_mask);

  return gin::ConvertToV8(isolate, accelerator.GetShortcutText());
}

void MenuMac::PopupOnUI(const base::WeakPtr<NativeWindow>& native_window,
                        int32_t window_id,
                        int x,
                        int y,
                        int positioning_item,
                        base::OnceClosure callback) {
  if (!native_window)
    return;
  NSWindow* nswindow = native_window->GetNativeWindow().GetNativeNSWindow();

  base::OnceClosure close_callback =
      base::BindOnce(&MenuMac::OnClosed, weak_factory_.GetWeakPtr(), window_id,
                     std::move(callback));
  popup_controllers_[window_id] = base::scoped_nsobject<ElectronMenuController>(
      [[ElectronMenuController alloc] initWithModel:model()
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
  if (!item) {
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

  [popup_controllers_[window_id] setCloseCallback:std::move(close_callback)];
  // Make sure events can be pumped while the menu is up.
  base::CurrentThread::ScopedNestableTaskAllower allow;

  // One of the events that could be pumped is |window.close()|.
  // User-initiated event-tracking loops protect against this by
  // setting flags in -[CrApplication sendEvent:], but since
  // web-content menus are initiated by IPC message the setup has to
  // be done manually.
  base::mac::ScopedSendingEvent sendingEventScoper;

  // Don't emit unresponsive event when showing menu.
  electron::UnresponsiveSuppressor suppressor;
  [menu popUpMenuPositioningItem:item atLocation:position inView:view];
}

void MenuMac::ClosePopupAt(int32_t window_id) {
  auto close_popup = base::BindOnce(&MenuMac::ClosePopupOnUI,
                                    weak_factory_.GetWeakPtr(), window_id);
  base::SequencedTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                   std::move(close_popup));
}

std::u16string MenuMac::GetAcceleratorTextAtForTesting(int index) const {
  // A least effort to get the real shortcut text of NSMenuItem, the code does
  // not need to be perfect since it is test only.
  base::scoped_nsobject<ElectronMenuController> controller(
      [[ElectronMenuController alloc] initWithModel:model()
                              useDefaultAccelerator:NO]);
  NSMenuItem* item = [[controller menu] itemAtIndex:index];
  std::u16string text;
  NSEventModifierFlags modifiers = [item keyEquivalentModifierMask];
  if (modifiers & NSEventModifierFlagControl)
    text += u"Ctrl";
  if (modifiers & NSEventModifierFlagShift) {
    if (!text.empty())
      text += u"+";
    text += u"Shift";
  }
  if (modifiers & NSEventModifierFlagOption) {
    if (!text.empty())
      text += u"+";
    text += u"Alt";
  }
  if (modifiers & NSEventModifierFlagCommand) {
    if (!text.empty())
      text += u"+";
    text += u"Command";
  }
  if (!text.empty())
    text += u"+";
  auto key = base::ToUpperASCII(base::SysNSStringToUTF16([item keyEquivalent]));
  if (key == u"\t")
    text += u"Tab";
  else
    text += key;
  return text;
}

void MenuMac::ClosePopupOnUI(int32_t window_id) {
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

void MenuMac::OnClosed(int32_t window_id, base::OnceClosure callback) {
  popup_controllers_.erase(window_id);
  std::move(callback).Run();
}

// static
void Menu::SetApplicationMenu(Menu* base_menu) {
  MenuMac* menu = static_cast<MenuMac*>(base_menu);
  base::scoped_nsobject<ElectronMenuController> menu_controller(
      [[ElectronMenuController alloc] initWithModel:menu->model_.get()
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
gin::Handle<Menu> Menu::New(gin::Arguments* args) {
  auto handle =
      gin::CreateHandle(args->isolate(), static_cast<Menu*>(new MenuMac(args)));
  gin_helper::CallMethod(args->isolate(), handle.get(), "_init");
  return handle;
}

}  // namespace api

}  // namespace electron
