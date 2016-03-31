// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_UI_TRAY_ICON_COCOA_H_
#define ELECTRON_BROWSER_UI_TRAY_ICON_COCOA_H_

#import <Cocoa/Cocoa.h>

#include <string>

#include "electron/browser/ui/atom_menu_model.h"
#include "electron/browser/ui/tray_icon.h"
#include "base/mac/scoped_nsobject.h"

@class AtomMenuController;
@class StatusItemView;

namespace atom {

class TrayIconCocoa : public TrayIcon,
                      public AtomMenuModel::Observer {
 public:
  TrayIconCocoa();
  virtual ~TrayIconCocoa();

  void SetImage(const gfx::Image& image) override;
  void SetPressedImage(const gfx::Image& image) override;
  void SetToolTip(const std::string& tool_tip) override;
  void SetTitle(const std::string& title) override;
  void SetHighlightMode(bool highlight) override;
  void PopUpContextMenu(const gfx::Point& pos,
                        ui::SimpleMenuModel* menu_model) override;
  void SetContextMenu(ui::SimpleMenuModel* menu_model) override;

 protected:
  // AtomMenuModel::Observer:
  void MenuClosed() override;

 private:
  // Atom custom view for NSStatusItem.
  base::scoped_nsobject<StatusItemView> status_item_view_;

  // Status menu shown when right-clicking the system icon.
  base::scoped_nsobject<AtomMenuController> menu_;

  // Used for unregistering observer.
  AtomMenuModel* menu_model_;  // weak ref.

  DISALLOW_COPY_AND_ASSIGN(TrayIconCocoa);
};

}  // namespace atom

#endif  // ELECTRON_BROWSER_UI_TRAY_ICON_COCOA_H_
