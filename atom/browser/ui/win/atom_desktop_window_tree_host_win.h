// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <windows.h>

#include <vector>

#include "atom/browser/native_window.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_win.h"

namespace atom {

class MessageHandlerDelegate;

class AtomDesktopWindowTreeHostWin : public views::DesktopWindowTreeHostWin {
 public:
  AtomDesktopWindowTreeHostWin(
      MessageHandlerDelegate* delegate,
      views::internal::NativeWidgetDelegate* native_widget_delegate,
      views::DesktopNativeWidgetAura* desktop_native_widget_aura);
  ~AtomDesktopWindowTreeHostWin() override;

 protected:
  bool PreHandleMSG(
      UINT message, WPARAM w_param, LPARAM l_param, LRESULT* result) override;

 private:
  MessageHandlerDelegate* delegate_;  // weak ref

  DISALLOW_COPY_AND_ASSIGN(AtomDesktopWindowTreeHostWin);
};

}  // namespace atom
