// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/menu_delegate.h"

#include "atom/browser/ui/views/menu_bar.h"
#include "base/stl_util.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/widget/widget.h"

namespace atom {

MenuDelegate::MenuDelegate(MenuBar* menu_bar)
    : menu_bar_(menu_bar),
      id_(-1),
      items_(menu_bar_->GetItemCount()),
      delegates_(menu_bar_->GetItemCount()) {
}

MenuDelegate::~MenuDelegate() {
  STLDeleteElements(&delegates_);
}

void MenuDelegate::RunMenu(ui::MenuModel* model, views::MenuButton* button) {
  gfx::Point screen_loc;
  views::View::ConvertPointToScreen(button, &screen_loc);
  // Subtract 1 from the height to make the popup flush with the button border.
  gfx::Rect bounds(screen_loc.x(), screen_loc.y(), button->width(),
                   button->height() - 1);

  id_ = button->tag();
  views::MenuItemView* item = BuildMenu(model);

  views::MenuRunner menu_runner(
      item,
      views::MenuRunner::CONTEXT_MENU | views::MenuRunner::HAS_MNEMONICS);
  ignore_result(menu_runner.RunMenuAt(
      button->GetWidget()->GetTopLevelWidget(),
      button,
      bounds,
      views::MENU_ANCHOR_TOPRIGHT,
      ui::MENU_SOURCE_MOUSE));
}

views::MenuItemView* MenuDelegate::BuildMenu(ui::MenuModel* model) {
  DCHECK_GE(id_, 0);

  if (!items_[id_]) {
    views::MenuModelAdapter* delegate = new views::MenuModelAdapter(model);
    delegates_[id_] = delegate;

    views::MenuItemView* item = new views::MenuItemView(this);
    delegate->BuildMenu(item);
    items_[id_] = item;
  }

  return items_[id_];
}

void MenuDelegate::ExecuteCommand(int id) {
  delegate()->ExecuteCommand(id);
}

void MenuDelegate::ExecuteCommand(int id, int mouse_event_flags) {
  delegate()->ExecuteCommand(id, mouse_event_flags);
}

bool MenuDelegate::IsTriggerableEvent(views::MenuItemView* source,
                                      const ui::Event& e) {
  return delegate()->IsTriggerableEvent(source, e);
}

bool MenuDelegate::GetAccelerator(int id, ui::Accelerator* accelerator) const {
  return delegate()->GetAccelerator(id, accelerator);
}

base::string16 MenuDelegate::GetLabel(int id) const {
  return delegate()->GetLabel(id);
}

const gfx::FontList* MenuDelegate::GetLabelFontList(int id) const {
  return delegate()->GetLabelFontList(id);
}

bool MenuDelegate::IsCommandEnabled(int id) const {
  return delegate()->IsCommandEnabled(id);
}

bool MenuDelegate::IsCommandVisible(int id) const {
  return delegate()->IsCommandVisible(id);
}

bool MenuDelegate::IsItemChecked(int id) const {
  return delegate()->IsItemChecked(id);
}

void MenuDelegate::SelectionChanged(views::MenuItemView* menu) {
  delegate()->SelectionChanged(menu);
}

void MenuDelegate::WillShowMenu(views::MenuItemView* menu) {
  delegate()->WillShowMenu(menu);
}

void MenuDelegate::WillHideMenu(views::MenuItemView* menu) {
  delegate()->WillHideMenu(menu);
}

views::MenuItemView* MenuDelegate::GetSiblingMenu(
    views::MenuItemView* menu,
    const gfx::Point& screen_point,
    views::MenuAnchorPosition* anchor,
    bool* has_mnemonics,
    views::MenuButton** button) {
  ui::MenuModel* model;
  if (!menu_bar_->GetMenuButtonFromScreenPoint(screen_point, &model, button))
    return NULL;

  *anchor = views::MENU_ANCHOR_TOPLEFT;
  *has_mnemonics = true;

  id_ = (*button)->tag();
  return BuildMenu(model);
}

}  // namespace atom
