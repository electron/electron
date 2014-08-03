// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/global_shortcut_listener.h"

#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/accelerators/accelerator.h"

using content::BrowserThread;

namespace extensions {

GlobalShortcutListener::GlobalShortcutListener()
    : shortcut_handling_suspended_(false) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

GlobalShortcutListener::~GlobalShortcutListener() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(accelerator_map_.empty());  // Make sure we've cleaned up.
}

bool GlobalShortcutListener::RegisterAccelerator(
    const ui::Accelerator& accelerator, Observer* observer) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (IsShortcutHandlingSuspended())
    return false;

  AcceleratorMap::const_iterator it = accelerator_map_.find(accelerator);
  if (it != accelerator_map_.end()) {
    // The accelerator has been registered.
    return false;
  }

  if (!RegisterAcceleratorImpl(accelerator)) {
    // If the platform-specific registration fails, mostly likely the shortcut
    // has been registered by other native applications.
    return false;
  }

  if (accelerator_map_.empty())
    StartListening();

  accelerator_map_[accelerator] = observer;
  return true;
}

void GlobalShortcutListener::UnregisterAccelerator(
    const ui::Accelerator& accelerator, Observer* observer) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (IsShortcutHandlingSuspended())
    return;

  AcceleratorMap::iterator it = accelerator_map_.find(accelerator);
  // We should never get asked to unregister something that we didn't register.
  DCHECK(it != accelerator_map_.end());
  // The caller should call this function with the right observer.
  DCHECK(it->second == observer);

  UnregisterAcceleratorImpl(accelerator);
  accelerator_map_.erase(it);
  if (accelerator_map_.empty())
    StopListening();
}

void GlobalShortcutListener::UnregisterAccelerators(Observer* observer) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (IsShortcutHandlingSuspended())
    return;

  AcceleratorMap::iterator it = accelerator_map_.begin();
  while (it != accelerator_map_.end()) {
    if (it->second == observer) {
      AcceleratorMap::iterator to_remove = it++;
      UnregisterAccelerator(to_remove->first, observer);
    } else {
      ++it;
    }
  }
}

void GlobalShortcutListener::SetShortcutHandlingSuspended(bool suspended) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (shortcut_handling_suspended_ == suspended)
    return;

  shortcut_handling_suspended_ = suspended;
  for (AcceleratorMap::iterator it = accelerator_map_.begin();
       it != accelerator_map_.end();
       ++it) {
    // On Linux, when shortcut handling is suspended we cannot simply early
    // return in NotifyKeyPressed (similar to what we do for non-global
    // shortcuts) because we'd eat the keyboard event thereby preventing the
    // user from setting the shortcut. Therefore we must unregister while
    // handling is suspended and register when handling resumes.
    if (shortcut_handling_suspended_)
      UnregisterAcceleratorImpl(it->first);
    else
      RegisterAcceleratorImpl(it->first);
  }
}

bool GlobalShortcutListener::IsShortcutHandlingSuspended() const {
  return shortcut_handling_suspended_;
}

void GlobalShortcutListener::NotifyKeyPressed(
    const ui::Accelerator& accelerator) {
  AcceleratorMap::iterator iter = accelerator_map_.find(accelerator);
  if (iter == accelerator_map_.end()) {
    // This should never occur, because if it does, we have failed to unregister
    // or failed to clean up the map after unregistering the shortcut.
    NOTREACHED();
    return;  // No-one is listening to this key.
  }

  iter->second->OnKeyPressed(accelerator);
}

}  // namespace extensions
