// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/menu_bar.h"

#include <memory>
#include <set>
#include <sstream>

#include "shell/browser/ui/views/submenu_button.h"
#include "shell/common/keyboard_util.h"
#include "ui/aura/window.h"
#include "ui/base/models/menu_model.h"
#include "ui/native_theme/common_theme.h"
#include "ui/views/background.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

#if defined(USE_X11)
#include "ui/gtk/gtk_util.h"
#endif

#if defined(OS_WIN)
#include "ui/gfx/color_utils.h"
#endif

namespace electron {

namespace {

// Default color of the menu bar.
const SkColor kDefaultColor = SkColorSetARGB(255, 233, 233, 233);

}  // namespace

const char MenuBar::kViewClassName[] = "ElectronMenuBar";

MenuBarColorUpdater::MenuBarColorUpdater(MenuBar* menu_bar)
    : menu_bar_(menu_bar) {}

MenuBarColorUpdater::~MenuBarColorUpdater() = default;

void MenuBarColorUpdater::OnDidChangeFocus(views::View* focused_before,
                                           views::View* focused_now) {
  if (menu_bar_) {
    // if we've changed window focus, update menu bar colors
    const auto had_focus = menu_bar_->has_focus_;
    menu_bar_->has_focus_ = focused_now != nullptr;
    if (menu_bar_->has_focus_ != had_focus)
      menu_bar_->UpdateViewColors();
  }
}

MenuBar::MenuBar(RootView* window)
    : background_color_(kDefaultColor),
      window_(window),
      color_updater_(new MenuBarColorUpdater(this)) {
  RefreshColorCache();
  UpdateViewColors();
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal));
  window_->GetFocusManager()->AddFocusChangeListener(color_updater_.get());
}

MenuBar::~MenuBar() {
  window_->GetFocusManager()->RemoveFocusChangeListener(color_updater_.get());
}

void MenuBar::SetMenu(ElectronMenuModel* model) {
  menu_model_ = model;
  RebuildChildren();
}

void MenuBar::SetAcceleratorVisibility(bool visible) {
  for (auto* child : GetChildrenInZOrder())
    static_cast<SubmenuButton*>(child)->SetAcceleratorVisibility(visible);
}

MenuBar::View* MenuBar::FindAccelChild(base::char16 key) {
  for (auto* child : GetChildrenInZOrder()) {
    if (static_cast<SubmenuButton*>(child)->accelerator() == key)
      return child;
  }
  return nullptr;
}

bool MenuBar::HasAccelerator(base::char16 key) {
  return FindAccelChild(key) != nullptr;
}

void MenuBar::ActivateAccelerator(base::char16 key) {
  auto* child = FindAccelChild(key);
  if (child)
    static_cast<SubmenuButton*>(child)->Activate(nullptr);
}

int MenuBar::GetItemCount() const {
  return menu_model_ ? menu_model_->GetItemCount() : 0;
}

bool MenuBar::GetMenuButtonFromScreenPoint(const gfx::Point& screenPoint,
                                           ElectronMenuModel** menu_model,
                                           views::MenuButton** button) {
  if (!GetBoundsInScreen().Contains(screenPoint))
    return false;

  auto children = GetChildrenInZOrder();
  for (int i = 0, n = children.size(); i < n; ++i) {
    if (children[i]->GetBoundsInScreen().Contains(screenPoint) &&
        (menu_model_->GetTypeAt(i) == ElectronMenuModel::TYPE_SUBMENU)) {
      *menu_model = menu_model_->GetSubmenuModelAt(i);
      *button = static_cast<views::MenuButton*>(children[i]);
      return true;
    }
  }

  return false;
}

void MenuBar::OnBeforeExecuteCommand() {
  if (GetPaneFocusTraversable() != nullptr) {
    RemovePaneFocus();
  }
  window_->RestoreFocus();
}

void MenuBar::OnMenuClosed() {
  SetAcceleratorVisibility(true);
}

