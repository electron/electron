// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_WIN_THUMBAR_HOST_H_
#define ATOM_BROWSER_UI_WIN_THUMBAR_HOST_H_

#include <windows.h>

#include <map>
#include <vector>

#include "atom/browser/native_window.h"

namespace atom {

class ThumbarHost {
 public:
  explicit ThumbarHost(HWND window);
  ~ThumbarHost();

  bool SetThumbarButtons(
      const std::vector<NativeWindow::ThumbarButton>& buttons);
  bool HandleThumbarButtonEvent(int button_id);

 private:
  using ThumbarButtonClickedCallbackMap = std::map<
      int, NativeWindow::ThumbarButtonClickedCallback>;
  ThumbarButtonClickedCallbackMap thumbar_button_clicked_callback_map_;

  bool is_initialized_;

  HWND window_;

  DISALLOW_COPY_AND_ASSIGN(ThumbarHost);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_WIN_THUMBAR_HOST_H_
