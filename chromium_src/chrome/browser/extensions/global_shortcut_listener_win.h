// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_WIN_H_
#define CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_WIN_H_

#include <windows.h>

#include "base/memory/scoped_ptr.h"
#include "chrome/browser/extensions/global_shortcut_listener.h"
#include "ui/gfx/win/singleton_hwnd.h"
#include "ui/gfx/win/singleton_hwnd_observer.h"

namespace extensions {

// Windows-specific implementation of the GlobalShortcutListener class that
// listens for global shortcuts. Handles setting up a keyboard hook and
// forwarding its output to the base class for processing.
class GlobalShortcutListenerWin : public GlobalShortcutListener {
 public:
  GlobalShortcutListenerWin();
  virtual ~GlobalShortcutListenerWin();

 private:
  // The implementation of our Window Proc, called by SingletonHwndObserver.
  void OnWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

  // GlobalShortcutListener implementation.
  virtual void StartListening() override;
  virtual void StopListening() override;
  virtual bool RegisterAcceleratorImpl(
      const ui::Accelerator& accelerator) override;
  virtual void UnregisterAcceleratorImpl(
      const ui::Accelerator& accelerator) override;

  // Whether this object is listening for global shortcuts.
  bool is_listening_;

  // A map of registered accelerators and their registration ids.
  typedef std::map<ui::Accelerator, int> HotkeyIdMap;
  HotkeyIdMap hotkey_ids_;

  scoped_ptr<gfx::SingletonHwndObserver> singleton_hwnd_observer_;

  DISALLOW_COPY_AND_ASSIGN(GlobalShortcutListenerWin);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_WIN_H_
