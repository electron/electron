// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_MAC_H_
#define CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_MAC_H_

#include "chrome/browser/extensions/global_shortcut_listener.h"

#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>

#include <map>

#include "base/mac/scoped_nsobject.h"

namespace extensions {

// Mac-specific implementation of the GlobalShortcutListener class that
// listens for global shortcuts. Handles basic keyboard intercepting and
// forwards its output to the base class for processing.
//
// This class does two things:
// 1. Intercepts media keys. Uses an event tap for intercepting media keys
// (PlayPause, NextTrack, PreviousTrack).
// 2. Binds keyboard shortcuts (hot keys). Carbon RegisterEventHotKey API for
// binding to non-media key global hot keys (eg. Command-Shift-1).
class GlobalShortcutListenerMac : public GlobalShortcutListener {
 public:
  GlobalShortcutListenerMac();
  virtual ~GlobalShortcutListenerMac();

 private:
  typedef int KeyId;
  typedef std::map<ui::Accelerator, KeyId> AcceleratorIdMap;
  typedef std::map<KeyId, ui::Accelerator> IdAcceleratorMap;
  typedef std::map<KeyId, EventHotKeyRef> IdHotKeyRefMap;

  // Keyboard event callbacks.
  void OnHotKeyEvent(EventHotKeyID hot_key_id);
  bool OnMediaKeyEvent(int key_code);

  // GlobalShortcutListener implementation.
  virtual void StartListening() OVERRIDE;
  virtual void StopListening() OVERRIDE;
  virtual bool RegisterAcceleratorImpl(
      const ui::Accelerator& accelerator) OVERRIDE;
  virtual void UnregisterAcceleratorImpl(
      const ui::Accelerator& accelerator) OVERRIDE;

  // Mac-specific functions for registering hot keys with modifiers.
  bool RegisterHotKey(const ui::Accelerator& accelerator, KeyId hot_key_id);
  void UnregisterHotKey(const ui::Accelerator& accelerator);

  // Enable and disable the media key event tap.
  void StartWatchingMediaKeys();
  void StopWatchingMediaKeys();

  // Enable and disable the hot key event handler.
  void StartWatchingHotKeys();
  void StopWatchingHotKeys();

  // Whether or not any media keys are currently registered.
  bool IsAnyMediaKeyRegistered();

  // Whether or not any hot keys are currently registered.
  bool IsAnyHotKeyRegistered();

  // The callback for when an event tap happens.
  static CGEventRef EventTapCallback(
      CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* refcon);

  // The callback for when a hot key event happens.
  static OSStatus HotKeyHandler(
      EventHandlerCallRef next_handler, EventRef event, void* user_data);

  // Whether this object is listening for global shortcuts.
  bool is_listening_;

  // The hotkey identifier for the next global shortcut that is added.
  KeyId hot_key_id_;

  // A map of all hotkeys (media keys and shortcuts) mapping to their
  // corresponding hotkey IDs. For quickly finding if an accelerator is
  // registered.
  AcceleratorIdMap accelerator_ids_;

  // The inverse map for quickly looking up accelerators by hotkey id.
  IdAcceleratorMap id_accelerators_;

  // Keyboard shortcut IDs to hotkeys map for unregistration.
  IdHotKeyRefMap id_hot_key_refs_;

  // Event tap for intercepting mac media keys.
  CFMachPortRef event_tap_;
  CFRunLoopSourceRef event_tap_source_;

  // Event handler for keyboard shortcut hot keys.
  EventHandlerRef event_handler_;

  DISALLOW_COPY_AND_ASSIGN(GlobalShortcutListenerMac);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_MAC_H_
