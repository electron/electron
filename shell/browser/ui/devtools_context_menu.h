// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_DEVTOOLS_CONTEXT_MENU_H_
#define ELECTRON_SHELL_BROWSER_UI_DEVTOOLS_CONTEXT_MENU_H_

#include <memory>
#include <vector>

#include "base/containers/flat_map.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/blink/public/mojom/context_menu/context_menu.mojom-forward.h"
#include "ui/menus/simple_menu_model.h"

namespace content {
class WebContents;
}  // namespace content

namespace views {
class MenuRunner;
class Widget;
}  // namespace views

namespace electron {

// Shows a native menu for a context menu request originating from a DevTools
// frontend (InspectorFrontendHost.showContextMenuAtPoint). The menu is built
// from the Blink-provided items in |params.custom_items| and selections are
// reported back to the frontend through the standard custom context menu
// plumbing (DevToolsAPI.contextMenuItemSelected / contextMenuCleared).
// Observes the DevTools WebContents so the menu closes, without touching the
// dying WebContents, if it is destroyed while the menu is open.
class DevToolsContextMenu : private content::WebContentsObserver,
                            private ui::SimpleMenuModel::Delegate {
 public:
  DevToolsContextMenu(content::WebContents* devtools_web_contents,
                      const content::ContextMenuParams& params);
  ~DevToolsContextMenu() override;

  // disable copy
  DevToolsContextMenu(const DevToolsContextMenu&) = delete;
  DevToolsContextMenu& operator=(const DevToolsContextMenu&) = delete;

  // Shows the menu anchored to |parent_widget|. Note that on macOS this is a
  // blocking call while the native menu is open.
  void RunMenuAt(views::Widget* parent_widget);

 private:
  // Standard edit commands, shown when DevTools requests a context menu over
  // an editable element without providing any custom items. Custom item
  // actions are bounded by Blink's DevToolsHost::kMaxContextMenuAction
  // (1000), so these ids can never collide with them.
  enum EditCommands {
    kEditCommandUndo = 10000,
    kEditCommandRedo,
    kEditCommandCut,
    kEditCommandCopy,
    kEditCommandPaste,
    kEditCommandPasteAndMatchStyle,
    kEditCommandDelete,
    kEditCommandSelectAll,
  };

  void AppendCustomItems(
      const std::vector<blink::mojom::CustomContextMenuItemPtr>& items,
      ui::SimpleMenuModel* model,
      size_t depth,
      size_t* total_items);
  void AppendEditItems(ui::SimpleMenuModel* model);

  void OnMenuClosed();

  void WebContentsDestroyed() override;

  // ui::SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

  struct ItemState {
    bool enabled = true;
    bool checked = false;
  };

  content::ContextMenuParams params_;

  bool menu_open_ = false;
  base::flat_map<int, ItemState> item_states_;
  ui::SimpleMenuModel menu_model_{this};
  std::vector<std::unique_ptr<ui::SimpleMenuModel>> submenu_models_;
  std::unique_ptr<views::MenuRunner> menu_runner_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_DEVTOOLS_CONTEXT_MENU_H_