bool MenuBar::AcceleratorPressed(const ui::Accelerator& accelerator) {
  views::View* focused_view = GetFocusManager()->GetFocusedView();
  if (!ContainsForFocusSearch(this, focused_view))
    return false;

  switch (accelerator.key_code()) {
    case ui::VKEY_MENU:
    case ui::VKEY_ESCAPE: {
      RemovePaneFocus();
      window_->RestoreFocus();
      return true;
    }
    case ui::VKEY_LEFT:
      GetFocusManager()->AdvanceFocus(true);
      return true;
    case ui::VKEY_RIGHT:
      GetFocusManager()->AdvanceFocus(false);
      return true;
    case ui::VKEY_HOME:
      GetFocusManager()->SetFocusedViewWithReason(
          GetFirstFocusableChild(),
          views::FocusManager::FocusChangeReason::kFocusTraversal);
      return true;
    case ui::VKEY_END:
      GetFocusManager()->SetFocusedViewWithReason(
          GetLastFocusableChild(),
          views::FocusManager::FocusChangeReason::kFocusTraversal);
      return true;
    default: {
      for (auto* child : GetChildrenInZOrder()) {
        auto* button = static_cast<SubmenuButton*>(child);
        bool shifted = false;
        auto keycode =
            electron::KeyboardCodeFromCharCode(button->accelerator(), &shifted);

        if (keycode == accelerator.key_code()) {
          auto event = accelerator.ToKeyEvent();
          ButtonPressed(button, event);
          return true;
        }
      }

      return false;
    }
  }
}

bool MenuBar::SetPaneFocus(views::View* initial_focus) {
  // TODO(zcbenz): Submit patch to upstream Chromium to fix the crash.
  //
  // Without this check, Electron would crash when running tests.
  //
  // Check failed: rules_->CanFocusWindow(window, nullptr).
  //   logging::LogMessage::~LogMessage
  //   wm::FocusController::SetFocusedWindow
  //   wm::FocusController::ResetFocusWithinActiveWindow
  //   views::View::OnFocus
  //   views::Button::OnFocus
  //   views::LabelButton::OnFocus
  //   views::View::Focus
  //   views::FocusManager::SetFocusedViewWithReason
  //   views::AccessiblePaneView::SetPaneFocus
  //   electron::MenuBar::SetPaneFocus
  if (initial_focus && initial_focus->GetWidget()) {
    aura::Window* window = initial_focus->GetWidget()->GetNativeWindow();
    if (!window || !window->GetRootWindow())
      return false;
  }

  bool result = views::AccessiblePaneView::SetPaneFocus(initial_focus);

  if (result) {
    std::set<ui::KeyboardCode> reg;
    for (auto* child : GetChildrenInZOrder()) {
      auto* button = static_cast<SubmenuButton*>(child);
      bool shifted = false;
      auto keycode =
          electron::KeyboardCodeFromCharCode(button->accelerator(), &shifted);

      // We want the menu items to activate if the user presses the accelerator
      // key, even without alt, since we are now focused on the menu bar
      if (keycode != ui::VKEY_UNKNOWN && reg.find(keycode) != reg.end()) {
        reg.insert(keycode);
        focus_manager()->RegisterAccelerator(
            ui::Accelerator(keycode, ui::EF_NONE),
            ui::AcceleratorManager::kNormalPriority, this);
      }
    }

    // We want to remove focus / hide menu bar when alt is pressed again
    focus_manager()->RegisterAccelerator(
        ui::Accelerator(ui::VKEY_MENU, ui::EF_ALT_DOWN),
        ui::AcceleratorManager::kNormalPriority, this);
  }

  return result;
}

