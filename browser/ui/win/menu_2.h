// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_UI_WIN_MENU_2_H_
#define BROWSER_UI_WIN_MENU_2_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "browser/ui/win/native_menu_win.h"
#include "ui/gfx/native_widget_types.h"

namespace gfx {
class Point;
}

namespace ui {
class MenuModel;
}

namespace atom {

// A menu. Populated from a model, and relies on a delegate to execute commands.
//
// WARNING: do NOT create and use Menu2 on the stack. Menu2 notifies the model
// of selection AFTER a delay. This means that if use a Menu2 on the stack
// ActivatedAt is never invoked.
class Menu2 {
 public:
  // How the menu is aligned relative to the point it is shown at.
  // The alignment is reversed by menu if text direction is right to left.
  enum Alignment {
    ALIGN_TOPLEFT,
    ALIGN_TOPRIGHT
  };

  // Creates a new menu populated with the contents of |model|.
  // WARNING: this populates the menu on construction by invoking methods on
  // the model. As such, it is typically not safe to use this as the model
  // from the constructor. EG:
  //   MyClass : menu_(this) {}
  // is likely to have problems.
  explicit Menu2(ui::MenuModel* model, bool as_window_menu = false);
  virtual ~Menu2();

  // Runs the menu at the specified point. This method blocks until done.
  // RunContextMenuAt is the same, but the alignment is the default for a
  // context menu.
  void RunMenuAt(const gfx::Point& point, Alignment alignment);
  void RunContextMenuAt(const gfx::Point& point);

  // Cancels the active menu.
  void CancelMenu();

  // Called when the model supplying data to this menu has changed, and the menu
  // must be rebuilt.
  void Rebuild();

  // Called when the states of the menu items in the menu should be refreshed
  // from the model.
  void UpdateStates();

  // For submenus.
  HMENU GetNativeMenu() const;

  // Get the result of the last call to RunMenuAt to determine whether an
  // item was selected, the user navigated to a next or previous menu, or
  // nothing.
  NativeMenuWin::MenuAction GetMenuAction() const;

  // Add a listener to receive a callback when the menu opens.
  void AddMenuListener(views::MenuListener* listener);

  // Remove a menu listener.
  void RemoveMenuListener(views::MenuListener* listener);

  // Accessors.
  ui::MenuModel* model() const { return model_; }
  NativeMenuWin* wrapper() const { return wrapper_.get(); }

  // Sets the minimum width of the menu.
  void SetMinimumWidth(int width);

 private:
  ui::MenuModel* model_;

  // The object that actually implements the menu.
  scoped_ptr<NativeMenuWin> wrapper_;

  DISALLOW_COPY_AND_ASSIGN(Menu2);
};

}  // namespace atom

#endif  // BROWSER_UI_WIN_MENU_2_H_
