// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/menu_delegate.h"

#include "atom/browser/ui/views/menu_bar.h"
#include "atom/browser/ui/views/menu_model_adapter.h"
#include "content/public/browser/browser_thread.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/widget/widget.h"

namespace atom {

MenuDelegate::MenuDelegate(MenuBar* menu_bar)
    : menu_bar_(menu_bar),
      id_(-1) {
}

MenuDelegate::~MenuDelegate() {
}

void MenuDelegate::RunMenu(AtomMenuModel* model, views::MenuButton* button) {
  gfx::Point screen_loc;
  views::View::ConvertPointToScreen(button, &screen_loc);
  // Subtract 1 from the height to make the popup flush with the button border.
  gfx::Rect bounds(screen_loc.x(), screen_loc.y(), button->width(),
                   button->height() - 1);

  id_ = button->tag();
  adapter_.reset(new atom::MenuModelAdapter(model));

  views::MenuItemView* item = new views::MenuItemView(this);
  static_cast<atom::MenuModelAdapter*>(adapter_.get())->BuildMenu(item);

  menu_runner_.reset(new views::MenuRunner(
      item,
      views::MenuRunner::CONTEXT_MENU | views::MenuRunner::HAS_MNEMONICS));
  ignore_result(menu_runner_->RunMenuAt(
      button->GetWidget()->GetTopLevelWidget(),
      button,
      bounds,
      views::MENU_ANCHOR_TOPRIGHT,
      ui::MENU_SOURCE_MOUSE));
}

void MenuDelegate::ExecuteCommand(int id) {
  adapter_->ExecuteCommand(id);
}

void MenuDelegate::ExecuteCommand(int id, int mouse_event_flags) {
  adapter_->ExecuteCommand(id, mouse_event_flags);
}

bool MenuDelegate::IsTriggerableEvent(views::MenuItemView* source,
                                      const ui::Event& e) {
  return adapter_->IsTriggerableEvent(source, e);
}

bool MenuDelegate::GetAccelerator(int id, ui::Accelerator* accelerator) const {
  return adapter_->GetAccelerator(id, accelerator);
}

base::string16 MenuDelegate::GetLabel(int id) const {
  return adapter_->GetLabel(id);
}

const gfx::FontList* MenuDelegate::GetLabelFontList(int id) const {
  return adapter_->GetLabelFontList(id);
}

bool MenuDelegate::IsCommandEnabled(int id) const {
  return adapter_->IsCommandEnabled(id);
}

bool MenuDelegate::IsCommandVisible(int id) const {
  return adapter_->IsCommandVisible(id);
}

bool MenuDelegate::IsItemChecked(int id) const {
  return adapter_->IsItemChecked(id);
}

void MenuDelegate::SelectionChanged(views::MenuItemView* menu) {
  adapter_->SelectionChanged(menu);
}

void MenuDelegate::WillShowMenu(views::MenuItemView* menu) {
  adapter_->WillShowMenu(menu);
}

void MenuDelegate::WillHideMenu(views::MenuItemView* menu) {
  adapter_->WillHideMenu(menu);
}

views::MenuItemView* MenuDelegate::GetSiblingMenu(
    views::MenuItemView* menu,
    const gfx::Point& screen_point,
    views::MenuAnchorPosition* anchor,
    bool* has_mnemonics,
    views::MenuButton**) {
  views::MenuButton* button;
  AtomMenuModel* model;
  if (menu_bar_->GetMenuButtonFromScreenPoint(screen_point, &model, &button) &&
      button->tag() != id_) {
    DCHECK(menu_runner_->IsRunning());
    menu_runner_->Cancel();
    // After canceling the menu, we need to wait until next tick
    // so we are out of nested message loop.
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::Bind(base::IgnoreResult(&views::MenuButton::Activate),
                   base::Unretained(button), nullptr));
  }

  return nullptr;
}

}  // namespace atom
