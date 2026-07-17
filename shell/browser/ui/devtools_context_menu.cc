// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/devtools_context_menu.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/context_menu_data/edit_flags.h"
#include "third_party/blink/public/mojom/context_menu/context_menu.mojom.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/models/menu_separator_types.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace electron {

namespace {

// Match the limits enforced by Chrome's RenderViewContextMenuBase.
constexpr size_t kMaxCustomMenuDepth = 5;
constexpr size_t kMaxCustomMenuTotalItems = 1000;

}  // namespace

DevToolsContextMenu::DevToolsContextMenu(
    content::WebContents* devtools_web_contents,
    const content::ContextMenuParams& params)
    : web_contents_(devtools_web_contents), params_(params) {
  if (!params_.custom_items.empty()) {
    size_t total_items = 0;
    AppendCustomItems(params_.custom_items, &menu_model_, 0, &total_items);
  } else if (params_.is_editable) {
    AppendEditItems(&menu_model_);
  }
}

DevToolsContextMenu::~DevToolsContextMenu() {
  // If the menu is being torn down while still open (e.g. DevTools is
  // closing), restore the WebContents' context menu state. The menu runner
  // cancels the native menu in its own destructor.
  OnMenuClosed();
}

void DevToolsContextMenu::RunMenuAt(views::Widget* parent_widget) {
  if (menu_model_.GetItemCount() == 0) {
    // Nothing to show; let the frontend clean up its pending menu state.
    web_contents_->NotifyContextMenuClosed(params_.link_followed);
    return;
  }

  // Block further context menu requests and renderer mouse-move handling
  // while the menu is showing, like Chrome's RenderViewContextMenuBase does.
  menu_open_ = true;
  web_contents_->SetShowingContextMenu(true);

  menu_runner_ = std::make_unique<views::MenuRunner>(
      &menu_model_,
      views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU,
      base::BindRepeating(&DevToolsContextMenu::OnMenuClosed,
                          base::Unretained(this)));

  // |params_| coordinates are relative to the DevTools view's origin; convert
  // them to screen coordinates for the menu runner.
  gfx::Point screen_point{params_.x, params_.y};
  if (content::RenderWidgetHostView* view =
          web_contents_->GetRenderWidgetHostView())
    screen_point += view->GetViewBounds().OffsetFromOrigin();

  menu_runner_->RunMenuAt(
      parent_widget, nullptr, gfx::Rect{screen_point, gfx::Size{}},
      views::MenuAnchorPosition::kTopLeft, params_.source_type);
}

void DevToolsContextMenu::AppendCustomItems(
    const std::vector<blink::mojom::CustomContextMenuItemPtr>& items,
    ui::SimpleMenuModel* model,
    size_t depth,
    size_t* total_items) {
  if (depth > kMaxCustomMenuDepth)
    return;

  for (const auto& item : items) {
    if (*total_items >= kMaxCustomMenuTotalItems)
      return;
    ++*total_items;

    const int command_id = item->action;
    switch (item->type) {
      case blink::mojom::CustomContextMenuItemType::kOption:
        model->AddItem(command_id, item->label);
        if (item->accelerator) {
          model->SetAcceleratorAt(
              model->GetItemCount() - 1,
              ui::Accelerator{
                  static_cast<ui::KeyboardCode>(item->accelerator->key_code),
                  item->accelerator->modifiers});
        }
        break;
      case blink::mojom::CustomContextMenuItemType::kCheckableOption:
        model->AddCheckItem(command_id, item->label);
        break;
      case blink::mojom::CustomContextMenuItemType::kGroup:
        // Not produced by the DevTools frontend.
        break;
      case blink::mojom::CustomContextMenuItemType::kSeparator:
        model->AddSeparator(ui::NORMAL_SEPARATOR);
        continue;
      case blink::mojom::CustomContextMenuItemType::kSubMenu: {
        auto submenu = std::make_unique<ui::SimpleMenuModel>(
            static_cast<ui::SimpleMenuModel::Delegate*>(this));
        AppendCustomItems(item->submenu, submenu.get(), depth + 1, total_items);
        model->AddSubMenu(command_id, item->label, submenu.get());
        submenu_models_.push_back(std::move(submenu));
        break;
      }
    }

    item_states_.insert_or_assign(command_id,
                                  ItemState{item->enabled, item->checked});
  }
}

void DevToolsContextMenu::AppendEditItems(ui::SimpleMenuModel* model) {
  const auto add_item = [&](int command_id, const char16_t* label,
                            int required_flag) {
    model->AddItem(command_id, label);
    item_states_.insert_or_assign(
        command_id,
        ItemState{(params_.edit_flags & required_flag) != 0, false});
  };

  add_item(kEditCommandUndo, u"Undo",
           blink::ContextMenuDataEditFlags::kCanUndo);
  add_item(kEditCommandRedo, u"Redo",
           blink::ContextMenuDataEditFlags::kCanRedo);
  model->AddSeparator(ui::NORMAL_SEPARATOR);
  add_item(kEditCommandCut, u"Cut", blink::ContextMenuDataEditFlags::kCanCut);
  add_item(kEditCommandCopy, u"Copy",
           blink::ContextMenuDataEditFlags::kCanCopy);
  add_item(kEditCommandPaste, u"Paste",
           blink::ContextMenuDataEditFlags::kCanPaste);
  add_item(kEditCommandPasteAndMatchStyle, u"Paste and Match Style",
           blink::ContextMenuDataEditFlags::kCanPaste);
  add_item(kEditCommandDelete, u"Delete",
           blink::ContextMenuDataEditFlags::kCanDelete);
  model->AddSeparator(ui::NORMAL_SEPARATOR);
  add_item(kEditCommandSelectAll, u"Select All",
           blink::ContextMenuDataEditFlags::kCanSelectAll);
}

void DevToolsContextMenu::OnMenuClosed() {
  if (!menu_open_)
    return;
  menu_open_ = false;
  web_contents_->SetShowingContextMenu(false);
  web_contents_->NotifyContextMenuClosed(params_.link_followed);
}

bool DevToolsContextMenu::IsCommandIdChecked(int command_id) const {
  if (const auto iter = item_states_.find(command_id);
      iter != item_states_.end())
    return iter->second.checked;
  return false;
}

bool DevToolsContextMenu::IsCommandIdEnabled(int command_id) const {
  if (const auto iter = item_states_.find(command_id);
      iter != item_states_.end())
    return iter->second.enabled;
  return true;
}

void DevToolsContextMenu::ExecuteCommand(int command_id, int event_flags) {
  switch (command_id) {
    case kEditCommandUndo:
      web_contents_->Undo();
      return;
    case kEditCommandRedo:
      web_contents_->Redo();
      return;
    case kEditCommandCut:
      web_contents_->Cut();
      return;
    case kEditCommandCopy:
      web_contents_->Copy();
      return;
    case kEditCommandPaste:
      web_contents_->Paste();
      return;
    case kEditCommandPasteAndMatchStyle:
      web_contents_->PasteAndMatchStyle();
      return;
    case kEditCommandDelete:
      web_contents_->Delete();
      return;
    case kEditCommandSelectAll:
      web_contents_->SelectAll();
      return;
    default:
      // A custom DevTools item: the frontend receives the action as
      // DevToolsAPI.contextMenuItemSelected(action).
      web_contents_->ExecuteCustomContextMenuCommand(command_id,
                                                     params_.link_followed);
      return;
  }
}

}  // namespace electron
