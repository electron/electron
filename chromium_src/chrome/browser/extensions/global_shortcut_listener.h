// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_H_
#define CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_H_

#include <map>

#include "base/macros.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace ui {
class Accelerator;
}

namespace extensions {

// Platform-neutral implementation of a class that keeps track of observers and
// monitors keystrokes. It relays messages to the appropriate observer when a
// global shortcut has been struck by the user.
class GlobalShortcutListener {
 public:
  class Observer {
   public:
    // Called when your global shortcut (|accelerator|) is struck.
    virtual void OnKeyPressed(const ui::Accelerator& accelerator) = 0;
  };

  virtual ~GlobalShortcutListener();

  static GlobalShortcutListener* GetInstance();

  // Register an observer for when a certain |accelerator| is struck. Returns
  // true if register successfully, or false if 1) the specificied |accelerator|
  // has been registered by another caller or other native applications, or
  // 2) shortcut handling is suspended.
  //
  // Note that we do not support recognizing that an accelerator has been
  // registered by another application on all platforms. This is a per-platform
  // consideration.
  bool RegisterAccelerator(const ui::Accelerator& accelerator,
                           Observer* observer);

  // Stop listening for the given |accelerator|, does nothing if shortcut
  // handling is suspended.
  void UnregisterAccelerator(const ui::Accelerator& accelerator,
                             Observer* observer);

  // Stop listening for all accelerators of the given |observer|, does nothing
  // if shortcut handling is suspended.
  void UnregisterAccelerators(Observer* observer);

  // Suspend/Resume global shortcut handling. Note that when suspending,
  // RegisterAccelerator/UnregisterAccelerator/UnregisterAccelerators are not
  // allowed to be called until shortcut handling has been resumed.
  void SetShortcutHandlingSuspended(bool suspended);

  // Returns whether shortcut handling is currently suspended.
  bool IsShortcutHandlingSuspended() const;

 protected:
  GlobalShortcutListener();

  // Called by platform specific implementations of this class whenever a key
  // is struck. Only called for keys that have an observer registered.
  void NotifyKeyPressed(const ui::Accelerator& accelerator);

 private:
  // The following methods are implemented by platform-specific implementations
  // of this class.
  //
  // Start/StopListening are called when transitioning between zero and nonzero
  // registered accelerators. StartListening will be called after
  // RegisterAcceleratorImpl and StopListening will be called after
  // UnregisterAcceleratorImpl.
  //
  // For RegisterAcceleratorImpl, implementations return false if registration
  // did not complete successfully.
  virtual void StartListening() = 0;
  virtual void StopListening() = 0;
  virtual bool RegisterAcceleratorImpl(const ui::Accelerator& accelerator) = 0;
  virtual void UnregisterAcceleratorImpl(
      const ui::Accelerator& accelerator) = 0;

  // The map of accelerators that have been successfully registered as global
  // shortcuts and their observer.
  typedef std::map<ui::Accelerator, Observer*> AcceleratorMap;
  AcceleratorMap accelerator_map_;

  // Keeps track of whether shortcut handling is currently suspended.
  bool shortcut_handling_suspended_;

  DISALLOW_COPY_AND_ASSIGN(GlobalShortcutListener);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_H_
