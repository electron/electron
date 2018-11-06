// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/root_view.h"

#include "atom/browser/native_window.h"
#include "atom/browser/ui/views/menu_bar.h"
#include "content/public/browser/native_web_keyboard_event.h"

namespace atom {

namespace {

// The menu bar height in pixels.
#if defined(OS_WIN)
const int kMenuBarHeight = 20;
#else
const int kMenuBarHeight = 25;
#endif

bool IsAltKey(const content::NativeWebKeyboardEvent& event) {
  return event.windows_key_code == ui::VKEY_MENU;
}

bool IsAltModifier(const content::NativeWebKeyboardEvent& event) {
  typedef content::NativeWebKeyboardEvent::Modifiers Modifiers;
  int modifiers = event.GetModifiers();
  modifiers &= ~Modifiers::kNumLockOn;
  modifiers &= ~Modifiers::kCapsLockOn;
  return (modifiers == Modifiers::kAltKey) ||
         (modifiers == (Modifiers::kAltKey | Modifiers::kIsLeft)) ||
         (modifiers == (Modifiers::kAltKey | Modifiers::kIsRight));
}

}  // namespace

RootView::RootView(NativeWindow* window) : window_(window) {
  set_owned_by_client();
}

RootView::~RootView() {}

void RootView::SetMenu(AtomMenuModel* menu_model) {
  if (menu_model == nullptr) {
    // Remove accelerators
    UnregisterAcceleratorsWithFocusManager();
    // and menu bar.
    SetMenuBarVisibility(false);
    menu_bar_.reset();
    return;
  }

  RegisterAcceleratorsWithFocusManager(menu_model);

  // Do not show menu bar in frameless window.
  if (!window_->has_frame())
    return;

  if (!menu_bar_) {
    menu_bar_.reset(new MenuBar(this));
    menu_bar_->set_owned_by_client();
    if (!menu_bar_autohide_)
      SetMenuBarVisibility(true);
  }

  menu_bar_->SetMenu(menu_model);
  Layout();
}

bool RootView::HasMenu() const {
  return !!menu_bar_;
}

int RootView::GetMenuBarHeight() const {
  return kMenuBarHeight;
}

void RootView::SetAutoHideMenuBar(bool auto_hide) {
  menu_bar_autohide_ = auto_hide;
}

bool RootView::IsMenuBarAutoHide() const {
  return menu_bar_autohide_;
}

void RootView::SetMenuBarVisibility(bool visible) {
  if (!window_->content_view() || !menu_bar_ || menu_bar_visible_ == visible)
    return;

  // Always show the accelerator when the auto-hide menu bar shows.
  if (menu_bar_autohide_)
    menu_bar_->SetAcceleratorVisibility(visible);

  menu_bar_visible_ = visible;
  if (visible) {
    DCHECK_EQ(child_count(), 1);
    AddChildView(menu_bar_.get());
  } else {
    DCHECK_EQ(child_count(), 2);
    RemoveChildView(menu_bar_.get());
  }

  Layout();
}

bool RootView::IsMenuBarVisible() const {
  return menu_bar_visible_;
}

void RootView::HandleKeyEvent(const content::NativeWebKeyboardEvent& event) {
  if (!menu_bar_)
    return;

  // Show accelerator when "Alt" is pressed.
  if (menu_bar_visible_ && IsAltKey(event))
    menu_bar_->SetAcceleratorVisibility(event.GetType() ==
                                        blink::WebInputEvent::kRawKeyDown);

  // Show the submenu when "Alt+Key" is pressed.
  if (event.GetType() == blink::WebInputEvent::kRawKeyDown &&
      !IsAltKey(event) && IsAltModifier(event)) {
    if (!menu_bar_visible_ &&
        (menu_bar_->HasAccelerator(event.windows_key_code)))
      SetMenuBarVisibility(true);
    menu_bar_->ActivateAccelerator(event.windows_key_code);
    return;
  }

  if (!menu_bar_autohide_)
    return;

  // Toggle the menu bar only when a single Alt is released.
  if (event.GetType() == blink::WebInputEvent::kRawKeyDown && IsAltKey(event)) {
    // When a single Alt is pressed:
    menu_bar_alt_pressed_ = true;
  } else if (event.GetType() == blink::WebInputEvent::kKeyUp &&
             IsAltKey(event) && menu_bar_alt_pressed_) {
    // When a single Alt is released right after a Alt is pressed:
    menu_bar_alt_pressed_ = false;
    SetMenuBarVisibility(!menu_bar_visible_);
  } else {
    // When any other keys except single Alt have been pressed/released:
    menu_bar_alt_pressed_ = false;
  }
}

void RootView::ResetAltState() {
  menu_bar_alt_pressed_ = false;
}

void RootView::Layout() {
  if (!window_->content_view())  // Not ready yet.
    return;

  const auto menu_bar_bounds =
      menu_bar_visible_ ? gfx::Rect(0, 0, size().width(), kMenuBarHeight)
                        : gfx::Rect();
  if (menu_bar_)
    menu_bar_->SetBoundsRect(menu_bar_bounds);

  window_->content_view()->SetBoundsRect(
      gfx::Rect(0, menu_bar_bounds.height(), size().width(),
                size().height() - menu_bar_bounds.height()));
}

gfx::Size RootView::GetMinimumSize() const {
  return window_->GetMinimumSize();
}

gfx::Size RootView::GetMaximumSize() const {
  return window_->GetMaximumSize();
}

bool RootView::AcceleratorPressed(const ui::Accelerator& accelerator) {
  return accelerator_util::TriggerAcceleratorTableCommand(&accelerator_table_,
                                                          accelerator);
}

void RootView::RegisterAcceleratorsWithFocusManager(AtomMenuModel* menu_model) {
  if (!menu_model)
    return;
  // Clear previous accelerators.
  UnregisterAcceleratorsWithFocusManager();
  views::FocusManager* focus_manager = GetFocusManager();
  // Register accelerators with focus manager.
  accelerator_util::GenerateAcceleratorTable(&accelerator_table_, menu_model);
  for (const auto& iter : accelerator_table_) {
    focus_manager->RegisterAccelerator(
        iter.first, ui::AcceleratorManager::kNormalPriority, this);
  }
}

void RootView::UnregisterAcceleratorsWithFocusManager() {
  views::FocusManager* focus_manager = GetFocusManager();
  accelerator_table_.clear();
  focus_manager->UnregisterAccelerators(this);
}

}  // namespace atom
