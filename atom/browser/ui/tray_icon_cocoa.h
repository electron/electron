// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_TRAY_ICON_COCOA_H_
#define ATOM_BROWSER_UI_TRAY_ICON_COCOA_H_

#import <Cocoa/Cocoa.h>

#include <string>

#include "atom/browser/ui/tray_icon.h"
#include "base/mac/scoped_nsobject.h"

@class AtomMenuController;
@class StatusItemView;

namespace atom {

class TrayIconCocoa : public TrayIcon, public AtomMenuModel::Observer {
 public:
  TrayIconCocoa();
  ~TrayIconCocoa() override;

  void SetImage(const gfx::Image& image) override;
  void SetPressedImage(const gfx::Image& image) override;
  void SetToolTip(const std::string& tool_tip) override;
  void SetTitle(const std::string& title) override;
  std::string GetTitle() override;
  void SetHighlightMode(TrayIcon::HighlightMode mode) override;
  void SetIgnoreDoubleClickEvents(bool ignore) override;
  bool GetIgnoreDoubleClickEvents() override;
  void PopUpOnUI(AtomMenuModel* menu_model);
  void PopUpContextMenu(const gfx::Point& pos,
                        AtomMenuModel* menu_model) override;
  void SetContextMenu(AtomMenuModel* menu_model) override;
  gfx::Rect GetBounds() override;

 protected:
  // AtomMenuModel::Observer:
  void OnMenuWillClose() override;

 private:
  // Atom custom view for NSStatusItem.
  base::scoped_nsobject<StatusItemView> status_item_view_;

  // Status menu shown when right-clicking the system icon.
  base::scoped_nsobject<AtomMenuController> menu_;

  // Used for unregistering observer.
  AtomMenuModel* menu_model_ = nullptr;  // weak ref.

  base::WeakPtrFactory<TrayIconCocoa> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TrayIconCocoa);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_TRAY_ICON_COCOA_H_
