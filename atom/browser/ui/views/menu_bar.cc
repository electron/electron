// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/menu_bar.h"

#if defined(USE_X11)
#include "gtk/gtk.h"
#endif

#include "atom/browser/ui/views/menu_delegate.h"
#include "atom/browser/ui/views/submenu_button.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/background.h"
#include "ui/views/layout/box_layout.h"

#if defined(OS_WIN)
#include "ui/gfx/color_utils.h"
#elif defined(USE_X11)
#include "chrome/browser/ui/libgtk2ui/skia_utils_gtk2.h"
#endif

namespace atom {

namespace {

const char kViewClassName[] = "ElectronMenuBar";

// Default color of the menu bar.
const SkColor kDefaultColor = SkColorSetARGB(255, 233, 233, 233);

#if defined(USE_X11)
void GetMenuBarColor(SkColor* enabled, SkColor* disabled, SkColor* highlight,
                     SkColor* hover, SkColor* background) {
  GtkWidget* menu_bar = gtk_menu_bar_new();

  GtkStyle* style = gtk_rc_get_style(menu_bar);
  *enabled = libgtk2ui::GdkColorToSkColor(style->fg[GTK_STATE_NORMAL]);
  *disabled = libgtk2ui::GdkColorToSkColor(style->fg[GTK_STATE_INSENSITIVE]);
  *highlight = libgtk2ui::GdkColorToSkColor(style->fg[GTK_STATE_SELECTED]);
  *hover = libgtk2ui::GdkColorToSkColor(style->fg[GTK_STATE_PRELIGHT]);
  *background = libgtk2ui::GdkColorToSkColor(style->bg[GTK_STATE_NORMAL]);

  gtk_widget_destroy(menu_bar);
}
#endif

}  // namespace

MenuBar::MenuBar()
    : background_color_(kDefaultColor),
      menu_model_(NULL) {
  UpdateMenuBarColor();
  SetLayoutManager(new views::BoxLayout(
      views::BoxLayout::kHorizontal, 0, 0, 0));
}

MenuBar::~MenuBar() {
}

void MenuBar::SetMenu(ui::MenuModel* model) {
  menu_model_ = model;
  RemoveAllChildViews(true);

  for (int i = 0; i < model->GetItemCount(); ++i) {
    SubmenuButton* button = new SubmenuButton(this, model->GetLabelAt(i), this);
    button->set_tag(i);

#if defined(USE_X11)
    button->SetTextColor(views::Button::STATE_NORMAL, enabled_color_);
    button->SetTextColor(views::Button::STATE_DISABLED, disabled_color_);
    button->SetTextColor(views::Button::STATE_PRESSED, highlight_color_);
    button->SetTextColor(views::Button::STATE_HOVERED, hover_color_);
    button->SetUnderlineColor(enabled_color_);
#elif defined(OS_WIN)
    button->SetUnderlineColor(color_utils::GetSysSkColor(COLOR_GRAYTEXT));
#endif

    AddChildView(button);
  }
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
                                           ui::MenuModel** menu_model,
                                           views::MenuButton** button) {
  gfx::Point location(point);
  views::View::ConvertPointFromScreen(this, &location);

  if (location.x() < 0 || location.x() >= width() || location.y() < 0 ||
      location.y() >= height())
    return false;

  for (int i = 0; i < child_count(); ++i) {
    views::View* view = child_at(i);
    if (view->bounds().Contains(location) &&
        (menu_model_->GetTypeAt(i) == ui::MenuModel::TYPE_SUBMENU)) {
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

void MenuBar::ButtonPressed(views::Button* sender, const ui::Event& event) {
}

void MenuBar::OnMenuButtonClicked(views::MenuButton* source,
                                  const gfx::Point& point,
                                  const ui::Event* event) {
  // Hide the accelerator when a submenu is activated.
  SetAcceleratorVisibility(false);

  if (!menu_model_)
    return;

  int id = source->tag();
  ui::MenuModel::ItemType type = menu_model_->GetTypeAt(id);
  if (type != ui::MenuModel::TYPE_SUBMENU)
    return;

  MenuDelegate menu_delegate(this);
  menu_delegate.RunMenu(menu_model_->GetSubmenuModelAt(id), source);
}

void MenuBar::OnNativeThemeChanged(const ui::NativeTheme* theme) {
  UpdateMenuBarColor();
}

void MenuBar::UpdateMenuBarColor() {
#if defined(OS_WIN)
  background_color_ = color_utils::GetSysSkColor(COLOR_MENUBAR);
#elif defined(USE_X11)
  GetMenuBarColor(&enabled_color_, &disabled_color_, &highlight_color_,
                  &hover_color_, &background_color_);
#endif
  set_background(views::Background::CreateSolidBackground(background_color_));
}

}  // namespace atom
