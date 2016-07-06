// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_WIN_NOTIFY_ICON_H_
#define ATOM_BROWSER_UI_WIN_NOTIFY_ICON_H_

#include <windows.h>
#include <shellapi.h>

#include <string>

#include "atom/browser/ui/tray_icon.h"
#include "base/macros.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/win/scoped_gdi_object.h"

namespace gfx {
class Point;
}

namespace atom {

class NotifyIconHost;

class NotifyIcon : public TrayIcon {
 public:
  // Constructor which provides this icon's unique ID and messaging window.
  NotifyIcon(NotifyIconHost* host, UINT id, HWND window, UINT message);
  virtual ~NotifyIcon();

  // Handles a click event from the user - if |left_button_click| is true and
  // there is a registered observer, passes the click event to the observer,
  // otherwise displays the context menu if there is one.
  void HandleClickEvent(int modifiers,
                        bool left_button_click,
                        bool double_button_click);

  // Re-creates the status tray icon now after the taskbar has been created.
  void ResetIcon();

  UINT icon_id() const { return icon_id_; }
  HWND window() const { return window_; }
  UINT message_id() const { return message_id_; }

  // Overridden from TrayIcon:
  void SetImage(HICON image) override;
  void SetPressedImage(HICON image) override;
  void SetToolTip(const std::string& tool_tip) override;
  void DisplayBalloon(HICON icon,
                      const base::string16& title,
                      const base::string16& contents) override;
  void PopUpContextMenu(const gfx::Point& pos,
                        AtomMenuModel* menu_model) override;
  void SetContextMenu(AtomMenuModel* menu_model) override;
  gfx::Rect GetBounds() override;

 private:
  void InitIconData(NOTIFYICONDATA* icon_data);

  // The tray that owns us.  Weak.
  NotifyIconHost* host_;

  // The unique ID corresponding to this icon.
  UINT icon_id_;

  // Window used for processing messages from this icon.
  HWND window_;

  // The message identifier used for status icon messages.
  UINT message_id_;

  // The currently-displayed icon for the window.
  base::win::ScopedHICON icon_;

  // The context menu.
  AtomMenuModel* menu_model_;

  DISALLOW_COPY_AND_ASSIGN(NotifyIcon);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_WIN_NOTIFY_ICON_H_
