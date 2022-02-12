// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_OBSERVER_H_
#define ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"
#include "base/values.h"
#include "ui/base/window_open_disposition.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif

class GURL;

namespace gfx {
class Rect;
enum class ResizeEdge;
}  // namespace gfx

namespace electron {

class NativeView;

class NativeWindowObserver : public base::CheckedObserver {
 public:
  ~NativeWindowObserver() override {}

  // Called when the web page in window wants to create a popup window.
  virtual void WillCreatePopupWindow(const std::u16string& frame_name,
                                     const GURL& target_url,
                                     const std::string& partition_id,
                                     WindowOpenDisposition disposition) {}

  // Called when user is starting an navigation in web page.
  virtual void WillNavigate(bool* prevent_default, const GURL& url) {}

  // Called when the window is gonna closed.
  virtual void WillCloseWindow(bool* prevent_default) {}

  // Called when the window wants to know the preferred width.
  virtual void RequestPreferredWidth(int* width) {}

  // Called when closed button is clicked.
  virtual void OnCloseButtonClicked(bool* prevent_default) {}

  // Called when the window is closed.
  virtual void OnWindowClosed() {}

  // Called when Windows sends WM_ENDSESSION message
  virtual void OnWindowEndSession() {}

  // Called when window loses focus.
  virtual void OnWindowBlur() {}

  // Called when window gains focus.
  virtual void OnWindowFocus() {}

  // Called when window gained or lost key window status.
  virtual void OnWindowIsKeyChanged(bool is_key) {}

  // Called when window is shown.
  virtual void OnWindowShow() {}

  // Called when window is hidden.
  virtual void OnWindowHide() {}

  // Called when window state changed.
  virtual void OnWindowMaximize() {}
  virtual void OnWindowUnmaximize() {}
  virtual void OnWindowMinimize() {}
  virtual void OnWindowRestore() {}
  virtual void OnWindowWillResize(const gfx::Rect& new_bounds,
                                  const gfx::ResizeEdge& edge,
                                  bool* prevent_default) {}
  virtual void OnWindowResize() {}
  virtual void OnWindowResized() {}
  virtual void OnWindowWillMove(const gfx::Rect& new_bounds,
                                bool* prevent_default) {}
  virtual void OnWindowMove() {}
  virtual void OnWindowMoved() {}
  virtual void OnWindowScrollTouchBegin() {}
  virtual void OnWindowScrollTouchEnd() {}
  virtual void OnWindowSwipe(const std::string& direction) {}
  virtual void OnWindowRotateGesture(float rotation) {}
  virtual void OnWindowSheetBegin() {}
  virtual void OnWindowSheetEnd() {}
  virtual void OnWindowEnterFullScreen() {}
  virtual void OnWindowLeaveFullScreen() {}
  virtual void OnWindowEnterHtmlFullScreen() {}
  virtual void OnWindowLeaveHtmlFullScreen() {}
  virtual void OnWindowAlwaysOnTopChanged() {}
  virtual void OnTouchBarItemResult(const std::string& item_id,
                                    const base::DictionaryValue& details) {}
  virtual void OnNewWindowForTab() {}
  virtual void OnSystemContextMenu(int x, int y, bool* prevent_default) {}

// Called when window message received
#if BUILDFLAG(IS_WIN)
  virtual void OnWindowMessage(UINT message, WPARAM w_param, LPARAM l_param) {}
#endif

  // Called on Windows when App Commands arrive (WM_APPCOMMAND)
  // Some commands are implemented on on other platforms as well
  virtual void OnExecuteAppCommand(const std::string& command_name) {}

  virtual void UpdateWindowControlsOverlay(const gfx::Rect& bounding_rect) {}

  virtual void OnChildViewDetached(NativeView* view) {}
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_OBSERVER_H_