void MenuBar::RemovePaneFocus() {
  views::AccessiblePaneView::RemovePaneFocus();
  SetAcceleratorVisibility(false);

  std::set<ui::KeyboardCode> unreg;
  for (auto* child : GetChildrenInZOrder()) {
    auto* button = static_cast<SubmenuButton*>(child);
    bool shifted = false;
    auto keycode =
        electron::KeyboardCodeFromCharCode(button->accelerator(), &shifted);

    if (keycode != ui::VKEY_UNKNOWN && unreg.find(keycode) != unreg.end()) {
      unreg.insert(keycode);
      focus_manager()->UnregisterAccelerator(
          ui::Accelerator(keycode, ui::EF_NONE), this);
    }
  }

  focus_manager()->UnregisterAccelerator(
      ui::Accelerator(ui::VKEY_MENU, ui::EF_ALT_DOWN), this);
}

const char* MenuBar::GetClassName() const {
  return kViewClassName;
}

void MenuBar::ButtonPressed(views::Button* source, const ui::Event& event) {
  // Hide the accelerator when a submenu is activated.
  SetAcceleratorVisibility(false);

  if (!menu_model_)
    return;

  if (!window_->HasFocus())
    window_->RequestFocus();

  int id = source->tag();
  ElectronMenuModel::ItemType type = menu_model_->GetTypeAt(id);
  if (type != ElectronMenuModel::TYPE_SUBMENU) {
    menu_model_->ActivatedAt(id, 0);
    return;
  }

  // Deleted in MenuDelegate::OnMenuClosed
  MenuDelegate* menu_delegate = new MenuDelegate(this);
  menu_delegate->RunMenu(
      menu_model_->GetSubmenuModelAt(id), source,
      event.IsKeyEvent() ? ui::MENU_SOURCE_KEYBOARD : ui::MENU_SOURCE_MOUSE);
  menu_delegate->AddObserver(this);
}

void MenuBar::RefreshColorCache() {
  const ui::NativeTheme* theme = GetNativeTheme();
  if (theme) {
#if defined(USE_X11)
    background_color_ = gtk::GetBgColor("GtkMenuBar#menubar");
    enabled_color_ =
        gtk::GetFgColor("GtkMenuBar#menubar GtkMenuItem#menuitem GtkLabel");
    disabled_color_ = gtk::GetFgColor(
        "GtkMenuBar#menubar GtkMenuItem#menuitem:disabled GtkLabel");
#else
    background_color_ =
        ui::GetAuraColor(ui::NativeTheme::kColorId_MenuBackgroundColor, theme);
#endif
  }
}

void MenuBar::OnThemeChanged() {
  views::AccessiblePaneView::OnThemeChanged();
  RefreshColorCache();
  UpdateViewColors();
}

void MenuBar::RebuildChildren() {
  RemoveAllChildViews(true);
  for (int i = 0, n = GetItemCount(); i < n; ++i) {
    auto* button =
        new SubmenuButton(menu_model_->GetLabelAt(i), this, background_color_);
    button->set_tag(i);
    AddChildView(button);
  }
  UpdateViewColors();
}

void MenuBar::UpdateViewColors() {
  // set menubar background color
  SetBackground(views::CreateSolidBackground(background_color_));

  // set child colors
  if (menu_model_ == nullptr)
    return;
#if defined(USE_X11)
  const auto& textColor = has_focus_ ? enabled_color_ : disabled_color_;
  for (auto* child : GetChildrenInZOrder()) {
    auto* button = static_cast<SubmenuButton*>(child);
    button->SetTextColor(views::Button::STATE_NORMAL, textColor);
    button->SetTextColor(views::Button::STATE_DISABLED, disabled_color_);
    button->SetTextColor(views::Button::STATE_PRESSED, enabled_color_);
    button->SetTextColor(views::Button::STATE_HOVERED, textColor);
    button->SetUnderlineColor(textColor);
  }
#elif defined(OS_WIN)
  for (auto* child : GetChildrenInZOrder()) {
    auto* button = static_cast<SubmenuButton*>(child);
    button->SetUnderlineColor(color_utils::GetSysSkColor(COLOR_MENUTEXT));
  }
#endif
}

}  // namespace electron
