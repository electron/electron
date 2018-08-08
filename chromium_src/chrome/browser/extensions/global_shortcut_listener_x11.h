// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_X11_H_
#define CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_X11_H_

#include <stdint.h>

#include <set>

#include "base/macros.h"
#include "chrome/browser/extensions/global_shortcut_listener.h"
#include "ui/events/platform/platform_event_dispatcher.h"
#include "ui/gfx/x/x11.h"

namespace extensions {

// X11-specific implementation of the GlobalShortcutListener class that
// listens for global shortcuts. Handles basic keyboard intercepting and
// forwards its output to the base class for processing.
class GlobalShortcutListenerX11 : public GlobalShortcutListener,
                                  public ui::PlatformEventDispatcher {
 public:
  GlobalShortcutListenerX11();
  ~GlobalShortcutListenerX11() override;

  // ui::PlatformEventDispatcher implementation.
  bool CanDispatchEvent(const ui::PlatformEvent& event) override;
  uint32_t DispatchEvent(const ui::PlatformEvent& event) override;

 private:
  // GlobalShortcutListener implementation.
  void StartListening() override;
  void StopListening() override;
  bool RegisterAcceleratorImpl(const ui::Accelerator& accelerator) override;
  void UnregisterAcceleratorImpl(const ui::Accelerator& accelerator) override;

  // Invoked when a global shortcut is pressed.
  void OnXKeyPressEvent(::XEvent* x_event);

  // Whether this object is listening for global shortcuts.
  bool is_listening_;

  // The x11 default display and the native root window.
  ::Display* x_display_;
  ::Window x_root_window_;

  // A set of registered accelerators.
  typedef std::set<ui::Accelerator> RegisteredHotKeys;
  RegisteredHotKeys registered_hot_keys_;

  DISALLOW_COPY_AND_ASSIGN(GlobalShortcutListenerX11);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_X11_H_
