// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_X11_H_
#define SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_X11_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "ui/base/glib/glib_signal.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/x/xproto.h"

typedef struct _DbusmenuMenuitem DbusmenuMenuitem;
typedef struct _DbusmenuServer DbusmenuServer;

namespace ui {
class Accelerator;
}

namespace electron {

class NativeWindowViews;

// Controls the Mac style menu bar on Unity.
//
// Unity has an Apple-like menu bar at the top of the screen that changes
// depending on the active window. In the GTK port, we had a hidden GtkMenuBar
// object in each GtkWindow which existed only to be scrapped by the
// libdbusmenu-gtk code. Since we don't have GtkWindows anymore, we need to
// interface directly with the lower level libdbusmenu-glib, which we
// opportunistically dlopen() since not everyone is running Ubuntu.
//
// This class is like the chrome's corresponding one, but it generates the menu
// from menu models instead, and it is also per-window specific.
class GlobalMenuBarX11 {
 public:
  explicit GlobalMenuBarX11(NativeWindowViews* window);
  virtual ~GlobalMenuBarX11();

  // Creates the object path for DbusmenuServer which is attached to |window|.
  static std::string GetPathForWindow(x11::Window window);

  void SetMenu(ElectronMenuModel* menu_model);
  bool IsServerStarted() const;

  // Called by NativeWindow when it show/hides.
  void OnWindowMapped();
  void OnWindowUnmapped();

 private:
  // Creates a DbusmenuServer.
  void InitServer(x11::Window window);

  // Create a menu from menu model.
  void BuildMenuFromModel(ElectronMenuModel* model, DbusmenuMenuitem* parent);

  // Sets the accelerator for |item|.
  void RegisterAccelerator(DbusmenuMenuitem* item,
                           const ui::Accelerator& accelerator);

  CHROMEG_CALLBACK_1(GlobalMenuBarX11,
                     void,
                     OnItemActivated,
                     DbusmenuMenuitem*,
                     unsigned int);
  CHROMEG_CALLBACK_0(GlobalMenuBarX11, void, OnSubMenuShow, DbusmenuMenuitem*);

  NativeWindowViews* window_;
  x11::Window xwindow_;

  DbusmenuServer* server_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(GlobalMenuBarX11);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_X11_H_
