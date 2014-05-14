// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_WINDOW_GTK_H_
#define ATOM_BROWSER_NATIVE_WINDOW_GTK_H_

#include <gtk/gtk.h>

#include <string>
#include <vector>

#include "atom/browser/native_window.h"
#include "atom/browser/ui/accelerator_util.h"
#include "chrome/browser/ui/gtk/menu_gtk.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/gtk/gtk_signal.h"
#include "ui/base/x/active_window_watcher_x_observer.h"
#include "ui/gfx/size.h"

namespace atom {

class NativeWindowGtk : public NativeWindow,
                        public MenuGtk::Delegate,
                        public ui::ActiveWindowWatcherXObserver {
 public:
  explicit NativeWindowGtk(content::WebContents* web_contents,
                           base::DictionaryValue* options);
  virtual ~NativeWindowGtk();

  // NativeWindow implementation.
  virtual void Close() OVERRIDE;
  virtual void CloseImmediately() OVERRIDE;
  virtual void Move(const gfx::Rect& pos) OVERRIDE;
  virtual void Focus(bool focus) OVERRIDE;
  virtual bool IsFocused() OVERRIDE;
  virtual void Show() OVERRIDE;
  virtual void Hide() OVERRIDE;
  virtual bool IsVisible() OVERRIDE;
  virtual void Maximize() OVERRIDE;
  virtual void Unmaximize() OVERRIDE;
  virtual void Minimize() OVERRIDE;
  virtual void Restore() OVERRIDE;
  virtual void SetFullscreen(bool fullscreen) OVERRIDE;
  virtual bool IsFullscreen() OVERRIDE;
  virtual void SetSize(const gfx::Size& size) OVERRIDE;
  virtual gfx::Size GetSize() OVERRIDE;
  virtual void SetMinimumSize(const gfx::Size& size) OVERRIDE;
  virtual gfx::Size GetMinimumSize() OVERRIDE;
  virtual void SetMaximumSize(const gfx::Size& size) OVERRIDE;
  virtual gfx::Size GetMaximumSize() OVERRIDE;
  virtual void SetResizable(bool resizable) OVERRIDE;
  virtual bool IsResizable() OVERRIDE;
  virtual void SetAlwaysOnTop(bool top) OVERRIDE;
  virtual bool IsAlwaysOnTop() OVERRIDE;
  virtual void Center() OVERRIDE;
  virtual void SetPosition(const gfx::Point& position) OVERRIDE;
  virtual gfx::Point GetPosition() OVERRIDE;
  virtual void SetTitle(const std::string& title) OVERRIDE;
  virtual std::string GetTitle() OVERRIDE;
  virtual void FlashFrame(bool flash) OVERRIDE;
  virtual void SetKiosk(bool kiosk) OVERRIDE;
  virtual bool IsKiosk() OVERRIDE;
  virtual gfx::NativeWindow GetNativeWindow() OVERRIDE;

  // Set the native window menu.
  void SetMenu(ui::MenuModel* menu_model);

 protected:
  virtual void UpdateDraggableRegions(
      const std::vector<DraggableRegion>& regions) OVERRIDE;

  // Overridden from content::WebContentsDelegate:
  virtual void HandleKeyboardEvent(
      content::WebContents*,
      const content::NativeWebKeyboardEvent&) OVERRIDE;

  // Overridden from ActiveWindowWatcherXObserver.
  virtual void ActiveWindowChanged(GdkWindow* active_window) OVERRIDE;

 private:
  // Register accelerators supported by the menu model.
  void RegisterAccelerators();

  // Set WebKit's style from current theme.
  void SetWebKitColorStyle();

  // Set how font is renderered.
  void SetFontRenderering();

  // Whether window is maximized.
  bool IsMaximized() const;

  // If the point (|x|, |y|) is within the resize border area of the window,
  // returns true and sets |edge| to the appropriate GdkWindowEdge value.
  // Otherwise, returns false.
  bool GetWindowEdge(int x, int y, GdkWindowEdge* edge);

  CHROMEGTK_CALLBACK_1(NativeWindowGtk, gboolean, OnWindowDeleteEvent,
                       GdkEvent*);
  CHROMEGTK_CALLBACK_1(NativeWindowGtk, gboolean, OnFocusOut, GdkEventFocus*);
  CHROMEGTK_CALLBACK_1(NativeWindowGtk, gboolean, OnWindowState,
                       GdkEventWindowState*);

  // Mouse move and mouse button press callbacks.
  CHROMEGTK_CALLBACK_1(NativeWindowGtk, gboolean, OnMouseMoveEvent,
                       GdkEventMotion*);
  CHROMEGTK_CALLBACK_1(NativeWindowGtk, gboolean, OnButtonPress,
                       GdkEventButton*);

  GtkWindow* window_;
  GtkWidget* vbox_;

  GdkWindowState state_;
  bool is_always_on_top_;
  gfx::Size minimum_size_;
  gfx::Size maximum_size_;

  // The region is treated as title bar, can be dragged to move and double
  // clicked to maximize.
  scoped_ptr<SkRegion> draggable_region_;

  // True if the window manager thinks the window is active. It could happpen
  // that the WM thinks a window is active but it's actually not, like when
  // showing a context menu.
  bool is_active_;

  // If true, don't call gdk_window_raise() when we get a click in the title
  // bar or window border.  This is to work around a compiz bug.
  bool suppress_window_raise_;

  // True if the window has been visible for once, on Linux the window frame
  // would // only be considered as part of the window untill the window has
  // been shown, so we need it to correctly set the window size.
  bool has_ever_been_shown_;

  // The current window cursor.  We set it to a resize cursor when over the
  // custom frame border.  We set it to NULL if we want the default cursor.
  GdkCursor* frame_cursor_;

  // The window menu.
  scoped_ptr<MenuGtk> menu_;

  // Map from accelerator to menu item's command id.
  accelerator_util::AcceleratorTable accelerator_table_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowGtk);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_WINDOW_GTK_H_
