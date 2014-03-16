// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_WIN_NATIVE_MENU_WIN_H_
#define ATOM_BROWSER_UI_WIN_NATIVE_MENU_WIN_H_

#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"

namespace gfx {
class Point;
}

namespace ui {
class MenuModel;
}

namespace views {
class MenuInsertionDelegateWin;
class MenuListener;
}

namespace atom {

class NativeMenuWin {
 public:
  // All of the possible actions that can result from RunMenuAt.
  enum MenuAction {
    MENU_ACTION_NONE,      // Menu cancelled, or never opened.
    MENU_ACTION_SELECTED,  // An item was selected.
    MENU_ACTION_PREVIOUS,  // User wants to navigate to the previous menu.
    MENU_ACTION_NEXT,      // User wants to navigate to the next menu.
  };

  // Construct a NativeMenuWin, with a model and delegate. If |system_menu_for|
  // is non-NULL, the NativeMenuWin wraps the system menu for that window.
  // The caller owns the model and the delegate.
  NativeMenuWin(ui::MenuModel* model, HWND system_menu_for);
  virtual ~NativeMenuWin();

  void RunMenuAt(const gfx::Point& point, int alignment);
  void CancelMenu();
  void Rebuild(views::MenuInsertionDelegateWin* delegate);
  void UpdateStates();
  HMENU GetNativeMenu() const;
  MenuAction GetMenuAction() const;
  void AddMenuListener(views::MenuListener* listener);
  void RemoveMenuListener(views::MenuListener* listener);
  void SetMinimumWidth(int width);

  // Called by user to generate a menu command event.
  void OnMenuCommand(int position, HMENU menu);

  // Flag to create a window menu instead of popup menu.
  void set_create_as_window_menu(bool flag) { create_as_window_menu_ = flag; }
  bool create_as_window_menu() const { return create_as_window_menu_; }

 private:
  // IMPORTANT: Note about indices.
  //            Functions in this class deal in two index spaces:
  //            1. menu_index - the index of an item within the actual Windows
  //               native menu.
  //            2. model_index - the index of the item within our model.
  //            These two are most often but not always the same value! The
  //            notable exception is when this object is used to wrap the
  //            Windows System Menu. In this instance, the model indices start
  //            at 0, but the insertion index into the existing menu is not.
  //            It is important to take this into consideration when editing the
  //            code in the functions in this class.

  struct HighlightedMenuItemInfo;

  // Returns true if the item at the specified index is a separator.
  bool IsSeparatorItemAt(int menu_index) const;

  // Add items. See note above about indices.
  void AddMenuItemAt(int menu_index, int model_index);
  void AddSeparatorItemAt(int menu_index, int model_index);

  // Sets the state of the item at the specified index.
  void SetMenuItemState(int menu_index,
                        bool enabled,
                        bool checked,
                        bool is_default);

  // Sets the label of the item at the specified index.
  void SetMenuItemLabel(int menu_index,
                        int model_index,
                        const string16& label);

  // Updates the local data structure with the correctly formatted version of
  // |label| at the specified model_index, and adds string data to |mii| if
  // the menu is not owner-draw. That's a mouthful. This function exists because
  // of the peculiarities of the Windows menu API.
  void UpdateMenuItemInfoForString(MENUITEMINFO* mii,
                                   int model_index,
                                   const string16& label);

  // Returns the alignment flags to be passed to TrackPopupMenuEx, based on the
  // supplied alignment and the UI text direction.
  UINT GetAlignmentFlags(int alignment) const;

  // Resets the native menu stored in |menu_| by destroying any old menu then
  // creating a new empty one.
  void ResetNativeMenu();

  // Creates the host window that receives notifications from the menu.
  void CreateHostWindow();

  // Callback from task to notify menu it was selected.
  void DelayedSelect();

  // Given a menu that's currently popped-up, find the currently highlighted
  // item. Returns true if a highlighted item was found.
  static bool GetHighlightedMenuItemInfo(HMENU menu,
                                         HighlightedMenuItemInfo* info);

  // Hook to receive keyboard events while the menu is open.
  static LRESULT CALLBACK MenuMessageHook(
      int n_code, WPARAM w_param, LPARAM l_param);

  // Our attached model and delegate.
  ui::MenuModel* model_;

  HMENU menu_;

  // True if the contents of menu items in this menu are drawn by the menu host
  // window, rather than Windows.
  bool owner_draw_;

  // An object that collects all of the data associated with an individual menu
  // item.
  struct ItemData;
  std::vector<ItemData*> items_;

  // The window that receives notifications from the menu.
  class MenuHostWindow;
  friend MenuHostWindow;
  scoped_ptr<MenuHostWindow> host_window_;

  // The HWND this menu is the system menu for, or NULL if the menu is not a
  // system menu.
  HWND system_menu_for_;

  // The index of the first item in the model in the menu.
  int first_item_index_;

  // The action that took place during the call to RunMenuAt.
  MenuAction menu_action_;

  // A list of listeners to call when the menu opens.
  ObserverList<views::MenuListener> listeners_;

  // Keep track of whether the listeners have already been called at least
  // once.
  bool listeners_called_;

  // See comment in MenuMessageHook for details on these.
  NativeMenuWin* menu_to_select_;
  int position_to_select_;
  base::WeakPtrFactory<NativeMenuWin> menu_to_select_factory_;

  // If we're a submenu, this is our parent.
  NativeMenuWin* parent_;

  // If non-null the destructor sets this to true. This is set to non-null while
  // the menu is showing. It is used to detect if the menu was deleted while
  // running.
  bool* destroyed_flag_;

  // Ugly: a static pointer to the instance of this class that currently
  // has a menu open, because our hook function that receives keyboard
  // events doesn't have a mechanism to get a user data pointer.
  static NativeMenuWin* open_native_menu_win_;

  // Create as window menu.
  bool create_as_window_menu_;

  DISALLOW_COPY_AND_ASSIGN(NativeMenuWin);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_WIN_NATIVE_MENU_WIN_H_
