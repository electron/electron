// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/menu_bar.h"

#include <memory>

#include "atom/browser/ui/views/menu_delegate.h"
#include "atom/browser/ui/views/submenu_button.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/background.h"
#include "ui/views/layout/box_layout.h"

#if defined(USE_X11)
#include "chrome/browser/ui/libgtkui/gtk_util.h"
#endif

#if defined(OS_WIN)
#include "ui/gfx/color_utils.h"
#endif

namespace atom {

namespace {

// Default color of the menu bar.
const SkColor kDefaultColor = SkColorSetARGB(255, 233, 233, 233);

}  // namespace

const char MenuBar::kViewClassName[] = "ElectronMenuBar";

MenuBar::MenuBar(views::View* window)
    : background_color_(kDefaultColor), window_(window) {
  RefreshColorCache();
  UpdateViewColors();
  SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal));
  window_->GetFocusManager()->AddFocusChangeListener(this);
}

MenuBar::~MenuBar() {
  window_->GetFocusManager()->RemoveFocusChangeListener(this);
}

void MenuBar::SetMenu(AtomMenuModel* model) {
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
                                           AtomMenuModel** menu_model,
                                           views::MenuButton** button) {
  if (!GetBoundsInScreen().Contains(screenPoint))
    return false;

  auto children = GetChildrenInZOrder();
  for (int i = 0, n = children.size(); i < n; ++i) {
    if (children[i]->GetBoundsInScreen().Contains(screenPoint) &&
        (menu_model_->GetTypeAt(i) == AtomMenuModel::TYPE_SUBMENU)) {
      *menu_model = menu_model_->GetSubmenuModelAt(i);
      *button = static_cast<views::MenuButton*>(children[i]);
      return true;
    }
  }

  return false;
}

const char* MenuBar::GetClassName() const {
  return kViewClassName;
}

void MenuBar::OnMenuButtonClicked(views::MenuButton* source,
                                  const gfx::Point& point,
                                  const ui::Event* event) {
  // Hide the accelerator when a submenu is activated.
  SetAcceleratorVisibility(false);

  if (!menu_model_)
    return;

  if (!window_->HasFocus())
    window_->RequestFocus();

  int id = source->tag();
  AtomMenuModel::ItemType type = menu_model_->GetTypeAt(id);
  if (type != AtomMenuModel::TYPE_SUBMENU) {
    menu_model_->ActivatedAt(id, 0);
    return;
  }

  // Deleted in MenuDelegate::OnMenuClosed
  MenuDelegate* menu_delegate = new MenuDelegate(this);
  menu_delegate->RunMenu(menu_model_->GetSubmenuModelAt(id), source);
}

void MenuBar::RefreshColorCache(const ui::NativeTheme* theme) {
  if (!theme)
    theme = ui::NativeTheme::GetInstanceForNativeUi();
  if (theme) {
#if defined(USE_X11)
    background_color_ = libgtkui::GetBgColor("GtkMenuBar#menubar");
    enabled_color_ = libgtkui::GetFgColor(
        "GtkMenuBar#menubar GtkMenuItem#menuitem GtkLabel");
    disabled_color_ = libgtkui::GetFgColor(
        "GtkMenuBar#menubar GtkMenuItem#menuitem:disabled GtkLabel");
#else
    background_color_ =
        theme->GetSystemColor(ui::NativeTheme::kColorId_MenuBackgroundColor);
#endif
  }
#if defined(OS_WIN)
  background_color_ = color_utils::GetSysSkColor(COLOR_MENUBAR);
#endif
}

void MenuBar::OnNativeThemeChanged(const ui::NativeTheme* theme) {
  RefreshColorCache(theme);
  UpdateViewColors();
}

void MenuBar::OnDidChangeFocus(View* focused_before, View* focused_now) {
  // if we've changed focus, update our view
  const auto had_focus = has_focus_;
  has_focus_ = focused_now != nullptr;
  if (has_focus_ != had_focus)
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
    auto button = static_cast<SubmenuButton*>(child);
    button->SetUnderlineColor(color_utils::GetSysSkColor(COLOR_MENUTEXT));
  }
#endif
}

}  // namespace atom
