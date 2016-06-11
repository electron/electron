// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_WIN_TASKBAR_HOST_H_
#define ATOM_BROWSER_UI_WIN_TASKBAR_HOST_H_

#include <shobjidl.h>

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/win/scoped_comptr.h"
#include "ui/gfx/image/image.h"

namespace atom {

class TaskbarHost {
 public:
  struct ThumbarButton {
    std::string tooltip;
    gfx::Image icon;
    std::vector<std::string> flags;
    base::Closure clicked_callback;
  };

  TaskbarHost();
  virtual ~TaskbarHost();

  // Add or update the buttons in thumbar.
  bool SetThumbarButtons(
      HWND window, const std::vector<ThumbarButton>& buttons);

  // Set the progress state in taskbar.
  bool SetProgressBar(HWND window, double value);

  // Set the overlay icon in taskbar.
  bool SetOverlayIcon(
      HWND window, const gfx::Image& overlay, const std::string& text);

  // Called by the window that there is a button in thumbar clicked.
  bool HandleThumbarButtonEvent(int button_id);

 private:
  // Initialize the taskbar object.
  bool InitializeTaskbar();

  using CallbackMap = std::map<int, base::Closure>;
  CallbackMap callback_map_;

  // The COM object of taskbar.
  base::win::ScopedComPtr<ITaskbarList3> taskbar_;

  // Whether we have already added the buttons to thumbar.
  bool thumbar_buttons_added_;

  DISALLOW_COPY_AND_ASSIGN(TaskbarHost);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_WIN_TASKBAR_HOST_H_
