// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/root_view.h"

#include "content/public/browser/native_web_keyboard_event.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/views/menu_bar.h"
#include "shell/browser/ui/views/title_bar.h"
#include "ui/base/hit_test.h"

namespace electron {

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

RootView::RootView(NativeWindow* window)
    : window_(window),
      last_focused_view_tracker_(std::make_unique<views::ViewTracker>()) {
  set_owned_by_client();
  window_->AddObserver(this);
  if (window_->has_custom_frame()) {
    title_bar_.reset(new TitleBar(this));
    title_bar_->set_owned_by_client();
    AddChildView(title_bar_.get());
  }
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

  menu_bar_visible_ = visible;
  if (visible) {
    AddChildView(menu_bar_.get());
  } else {
    RemoveChildView(menu_bar_.get());
  }

  Layout();
}

bool RootView::IsMenuBarVisible() const {
  return menu_bar_ && menu_bar_visible_;
}

void RootView::SetTitleBarVisibility(bool visible) {
  if (!window_->content_view() || !title_bar_ || title_bar_visible_ == visible)
    return;

  title_bar_visible_ = visible;
  if (visible) {
    AddChildView(title_bar_.get());
  } else {
    RemoveChildView(title_bar_.get());
  }

  Layout();
}

bool RootView::IsTitleBarVisible() const {
  return title_bar_ && title_bar_visible_;
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
    if (menu_bar_->HasAccelerator(event.windows_key_code)) {
      if (!menu_bar_visible_) {
        SetMenuBarVisibility(true);

        View* focused_view = GetFocusManager()->GetFocusedView();
        last_focused_view_tracker_->SetView(focused_view);
        menu_bar_->RequestFocus();
      }

      menu_bar_->ActivateAccelerator(event.windows_key_code);
    }
    return;
  }

  // Toggle the menu bar only when a single Alt is released.
  if (event.GetType() == blink::WebInputEvent::kRawKeyDown && IsAltKey(event)) {
    // When a single Alt is pressed:
    menu_bar_alt_pressed_ = true;
  } else if (event.GetType() == blink::WebInputEvent::kKeyUp &&
             IsAltKey(event) && menu_bar_alt_pressed_) {
    // When a single Alt is released right after a Alt is pressed:
    menu_bar_alt_pressed_ = false;
    if (menu_bar_autohide_)
      SetMenuBarVisibility(!menu_bar_visible_);

    View* focused_view = GetFocusManager()->GetFocusedView();
    last_focused_view_tracker_->SetView(focused_view);
    if (menu_bar_visible_) {
      menu_bar_->RequestFocus();
      // Show accelerators when menu bar is focused
      menu_bar_->SetAcceleratorVisibility(true);
    }
  } else {
    // When any other keys except single Alt have been pressed/released:
    menu_bar_alt_pressed_ = false;
  }
}

void RootView::RestoreFocus() {
  View* last_focused_view = last_focused_view_tracker_->view();
  if (last_focused_view) {
    GetFocusManager()->SetFocusedViewWithReason(
        last_focused_view, views::FocusManager::kReasonFocusRestore);
  }
  if (menu_bar_autohide_)
    SetMenuBarVisibility(false);
}

void RootView::ResetAltState() {
  menu_bar_alt_pressed_ = false;
}

void RootView::Layout() {
  if (!window_->content_view())  // Not ready yet.
    return;

  gfx::Point top_left(insets_.left(), insets_.top());
  int width = size().width() - insets_.width();

  const auto title_bar_bounds =
      IsTitleBarVisible()
          ? gfx::Rect(top_left.x(), top_left.y(), width, title_bar_->Height())
          : gfx::Rect();
  top_left.Offset(0, title_bar_bounds.height());
  if (title_bar_)
    title_bar_->SetBoundsRect(title_bar_bounds);

  const auto menu_bar_bounds =
      IsMenuBarVisible()
          ? gfx::Rect(top_left.x(), top_left.y(), width, kMenuBarHeight)
          : gfx::Rect();
  top_left.Offset(0, menu_bar_bounds.height());
  if (menu_bar_)
    menu_bar_->SetBoundsRect(menu_bar_bounds);

  window_->content_view()->SetBoundsRect(gfx::Rect(
      top_left.x(), top_left.y(), width, size().height() - top_left.y()));
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

void RootView::OnWindowEnterFullScreen() {
  if (title_bar_)
    SetTitleBarVisibility(false);
}

void RootView::OnWindowLeaveFullScreen() {
  if (title_bar_)
    SetTitleBarVisibility(true);
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

void RootView::SetInsets(const gfx::Insets& insets) {
  if (insets != insets_) {
    insets_ = insets;
    Layout();
  }
}

int RootView::NonClientHitTest(const gfx::Point& point) {
  if (title_bar_) {
    return title_bar_->NonClientHitTest(point);
  }

  return HTNOWHERE;
}

void RootView::UpdateWindowIcon() {
  if (title_bar_)
    title_bar_->UpdateWindowIcon();
}

void RootView::UpdateWindowTitle() {
  if (title_bar_)
    title_bar_->UpdateWindowTitle();
}

}  // namespace electron
