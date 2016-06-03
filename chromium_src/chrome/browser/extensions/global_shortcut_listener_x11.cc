// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/global_shortcut_listener_x11.h"

#include <stddef.h>

#include "base/macros.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/keycodes/keyboard_code_conversion_x.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/gfx/x/x11_error_tracker.h"
#include "ui/gfx/x/x11_types.h"

using content::BrowserThread;

namespace {

// The modifiers masks used for grabing keys. Due to XGrabKey only working on
// exact modifiers, we need to grab all key combination including zero or more
// of the following: Num lock, Caps lock and Scroll lock. So that we can make
// sure the behavior of global shortcuts is consistent on all platforms.
const unsigned int kModifiersMasks[] = {
  0,                                // No additional modifier.
  Mod2Mask,                         // Num lock
  LockMask,                         // Caps lock
  Mod5Mask,                         // Scroll lock
  Mod2Mask | LockMask,
  Mod2Mask | Mod5Mask,
  LockMask | Mod5Mask,
  Mod2Mask | LockMask | Mod5Mask
};

int GetNativeModifiers(const ui::Accelerator& accelerator) {
  int modifiers = 0;
  modifiers |= accelerator.IsShiftDown() ? ShiftMask : 0;
  modifiers |= accelerator.IsCtrlDown() ? ControlMask : 0;
  modifiers |= accelerator.IsAltDown() ? Mod1Mask : 0;
  modifiers |= accelerator.IsCmdDown() ? Mod4Mask : 0;

  return modifiers;
}

}  // namespace

namespace extensions {

// static
GlobalShortcutListener* GlobalShortcutListener::GetInstance() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  static GlobalShortcutListenerX11* instance =
      new GlobalShortcutListenerX11();
  return instance;
}

GlobalShortcutListenerX11::GlobalShortcutListenerX11()
    : is_listening_(false),
      x_display_(gfx::GetXDisplay()),
      x_root_window_(DefaultRootWindow(x_display_)) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

GlobalShortcutListenerX11::~GlobalShortcutListenerX11() {
  if (is_listening_)
    StopListening();
}

void GlobalShortcutListenerX11::StartListening() {
  DCHECK(!is_listening_);  // Don't start twice.
  DCHECK(!registered_hot_keys_.empty());  // Also don't start if no hotkey is
                                          // registered.

  ui::PlatformEventSource::GetInstance()->AddPlatformEventDispatcher(this);

  is_listening_ = true;
}

void GlobalShortcutListenerX11::StopListening() {
  DCHECK(is_listening_);  // No point if we are not already listening.
  DCHECK(registered_hot_keys_.empty());  // Make sure the set is clean before
                                         // ending.

  ui::PlatformEventSource::GetInstance()->RemovePlatformEventDispatcher(this);

  is_listening_ = false;
}

bool GlobalShortcutListenerX11::CanDispatchEvent(
    const ui::PlatformEvent& event) {
  return event->type == KeyPress;
}

uint32_t GlobalShortcutListenerX11::DispatchEvent(
    const ui::PlatformEvent& event) {
  CHECK_EQ(KeyPress, event->type);
  OnXKeyPressEvent(event);

  return ui::POST_DISPATCH_NONE;
}

bool GlobalShortcutListenerX11::RegisterAcceleratorImpl(
    const ui::Accelerator& accelerator) {
  DCHECK(registered_hot_keys_.find(accelerator) == registered_hot_keys_.end());

  int modifiers = GetNativeModifiers(accelerator);
  KeyCode keycode = XKeysymToKeycode(x_display_,
      XKeysymForWindowsKeyCode(accelerator.key_code(), false));
  gfx::X11ErrorTracker err_tracker;

  // Because XGrabKey only works on the exact modifiers mask, we should register
  // our hot keys with modifiers that we want to ignore, including Num lock,
  // Caps lock, Scroll lock. See comment about |kModifiersMasks|.
  for (size_t i = 0; i < arraysize(kModifiersMasks); ++i) {
    XGrabKey(x_display_, keycode, modifiers | kModifiersMasks[i],
             x_root_window_, False, GrabModeAsync, GrabModeAsync);
  }

  if (err_tracker.FoundNewError()) {
    // We may have part of the hotkeys registered, clean up.
    for (size_t i = 0; i < arraysize(kModifiersMasks); ++i) {
      XUngrabKey(x_display_, keycode, modifiers | kModifiersMasks[i],
                 x_root_window_);
    }

    return false;
  }

  registered_hot_keys_.insert(accelerator);
  return true;
}

void GlobalShortcutListenerX11::UnregisterAcceleratorImpl(
    const ui::Accelerator& accelerator) {
  DCHECK(registered_hot_keys_.find(accelerator) != registered_hot_keys_.end());

  int modifiers = GetNativeModifiers(accelerator);
  KeyCode keycode = XKeysymToKeycode(x_display_,
      XKeysymForWindowsKeyCode(accelerator.key_code(), false));

  for (size_t i = 0; i < arraysize(kModifiersMasks); ++i) {
    XUngrabKey(x_display_, keycode, modifiers | kModifiersMasks[i],
               x_root_window_);
  }
  registered_hot_keys_.erase(accelerator);
}

void GlobalShortcutListenerX11::OnXKeyPressEvent(::XEvent* x_event) {
  DCHECK(x_event->type == KeyPress);
  int modifiers = 0;
  modifiers |= (x_event->xkey.state & ShiftMask) ? ui::EF_SHIFT_DOWN : 0;
  modifiers |= (x_event->xkey.state & ControlMask) ? ui::EF_CONTROL_DOWN : 0;
  modifiers |= (x_event->xkey.state & Mod1Mask) ? ui::EF_ALT_DOWN : 0;
  modifiers |= (x_event->xkey.state & Mod4Mask) ? ui::EF_COMMAND_DOWN: 0;

  ui::Accelerator accelerator(
      ui::KeyboardCodeFromXKeyEvent(x_event), modifiers);
  if (registered_hot_keys_.find(accelerator) != registered_hot_keys_.end())
    NotifyKeyPressed(accelerator);
}

}  // namespace extensions
