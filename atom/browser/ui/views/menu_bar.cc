// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/menu_bar.h"

#include "atom/browser/ui/views/menu_delegate.h"
#include "atom/browser/ui/views/submenu_button.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/background.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/layout/box_layout.h"

#if defined(OS_WIN)
#include "ui/gfx/color_utils.h"
#endif

namespace atom {

namespace {

const char kViewClassName[] = "ElectronMenuBar";

// Default color of the menu bar.
const SkColor kDefaultColor = SkColorSetARGB(255, 233, 233, 233);

}  // namespace

MenuBar::MenuBar(NativeWindow* window)
    : background_color_(kDefaultColor), menu_model_(NULL), window_(window) {
  UpdateColorCache();
  UpdateMenuBarView();
  SetLayoutManager(new views::BoxLayout(views::BoxLayout::kHorizontal));
}

MenuBar::~MenuBar() {
  auto fm = GetFocusManager();
  if (fm)
    fm->RemoveFocusChangeListener(this);
}

void MenuBar::ViewHierarchyChanged(const ViewHierarchyChangedDetails& details) {
  // The focus manager is tied to our parent view.
  // So when we change parents, keep our FocusChange listening in sync.
  if ((details.child == this) && (details.parent != nullptr)) {
    auto fm = details.parent->GetFocusManager();
    if (details.is_add)
      fm->AddFocusChangeListener(this);
    else
      fm->RemoveFocusChangeListener(this);
  }
}

void MenuBar::SetMenu(AtomMenuModel* model) {
  menu_model_ = model;
  UpdateMenuBarView();
}

void MenuBar::SetAcceleratorVisibility(bool visible) {
  for (int i = 0; i < child_count(); ++i)
    static_cast<SubmenuButton*>(child_at(i))->SetAcceleratorVisibility(visible);
}

int MenuBar::GetAcceleratorIndex(base::char16 key) {
  for (int i = 0; i < child_count(); ++i) {
    SubmenuButton* button = static_cast<SubmenuButton*>(child_at(i));
    if (button->accelerator() == key)
      return i;
  }
  return -1;
}

void MenuBar::ActivateAccelerator(base::char16 key) {
  int i = GetAcceleratorIndex(key);
  if (i != -1)
    static_cast<SubmenuButton*>(child_at(i))->Activate(nullptr);
}

int MenuBar::GetItemCount() const {
  return menu_model_->GetItemCount();
}

bool MenuBar::GetMenuButtonFromScreenPoint(const gfx::Point& point,
                                           AtomMenuModel** menu_model,
                                           views::MenuButton** button) {
  gfx::Point location(point);
  views::View::ConvertPointFromScreen(this, &location);

  if (location.x() < 0 || location.x() >= width() || location.y() < 0 ||
      location.y() >= height())
    return false;

  for (int i = 0; i < child_count(); ++i) {
    views::View* view = child_at(i);
    if (view->GetMirroredBounds().Contains(location) &&
        (menu_model_->GetTypeAt(i) == AtomMenuModel::TYPE_SUBMENU)) {
      *menu_model = menu_model_->GetSubmenuModelAt(i);
      *button = static_cast<views::MenuButton*>(view);
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

  if (!window_->IsFocused())
    window_->Focus(true);

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

void MenuBar::UpdateColorCache(const ui::NativeTheme* theme) {
  if (!theme)
    theme = ui::NativeTheme::GetInstanceForNativeUi();
  if (theme) {
#if defined(USE_X11)
    enabled_color_ = theme->GetSystemColor(
        ui::NativeTheme::kColorId_EnabledMenuItemForegroundColor);
    disabled_color_ = theme->GetSystemColor(
        ui::NativeTheme::kColorId_DisabledMenuItemForegroundColor);
#endif
    background_color_ =
        theme->GetSystemColor(ui::NativeTheme::kColorId_MenuBackgroundColor);
  }
#if defined(OS_WINDOWS)
  background_color_ = color_utils::GetSysSkColor(COLOR_MENUBAR);
#endif
}

void MenuBar::OnNativeThemeChanged(const ui::NativeTheme* theme) {
  UpdateColorCache(theme);
  UpdateMenuBarView();
}

void MenuBar::OnDidChangeFocus(View* focused_before, View* focused_now) {
  // if we've changed focus, update our view
  const auto had_focus = has_focus_;
  has_focus_ = focused_now != nullptr;
  if (has_focus_ != had_focus)
    UpdateMenuBarView();
}

void MenuBar::UpdateMenuBarView() {
  // set menubar background color
  SetBackground(views::CreateSolidBackground(background_color_));

  // set child colors
  RemoveAllChildViews(true);
  if (menu_model_ != nullptr) {
    const auto textColor = has_focus_ ? enabled_color_ : disabled_color_;
    for (int i = 0; i < menu_model_->GetItemCount(); ++i) {
      auto button = new SubmenuButton(menu_model_->GetLabelAt(i), this,
                                      background_color_);
      button->set_tag(i);
#if defined(USE_X11)
      button->SetTextColor(views::Button::STATE_NORMAL, textColor);
      button->SetTextColor(views::Button::STATE_DISABLED, disabled_color_);
      button->SetTextColor(views::Button::STATE_PRESSED, enabled_color_);
      button->SetTextColor(views::Button::STATE_HOVERED, enabled_color_);
      button->SetUnderlineColor(textColor);
#elif defined(OS_WIN)
      button->SetUnderlineColor(color_utils::GetSysSkColor(COLOR_GRAYTEXT));
#endif
      AddChildView(button);
    }
  }
}

}  // namespace atom
